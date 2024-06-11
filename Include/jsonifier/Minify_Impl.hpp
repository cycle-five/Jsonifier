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
#include <jsonifier/Minifier.hpp>
#include <jsonifier/Simd.hpp>

namespace jsonifier_internal {

	JSONIFIER_INLINE uint64_t findFirstNonWhitespaceDistance(const char* previousPtr, const char* iter) {
		uint64_t currentDistance   = 0;
		const uint64_t maxDistance = static_cast<uint64_t>(iter - previousPtr);
		if (maxDistance < 16) {
			while (currentDistance < maxDistance && !whitespaceTable[static_cast<uint8_t>(previousPtr[currentDistance])]) {
				++currentDistance;
			}

			return currentDistance;
		}
#if JSONIFIER_CHECK_FOR_AVX(JSONIFIER_AVX512)
		{
			using simd_type						 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 2>::type::type;
			using integer_type					 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 2>::type::integer_type;
			static constexpr uint64_t vectorSize = jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 2>::type::bytesProcessed;
			integer_type whitespaceMask;
			simd_type chunk;
			while (currentDistance + vectorSize < maxDistance) {
				chunk				= simd_internal::gatherValuesU<simd_type>(previousPtr + currentDistance);
				auto whitespaceMask = simd_internal::opCmpEq(simd_internal::opShuffle(simd_internal::whitespaceTable<simd_type>, chunk), chunk);
				if (whitespaceMask != 0) {
					currentDistance += simd_internal::tzcnt(whitespaceMask);
					return currentDistance;
				}

				currentDistance += vectorSize;
			}
		}
#endif
#if JSONIFIER_CHECK_FOR_AVX(JSONIFIER_AVX2)
		{
			using simd_type						 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 1>::type::type;
			using integer_type					 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 1>::type::integer_type;
			static constexpr uint64_t vectorSize = jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 1>::type::bytesProcessed;
			integer_type whitespaceMask;
			simd_type chunk;
			while (currentDistance + vectorSize < maxDistance) {
				chunk				= simd_internal::gatherValuesU<simd_type>(previousPtr + currentDistance);
				auto whitespaceMask = simd_internal::opCmpEq(simd_internal::opShuffle(simd_internal::whitespaceTable<simd_type>, chunk), chunk);
				if (whitespaceMask != 0) {
					currentDistance += simd_internal::tzcnt(whitespaceMask);
					return currentDistance;
				}

				currentDistance += vectorSize;
			}
		}
#endif
#if JSONIFIER_CHECK_FOR_AVX(JSONIFIER_AVX) || JSONIFIER_CHECK_FOR_INSTRUCTION(JSONIFIER_NEON)
		{
			using simd_type						 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 0>::type::type;
			using integer_type					 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 0>::type::integer_type;
			static constexpr uint64_t vectorSize = jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 0>::type::bytesProcessed;
			integer_type whitespaceMask;
			simd_type chunk;
			while (currentDistance + vectorSize < maxDistance) {
				chunk				= simd_internal::gatherValuesU<simd_type>(previousPtr + currentDistance);
				auto whitespaceMask = simd_internal::opCmpEq(simd_internal::opShuffle(simd_internal::whitespaceTable<simd_type>, chunk), chunk);
				if (whitespaceMask != 0) {
					currentDistance += simd_internal::tzcnt(whitespaceMask);
					return currentDistance;
				}

				currentDistance += vectorSize;
			}
		}
#endif
		while (currentDistance < maxDistance && !whitespaceTable[static_cast<uint8_t>(previousPtr[currentDistance])]) {
			++currentDistance;
		}

		return currentDistance;
	}

	JSONIFIER_INLINE uint64_t findLastNonWhitespaceDistance(const char* previousPtr, const char* iter) {
		uint64_t currentDistance = static_cast<uint64_t>(iter - previousPtr);
		if (currentDistance < 16) {
			while (currentDistance > 0 && whitespaceTable[static_cast<uint8_t>(previousPtr[currentDistance])]) {
				--currentDistance;
			};

			return currentDistance - 1;
		}
#if JSONIFIER_CHECK_FOR_AVX(JSONIFIER_AVX512)
		{
			using simd_type						 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 2>::type::type;
			using integer_type					 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 2>::type::integer_type;
			static constexpr uint64_t vectorSize = jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 2>::type::bytesProcessed;
			integer_type whitespaceMask;
			simd_type chunk;
			while (currentDistance >= vectorSize) {
				chunk				= simd_internal::gatherValuesU<simd_type>((previousPtr - vectorSize) + currentDistance);
				auto whitespaceMask = simd_internal::opCmpEq(simd_internal::opShuffle(simd_internal::whitespaceTable<simd_type>, chunk), chunk);
				if (whitespaceMask != 0xFFFFFFFFFFFFFFFF) {
					currentDistance -= simd_internal::lzcnt(static_cast<uint32_t>(~whitespaceMask));
					return currentDistance - 1;
				}
				currentDistance -= vectorSize;
			}
		}
#endif
#if JSONIFIER_CHECK_FOR_AVX(JSONIFIER_AVX2)
		{
			using simd_type						 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 1>::type::type;
			using integer_type					 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 1>::type::integer_type;
			static constexpr uint64_t vectorSize = jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 1>::type::bytesProcessed;
			integer_type whitespaceMask;
			simd_type chunk;
			while (currentDistance >= vectorSize) {
				chunk				= simd_internal::gatherValuesU<simd_type>((previousPtr - vectorSize) + currentDistance);
				auto whitespaceMask = simd_internal::opCmpEq(simd_internal::opShuffle(simd_internal::whitespaceTable<simd_type>, chunk), chunk);
				if (whitespaceMask != 0xFFFFFFFF) {
					currentDistance -= simd_internal::lzcnt(static_cast<uint32_t>(~whitespaceMask));
					return currentDistance - 1;
				}
				currentDistance -= vectorSize;
			}
		}
#endif
#if JSONIFIER_CHECK_FOR_AVX(JSONIFIER_AVX) || JSONIFIER_CHECK_FOR_INSTRUCTION(JSONIFIER_NEON)
		{
			using simd_type						 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 0>::type::type;
			using integer_type					 = typename jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 0>::type::integer_type;
			static constexpr uint64_t vectorSize = jsonifier::concepts::get_type_at_index<simd_internal::avx_list, 0>::type::bytesProcessed;
			integer_type whitespaceMask;
			simd_type chunk;
			while (currentDistance >= vectorSize) {
				chunk				= simd_internal::gatherValuesU<simd_type>((previousPtr - vectorSize) + currentDistance);
				auto whitespaceMask = simd_internal::opCmpEq(simd_internal::opShuffle(simd_internal::whitespaceTable<simd_type>, chunk), chunk);
				if (whitespaceMask != 0xFFFF) {
					currentDistance -= simd_internal::lzcnt(static_cast<uint16_t>(~whitespaceMask));
					return currentDistance - 1;
				}
				currentDistance -= vectorSize;
			}
		}
#endif
		while (currentDistance > 0 && whitespaceTable[static_cast<uint8_t>(previousPtr[currentDistance])]) {
			--currentDistance;
		};

		return currentDistance - 1;
	}

	template<typename derived_type> struct minify_impl {
		template<const minify_options_internal<derived_type>& options, typename iterator_type, jsonifier::concepts::buffer_like buffer_type,
			jsonifier::concepts::uint64_type index_type>
		JSONIFIER_INLINE static void impl(iterator_type&& iter, buffer_type&& out, index_type&& index) noexcept {
			auto previousPtr = iter.operator->();
			int64_t currentDistance{};

			++iter;
			while (iter) {
				switch (asciiClassesMap[static_cast<uint8_t>(*previousPtr)]) {
					[[likely]] case json_structural_type::String: {
						if (*(previousPtr + 1) == '"') {
							currentDistance = 1;
						} else {
							currentDistance = static_cast<int64_t>(findLastNonWhitespaceDistance(previousPtr, iter.operator->()));
						}
						if (currentDistance > 0) [[likely]] {
							writeCharactersUnchecked(out, previousPtr, currentDistance + 1, index);
						} else {
							static constexpr auto sourceLocation{ std::source_location::current() };
							options.minifierPtr->getErrors().emplace_back(constructError<sourceLocation, error_classes::Minifying, minify_errors::Invalid_String_Length>(
								iter.getCurrentStringIndex(), iter.getStringLength(), iter.getRootPtr()));
							return;
						}
						break;
					}
					[[unlikely]] case json_structural_type::Comma:
						writeCharacterUnchecked<','>(out, index);
						break;
					[[likely]] case json_structural_type::Number: {
						currentDistance = static_cast<int64_t>(findFirstNonWhitespaceDistance(previousPtr, iter.operator->()));
						if (currentDistance > 0) [[likely]] {
							writeCharactersUnchecked(out, previousPtr, static_cast<uint64_t>(currentDistance), index);
						} else {
							static constexpr auto sourceLocation{ std::source_location::current() };
							options.minifierPtr->getErrors().emplace_back(constructError<sourceLocation, error_classes::Minifying, minify_errors::Invalid_Number_Value>(
								iter.getCurrentStringIndex(), iter.getStringLength(), iter.getRootPtr()));
							return;
						}
						break;
					}
					[[unlikely]] case json_structural_type::Colon:
						writeCharacterUnchecked<':'>(out, index);
						break;
					[[unlikely]] case json_structural_type::Array_Start:
						writeCharacterUnchecked<'['>(out, index);
						break;
					[[unlikely]] case json_structural_type::Array_End:
						writeCharacterUnchecked<']'>(out, index);
						break;
					[[unlikely]] case json_structural_type::Null: {
						writeCharactersUnchecked(out, nullString.data(), nullString.size(), index);
						break;
					}
					[[unlikely]] case json_structural_type::Bool: {
						if (*previousPtr == 't') {
							writeCharactersUnchecked(out, trueString.data(), trueString.size(), index);
							break;
						} else {
							writeCharactersUnchecked(out, falseString.data(), falseString.size(), index);
							break;
						}
					}
					[[unlikely]] case json_structural_type::Object_Start:
						writeCharacterUnchecked<'{'>(out, index);
						break;
					[[unlikely]] case json_structural_type::Object_End:
						writeCharacterUnchecked<'}'>(out, index);
						break;
					[[unlikely]] case json_structural_type::Unset:
					[[unlikely]] case json_structural_type::Error:
					[[unlikely]] case json_structural_type::Type_Count:
						[[fallthrough]];
					[[unlikely]] default: {
						static constexpr auto sourceLocation{ std::source_location::current() };
						options.minifierPtr->getErrors().emplace_back(constructError<sourceLocation, error_classes::Minifying, minify_errors::Incorrect_Structural_Index>(
							iter.getCurrentStringIndex(), iter.getStringLength(), iter.getRootPtr()));
						return;
					}
				}
				previousPtr = iter.operator->();
				++iter;
			}
			writeCharacterUnchecked(*previousPtr, out, index);
		}
	};

}// namespace jsonifier_internal