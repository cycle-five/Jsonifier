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

#include <jsonifier/JsonStructuralIterator.hpp>
#include <jsonifier/Error.hpp>
#include <jsonifier/ISADetection.hpp>

namespace jsonifier_internal {

	template<typename iterator_type> JSONIFIER_INLINE void skipNumber(iterator_type&& iter, iterator_type&& end) noexcept {
		iter += *iter == '-';
		auto sigStartIt	 = iter;
		auto fracStartIt = end;
		auto fracStart	 = [&]() {
			  fracStartIt = iter;
			  iter		  = std::find_if_not(iter, end, isNumberType);
			  if (iter == fracStartIt) {
				  return true;
			  }
			  if ((*iter | ('E' ^ 'e')) != 'e') {
				  return true;
			  }
			  ++iter;
			  return false;
		};

		auto expStart = [&]() {
			iter += *iter == '+' || *iter == '-';
			auto expStartIt = iter;
			iter			= std::find_if_not(iter, end, isNumberType);
			if (iter == expStartIt) {
				return true;
			}
			return false;
		};
		if (*iter == '0') {
			++iter;
			if (*iter != '.') {
				return;
			}
			++iter;
			if (fracStart()) {
				return;
			}
		}
		iter = std::find_if_not(iter, end, isNumberType);
		if (iter == sigStartIt) {
			return;
		}
		if ((*iter | ('E' ^ 'e')) == 'e') {
			++iter;
			if (expStart()) {
				return;
			}
		}
		if (*iter != '.')
			return;
		++iter;
	}

	template<typename iterator_type> JSONIFIER_INLINE void skipWs(iterator_type&& iter) {
		while (whitespaceTable[static_cast<uint8_t>(*iter)]) {
			++iter;
		}
	}

	template<typename iterator_type> JSONIFIER_INLINE void skipString(iterator_type&& iter, iterator_type&& end) {
		++iter;
		auto newLength = static_cast<uint64_t>(end - iter);
		skipStringImpl(iter, newLength);
	}

	template<simd_structural_iterator_t iterator_type> JSONIFIER_INLINE void skipToEndOfValue(iterator_type&& iter, iterator_type&& end) {
		uint64_t currentDepth{ 1 };
		while (iter != end && currentDepth > 0) {
			switch (*iter) {
				[[unlikely]] case '[':
				[[unlikely]] case '{': {
					++currentDepth;
					++iter;
					break;
				}
				[[unlikely]] case ']':
				[[unlikely]] case '}': {
					--currentDepth;
					++iter;
					break;
				}
				default: {
					++iter;
					break;
				}
			}
		}
	}

	template<simd_structural_iterator_t iterator_type> JSONIFIER_INLINE void skipToNextValue(iterator_type&& iter, iterator_type&& end);

	template<simd_structural_iterator_t iterator_type> JSONIFIER_INLINE void skipObject(iterator_type&& iter, iterator_type&& end) noexcept {
		++iter;
		if (*iter == '}') {
			++iter;
			return;
		}
		while (true) {
			if (*iter != '"') {
				return;
			}
			++iter;
			++iter;
			skipToNextValue(iter, end);
			if (*iter != ',')
				break;
			++iter;
		}
		if (*iter == '}') {
			++iter;
		}
	}

	template<simd_structural_iterator_t iterator_type> JSONIFIER_INLINE void skipArray(iterator_type&& iter, iterator_type&& end) noexcept {
		++iter;
		if (*iter == ']') {
			++iter;
			return;
		}
		while (true) {
			skipToNextValue(iter, end);
			if (*iter != ',')
				break;
			++iter;
		}
		if (*iter == ']') {
			++iter;
		}
	}

	template<simd_structural_iterator_t iterator_type> JSONIFIER_INLINE void skipToNextValue(iterator_type&& iter, iterator_type&& end) {
		switch (*iter) {
			case '{': {
				skipObject(iter, end);
				break;
			}
			case '[': {
				skipArray(iter, end);
				break;
			}
			case '\0': {
				break;
			}
			default: {
				++iter;
			}
		}
	}

	template<typename iterator_type> JSONIFIER_INLINE void skipToEndOfValue(iterator_type&& iter, iterator_type&& end) {
		uint64_t currentDepth{ 1 };
		auto skipToEnd = [&]() {
			while (iter != end && currentDepth > 0) {
				switch (*iter) {
					[[unlikely]] case '[':
					[[unlikely]] case '{': {
						++currentDepth;
						++iter;
						break;
					}
					[[unlikely]] case ']':
					[[unlikely]] case '}': {
						--currentDepth;
						++iter;
						break;
					}
					case '"': {
						skipString(iter, end);
						break;
					}
					default: {
						++iter;
						break;
					}
				}
			}
		};
		switch (*iter) {
			[[unlikely]] case '[':
			[[unlikely]] case '{': {
				++iter;
				skipToEnd();
				break;
			}
			case '"': {
				skipString(iter, end);
				break;
			}
			case ':': {
				++iter;
				skipToEndOfValue(iter, end);
				break;
			}
			case 't': {
				iter += 4;
				break;
			}
			case 'f': {
				iter += 5;
				break;
			}
			case 'n': {
				iter += 4;
				break;
			}
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': {
				skipNumber(iter, end);
				break;
			}
				[[likely]] default : {
					++iter;
					break;
				}
		}
	}

	template<typename iterator_type> JSONIFIER_INLINE void skipToNextValue(iterator_type&& iter, iterator_type&& end);

	template<typename iterator_type> JSONIFIER_INLINE void skipObject(iterator_type&& iter, iterator_type&& end) noexcept {
		++iter;
		if (*iter == '}') {
			++iter;
			return;
		}
		while (true) {
			if (*iter != '"') {
				return;
			}
			skipString(iter, end);
			++iter;
			skipToNextValue(iter, end);
			if (*iter != ',')
				break;
			++iter;
		}
		if (*iter == '}') {
			++iter;
		}
	}

	template<typename iterator_type> JSONIFIER_INLINE void skipArray(iterator_type&& iter, iterator_type&& end) noexcept {
		++iter;
		if (*iter == ']') {
			++iter;
			return;
		}
		while (true) {
			skipToNextValue(iter, end);
			if (*iter != ',')
				break;
			++iter;
		}
		if (*iter == ']') {
			++iter;
		}
	}

	template<typename iterator_type> JSONIFIER_INLINE void skipToNextValue(iterator_type&& iter, iterator_type&& end) {
		switch (*iter) {
			case '{': {
				skipObject(iter, end);
				break;
			}
			case '[': {
				skipArray(iter, end);
				break;
			}
			case '"': {
				skipString(iter, end);
				break;
			}
			case 'n': {
				iter += 4;
				break;
			}
			case 'f': {
				iter += 5;
				break;
			}
			case 't': {
				iter += 4;
				break;
			}
			case '\0': {
				break;
			}
			default: {
				skipNumber(iter, end);
			}
		}
	}

	template<char startChar, char endChar, typename iterator_type> JSONIFIER_INLINE uint64_t countValueElements(iterator_type iter, iterator_type end) {
		auto newValue = *iter;
		if (newValue == ']' || newValue == '}') [[unlikely]] {
			return 0;
		}
		uint64_t currentCount{ 1 };
		while (iter != end) {
			switch (*iter) {
				[[unlikely]] case ',': {
					++currentCount;
					++iter;
					break;
				}
				[[unlikely]] case '{':
				[[unlikely]] case '[': {
					skipToEndOfValue(iter, end);
					break;
				}
				[[unlikely]] case endChar: { return currentCount; }
				[[likely]] case '"': {
					skipString(iter, end);
					break;
				}
				[[unlikely]] case '\\': {
					++iter;
					++iter;
					break;
				}
				[[unlikely]] case 't': {
					iter += 4;
					break;
				}
				[[unlikely]] case 'f': {
					iter += 5;
					break;
				}
				[[unlikely]] case 'n': {
					iter += 4;
					break;
				}
				[[unlikely]] case ':': {
					++iter;
					break;
				}
				[[unlikely]] case '0':
				[[unlikely]] case '1':
				[[unlikely]] case '2':
				[[unlikely]] case '3':
				[[unlikely]] case '4':
				[[unlikely]] case '5':
				[[unlikely]] case '6':
				[[unlikely]] case '7':
				[[unlikely]] case '8':
				[[unlikely]] case '9':
				[[unlikely]] case '-': {
					skipNumber(iter, end);
					break;
				}
					[[likely]] default : {
						++iter;
						break;
					}
			}
		}
		return currentCount;
	}

	struct key_stats_t {
		uint32_t minLength{ (std::numeric_limits<uint32_t>::max)() };
		uint32_t lengthRange{};
		uint32_t maxLength{};
	};

	template<size_t index, typename value_type> constexpr auto keyName = [] {
		using V = std::decay_t<value_type>;
		return std::get<0>(std::get<index>(jsonifier::concepts::core_v<V>));
	}();

	template<uint64_t index, uint64_t maxIndex, typename value_type> constexpr decltype(auto) getKeyStats(key_stats_t keyStats = key_stats_t{}) {
		if constexpr (index < maxIndex) {
			constexpr jsonifier::string_view key = keyName<index, value_type>;
			const auto size{ key.size() };
			if (size < keyStats.minLength) {
				keyStats.minLength = size;
			}
			if (size > keyStats.maxLength) {
				keyStats.maxLength = size;
			}
			return getKeyStats<index + 1, maxIndex, value_type>(keyStats);
		} else {
			if constexpr (maxIndex > 0) {
				keyStats.lengthRange = keyStats.maxLength - keyStats.minLength;
			}
			return keyStats;
		}
	}

	template<const auto& options, typename value_type, typename iterator_type> JSONIFIER_INLINE jsonifier::string_view parseKey(iterator_type&& iter, iterator_type&& end) {
		if (*iter != '"') [[unlikely]] {
			static constexpr auto sourceLocation{ std::source_location::current() };
			options.parserPtr->getErrors().emplace_back(constructError<sourceLocation, error_classes::Parsing, parse_errors::Missing_String_Start>(iter - options.rootIter,
				end - options.rootIter, options.rootIter));
			return {};
		} else {
			++iter;
		}

		static constexpr auto N{ std::tuple_size_v<jsonifier::concepts::core_t<value_type>> };

		const auto start{ iter };

		static constexpr auto stats{ getKeyStats<0, N, value_type>() };

		if constexpr (stats.lengthRange < 24) {
			if ((iter + stats.maxLength) < end) [[likely]] {
				static constexpr auto lengthRange{ stats.lengthRange };

				iter += stats.minLength;

				if constexpr (lengthRange == 0) {
					++iter;
					return { start, stats.minLength };
				} else if constexpr (lengthRange == 1) {
					if (*iter != '"') {
						++iter;
					}
					jsonifier::string_view key{ start, size_t(iter - start) };
					++iter;
					return key;
				}
			}
		}
		memchar<'"'>(iter, (end - iter));
		if (iter) {
			auto newPtr = iter;
			++iter;
			return jsonifier::string_view{ start, size_t(newPtr - start) };
		} else {
			iter = start;
			++iter;
			return {};
		}
	}

	template<const auto& options, typename value_type, simd_structural_iterator_t iterator_type>
	JSONIFIER_INLINE jsonifier::string_view parseKey(iterator_type&& iter, iterator_type&& end) {
		auto start{ iter.operator->() };

		if (*iter != '"') [[unlikely]] {
			static constexpr auto sourceLocation{ std::source_location::current() };
			options.parserPtr->getErrors().emplace_back(constructError<sourceLocation, error_classes::Parsing, parse_errors::Missing_String_Start>(
				iter.getCurrentStringIndex(), iter.getStringLength(), options.rootIter));
			return {};
		} else {
			++iter;
		}

		static constexpr auto N{ std::tuple_size_v<jsonifier::concepts::core_t<value_type>> };

		return jsonifier::string_view{ start + 1, static_cast<uint64_t>(iter.operator->() - (start + 2)) };
	}

}