/*
	MIT License

	Copyright (c) 2023 RealTimeChris

	Permission is hereby granted, free of charge, to any person obtaining a copy of this
	software and associated documentation files (the "Software"), to deal in the Software
	without restriction, including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software, and to permit
	persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or
	substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
	FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/
/// https://github.com/RealTimeChris/jsonifier
/// Feb 20, 2023
#pragma once

#include <jsonifier/Serializer.hpp>
#include <jsonifier/Parser.hpp>
#include <jsonifier/ISADetection.hpp>
#include <algorithm>

namespace jsonifier_internal {

	template<typename derived_type, jsonifier::concepts::jsonifier_value_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::jsonifier_value_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			static constexpr auto numMembers = std::tuple_size_v<jsonifier::concepts::core_t<value_type_new>>;
			writeObjectEntry<numMembers, options>(buffer, index);

			if constexpr (numMembers > 0) {
				serializeObjects<options, numMembers, 0>(value, buffer, index);
			}

			writeObjectExit<numMembers, options>(buffer, index);
		}

		template<const serialize_options_internal& options, uint64_t size, uint64_t indexNew = 0, jsonifier::concepts::jsonifier_value_t value_type,
			jsonifier::concepts::buffer_like buffer_type, jsonifier::concepts::uint64_type index_type>
		static void serializeObjects(value_type&& value, buffer_type&& buffer, index_type&& index) {
			static constexpr auto group = std::get<indexNew>(jsonifier::concepts::core_v<jsonifier::concepts::unwrap_t<value_type>>);

			static constexpr jsonifier::string_view key = std::get<0>(group);
			if constexpr (jsonifier::concepts::has_excluded_keys<value_type>) {
				auto& keys = value.jsonifierExcludedKeys;
				if (keys.find(static_cast<typename jsonifier::concepts::unwrap_t<decltype(keys)>::key_type>(key)) != keys.end()) {
					if constexpr (indexNew < size - 1) {
						serializeObjects<options, size, indexNew + 1>(value, buffer, index);
					} else {
						return;
					}
				}
			}

			static constexpr auto quotedKey = joinV < chars<"\"">, key, options.optionsReal.prettify ? chars<"\": "> : chars < "\":" >> ;
			writeCharacters<quotedKey>(buffer, index);

			static constexpr auto frozenMap = makeMap<value_type>();
			static constexpr auto memberIt	= frozenMap.find(key);
			static_assert(memberIt != frozenMap.end(), "Invalid key passed to partial write");
			std::visit(
				[&](auto&& memberPtr) {
					auto& newMember	  = getMember(value, memberPtr);
					using member_type = jsonifier::concepts::unwrap_t<decltype(newMember)>;
					serialize_impl<derived_type, member_type>::template impl<options>(newMember, buffer, index);
				},
				std::move(memberIt->second));

			if constexpr (indexNew != size - 1) {
				if constexpr (options.optionsReal.prettify) {
					if (auto k = index + options.indent + 256; k > buffer.size()) [[unlikely]] {
						buffer.resize(max(buffer.size() * 2, k));
					}
					writeCharactersUnchecked<",\n">(buffer, index);
					writeCharactersUnchecked<' '>(options.indent * options.optionsReal.indentSize, buffer, index);
				} else {
					writeCharacter<','>(buffer, index);
				}
			}


			if constexpr (indexNew < size - 1) {
				serializeObjects<options, size, indexNew + 1>(value, buffer, index);
			}
		}
	};

	template<typename derived_type, jsonifier::concepts::jsonifier_scalar_value_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::jsonifier_scalar_value_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			static constexpr auto size{ std::tuple_size_v<jsonifier::concepts::core_t<value_type_new>> };
			if constexpr (size > 0) {
				auto& newMember	  = getMember(value, std::get<0>(jsonifier::concepts::core_v<value_type_new>));
				using member_type = jsonifier::concepts::unwrap_t<decltype(newMember)>;
				serialize_impl<derived_type, member_type>::template impl<options>(newMember, buffer, index);
			}
		}
	};

	template<const serialize_options_internal& options, typename derived_type, typename key_type, typename value_type, jsonifier::concepts::buffer_like buffer_type,
		jsonifier::concepts::uint64_type index_type>
	JSONIFIER_INLINE void writePairContent(const key_type& key, value_type&& value, buffer_type&& buffer, index_type&& index) noexcept {
		serialize_impl<derived_type, key_type>::template impl<options>(key, buffer, index);
		if constexpr (options.optionsReal.prettify) {
			writeCharacters<": ">(buffer, index);
		} else {
			writeCharacter<':'>(buffer, index);
		}
		serialize_impl<derived_type, value_type>::template impl<options>(std::forward<value_type>(value), buffer, index);
	}

	template<typename derived_type, jsonifier::concepts::map_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::map_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			writeObjectEntry<options>(buffer, index, value.size());

			auto iter = std::begin(value);
			auto end  = std::end(value);
			writePairContent<options, derived_type>(iter->first, iter->second, buffer, index);
			for (++iter; iter != end; ++iter) {
				writeEntrySeparator<options>(buffer, index);

				writePairContent<options, derived_type>(iter->first, iter->second, buffer, index);
			}

			writeObjectExit<options>(buffer, index, value.size());
		}
	};

	template<typename derived_type, jsonifier::concepts::variant_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::variant_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			std::visit(
				[&](auto&& valueNew) {
					using member_type = decltype(valueNew);
					serialize_impl<derived_type, member_type>::template impl<options>(valueNew, buffer, index);
				},
				value);
		}
	};

	template<typename derived_type, jsonifier::concepts::optional_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::optional_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			if (value) {
				using member_type = typename jsonifier::concepts::unwrap_t<value_type_new>::value_type;
				serialize_impl<derived_type, member_type>::template impl<options>(*value, buffer, index);
			} else {
				writeCharacters<"null">(buffer, index);
			}
		}
	};

	template<typename derived_type, jsonifier::concepts::array_tuple_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::array_tuple_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			static constexpr auto size = std::tuple_size_v<jsonifier::concepts::unwrap_t<value_type>>;
			writeArrayEntry<options>(buffer, index, size);
			serializeObjects<options, size, 0>(value, buffer, index);
			writeArrayExit<options>(buffer, index, size);
		}

		template<const serialize_options_internal& options, uint64_t size, uint64_t indexNew = 0, bool areWeFirst = true, jsonifier::concepts::array_tuple_t value_type,
			jsonifier::concepts::buffer_like buffer_type, jsonifier::concepts::uint64_type index_type>
		static void serializeObjects(value_type&& value, buffer_type&& buffer, index_type&& index) {
			auto& item = std::get<indexNew>(value);

			if constexpr (indexNew > 0 && !areWeFirst) {
				writeEntrySeparator<options>(buffer, index);
			}
			using member_type = jsonifier::concepts::unwrap_t<decltype(item)>;
			serialize_impl<derived_type, member_type>::template impl<options>(item, buffer, index);
			if constexpr (indexNew < size - 1) {
				serializeObjects<options, size, indexNew + 1, false>(value, buffer, index);
			}
		}
	};

	template<typename derived_type, jsonifier::concepts::vector_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::vector_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			auto size = value.size();
			writeArrayEntry<options>(buffer, index, size);

			if (size != 0) {
				using member_type = typename jsonifier::concepts::unwrap_t<value_type_new>::value_type;
				auto iter		  = value.begin();
				serialize_impl<derived_type, member_type>::template impl<options>(*iter, buffer, index);
				++iter;
				for (auto fin = value.end(); iter != fin; ++iter) {
					writeEntrySeparator<options>(buffer, index);
					serialize_impl<derived_type, member_type>::template impl<options>(*iter, buffer, index);
				}
			}
			writeArrayExit<options>(buffer, index, size);
		}
	};

	template<typename derived_type, jsonifier::concepts::pointer_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::pointer_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type& value, buffer_type&& buffer, index_type&& index) {
			using member_type = jsonifier::concepts::unwrap_t<decltype(*value)>;
			serialize_impl<derived_type, member_type>::template impl<options>(*value, buffer, index);
		}
	};

	template<typename derived_type, jsonifier::concepts::raw_array_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::raw_array_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type& value, buffer_type&& buffer, index_type&& index) {
			using member_type		   = jsonifier::concepts::unwrap_t<decltype(value[0])>;
			static constexpr auto size = std::size(value);
			writeArrayEntry<options>(buffer, index, size);
			if constexpr (size > 0) {
				auto newPtr = value.data();
				serialize_impl<derived_type, member_type>::template impl<options>(*newPtr, buffer, index);
				++newPtr;
				for (uint64_t x = 1; x < size; ++x) {
					writeEntrySeparator<options>(buffer, index);
					serialize_impl<derived_type, member_type>::template impl<options>(*newPtr, buffer, index);
					++newPtr;
				}
			}
			writeArrayExit<options>(buffer, index, size);
		}
	};

	template<typename derived_type, jsonifier::concepts::raw_json_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::raw_json_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			using member_type = jsonifier::string;
			serialize_impl<derived_type, member_type>::template impl<options>(static_cast<const jsonifier::string>(value), buffer, index);
		}
	};

	template<typename derived_type, jsonifier::concepts::string_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::string_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			auto valueSize = value.size();
			auto k		   = index + 10 + (valueSize * 2);
			if (k >= buffer.size()) [[unlikely]] {
				buffer.resize(max(buffer.size() * 2, k));
			}
			writeCharacter<'"'>(buffer, index);
			auto newPtr = buffer.data() + index;
			serializeStringImpl(value.data(), newPtr, valueSize);
			index += newPtr - (buffer.data() + index);
			writeCharacter<'"'>(buffer, index);
		}
	};

	template<typename derived_type, jsonifier::concepts::char_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::char_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			writeCharacter<'"'>(buffer, index);
			switch (value) {
				[[unlikely]] case '\b': {
					writeCharacters<"\\b">(buffer, index);
					break;
				}
				[[unlikely]] case '\t': {
					writeCharacters<"\\t">(buffer, index);
					break;
				}
				[[unlikely]] case '\n': {
					writeCharacters<"\\n">(buffer, index);
					break;
				}
				[[unlikely]] case '\f': {
					writeCharacters<"\\f">(buffer, index);
					break;
				}
				[[unlikely]] case '\r': {
					writeCharacters<"\\r">(buffer, index);
					break;
				}
				[[unlikely]] case '"': {
					writeCharacters<"\\\"">(buffer, index);
					break;
				}
				[[unlikely]] case '\\': {
					writeCharacters<"\\\\">(buffer, index);
					break;
				}
				[[likely]] default: { writeCharacter(buffer, index, value); }
			}
			writeCharacter<'"'>(buffer, index);
		}
	};

	template<typename derived_type, jsonifier::concepts::unique_ptr_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::unique_ptr_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			using member_type = jsonifier::concepts::unwrap_t<decltype(*value)>;
			serialize_impl<derived_type, member_type>::template impl<options>(*value, buffer, index);
		}
	};

	template<typename derived_type, jsonifier::concepts::enum_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::enum_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			auto k = index + 32;
			if (k >= buffer.size()) [[unlikely]] {
				buffer.resize(max(buffer.size() * 2, k));
			}
			int64_t valueNew{ static_cast<int64_t>(value) };
			index = toChars(buffer.data() + index, valueNew) - buffer.data();
		}
	};

	template<typename derived_type, jsonifier::concepts::always_null_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::always_null_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&&, buffer_type&& buffer, index_type&& index) {
			writeCharacters<"null">(buffer, index);
		}
	};

	template<typename derived_type, jsonifier::concepts::bool_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::bool_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			if (value) {
				writeCharacters<"true">(buffer, index);
			} else {
				writeCharacters<"false">(buffer, index);
			}
		}
	};

	template<typename derived_type, jsonifier::concepts::num_t value_type_new> struct serialize_impl<derived_type, value_type_new> {
		template<const serialize_options_internal& options, jsonifier::concepts::num_t value_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(value_type&& value, buffer_type&& buffer, index_type&& index) {
			auto k = index + 32;
			if (k >= buffer.size()) [[unlikely]] {
				buffer.resize(max(buffer.size() * 2, k));
			}
			index = toChars(buffer.data() + index, value) - buffer.data();
		}
	};

}