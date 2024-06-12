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
/// Feb 3, 2023
#pragma once

#include <jsonifier/JsonStructuralIterator.hpp>
#include <jsonifier/Validator.hpp>
#include <jsonifier/HashMap.hpp>
#include <jsonifier/String.hpp>
#include <jsonifier/Error.hpp>
#include <jsonifier/Simd.hpp>

namespace jsonifier {

	struct parse_options {
		bool refreshString{ true };
		bool validateJson{ false };
		bool minified{ false };
	};

}

namespace jsonifier_internal {

	template<typename derived_type> class parser;

	template<typename derived_type> struct parse_options_internal {
		mutable parser<derived_type>* parserPtr{};
		jsonifier::parse_options optionsReal{};
		mutable string_view_ptr rootIter{};
	};

	JSONIFIER_INLINE const char* getEndPtr(const char* ptr) {
		while (whitespaceTable[*ptr]) {
			--ptr;
		}
		return ptr;
	}

	template<typename derived_type, typename value_type> struct parse_impl;

	template<typename derived_type> class parser {
	  public:
		template<const auto& options, typename value_type, typename iterator_type> friend void parseString(value_type&& value, iterator_type&& iter, iterator_type&& end);
		template<const auto& options, typename value_type, json_structural_iterator_t iterator_type>
		friend void parseString(value_type&& value, iterator_type&& iter, iterator_type&& end);
		template<const auto& options, typename value_type, typename iterator_type> friend jsonifier::string_view parseKey(iterator_type&& iter, iterator_type&& end);
		template<const auto& options, typename value_type, json_structural_iterator_t iterator_type>
		friend jsonifier::string_view parseKey(iterator_type&& iter, iterator_type&& end);
		template<const auto& options, typename value_type, json_structural_iterator_t iterator_type>
		friend void parseNumber(value_type&& value, iterator_type&& iter, iterator_type&& end);
		template<const auto& options, typename value_type, typename iterator_type> friend void parseNumber(value_type&& value, iterator_type&& iter, iterator_type&& end);

		template<typename derived_type_new, typename value_type> friend struct parse_impl;

		JSONIFIER_INLINE parser& operator=(const parser& other) = delete;
		JSONIFIER_INLINE parser(const parser& other)			= delete;

		template<jsonifier::parse_options options = jsonifier::parse_options{}, typename value_type, jsonifier::concepts::string_t buffer_type>
		JSONIFIER_INLINE bool parseJson(value_type&& object, buffer_type&& in) {
			static_assert(jsonifier::concepts::printErrorFunction<jsonifier::concepts::unwrap_t<value_type>>(),
				"No specialization of core exists for the type named above - please specialize it!");
			if constexpr (options.validateJson) {
				if (!derivedRef.validateJson(in)) {
					return false;
				}
			}
			derivedRef.errors.clear();
			static constexpr parse_options_internal<derived_type> optionsReal{ .optionsReal = options };
			optionsReal.parserPtr = this;
			optionsReal.rootIter  = reinterpret_cast<string_view_ptr>(in.data());
			if constexpr (!options.minified) {
				derivedRef.section.reset(in.data(), in.size());
				json_structural_iterator iter{ derivedRef.section.begin(), derivedRef.section.end(), in.data() };
				json_structural_iterator end{ derivedRef.section.begin(), derivedRef.section.end(), in.data() };
				if (!iter || (*iter != '{' && *iter != '[')) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return false;
				}
				parse_impl<derived_type, value_type>::template impl<optionsReal>(std::forward<value_type>(object), iter, end);
				if (iter < end) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return false;
				}
			} else {
				auto iter{ in.data() };
				auto end{ getEndPtr(in.data() + in.size()) };
				if (!iter || (*iter != '{' && *iter != '[')) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return false;
				}
				parse_impl<derived_type, value_type>::template impl<optionsReal>(std::forward<value_type>(object), iter, end);
				if (iter < end) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return false;
				}
			}
			return true;
		}

		template<typename value_type, jsonifier::parse_options options = jsonifier::parse_options{}, jsonifier::concepts::string_t buffer_type>
		JSONIFIER_INLINE value_type parseJson(buffer_type&& in) {
			static_assert(jsonifier::concepts::printErrorFunction<jsonifier::concepts::unwrap_t<value_type>>(),
				"No specialization of core exists for the type named above - please specialize it!");
			if constexpr (options.validateJson) {
				if (!derivedRef.validateJson(in)) {
					return value_type{};
				}
			}
			derivedRef.errors.clear();
			static constexpr parse_options_internal<derived_type> optionsReal{ .optionsReal = options };
			optionsReal.parserPtr = this;
			optionsReal.rootIter  = reinterpret_cast<string_view_ptr>(in.data());
			jsonifier::concepts::unwrap_t<value_type> object{};
			if constexpr (!options.minified) {
				derivedRef.section.reset(in.data(), in.size());
				json_structural_iterator iter{ derivedRef.section.begin(), derivedRef.section.end(), in.data() };
				json_structural_iterator end{ derivedRef.section.begin(), derivedRef.section.end(), in.data() };
				if (!iter || (*iter != '{' && *iter != '[')) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return object;
				}

				parse_impl<derived_type, value_type>::template impl<optionsReal>(std::forward<value_type>(object), iter, end);
				if (iter < end) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return object;
				}
			} else {
				auto iter{ in.data() };
				auto end{ getEndPtr(in.data() + in.size()) };
				if (!iter || (*iter != '{' && *iter != '[')) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return object;
				}
				parse_impl<derived_type, value_type>::template impl<optionsReal>(std::forward<value_type>(object), iter, end);
				if (iter < end) {
					static constexpr auto sourceLocation{ std::source_location::current() };
					getErrors().emplace_back(error::constructError<sourceLocation, error_classes::Parsing, parse_errors::No_Input>(iter - optionsReal.rootIter,
						end - optionsReal.rootIter, optionsReal.rootIter));
					return object;
				}
			}
			return object;
		}

	  protected:
		derived_type& derivedRef{ initializeSelfRef() };

		JSONIFIER_INLINE parser() noexcept : derivedRef{ initializeSelfRef() } {};

		JSONIFIER_INLINE derived_type& initializeSelfRef() {
			return *static_cast<derived_type*>(this);
		}

		JSONIFIER_INLINE jsonifier::string& getStringBuffer() {
			return derivedRef.stringBuffer;
		}

		JSONIFIER_INLINE jsonifier::vector<error>& getErrors() {
			return derivedRef.errors;
		}

		JSONIFIER_INLINE ~parser() noexcept = default;
	};
};