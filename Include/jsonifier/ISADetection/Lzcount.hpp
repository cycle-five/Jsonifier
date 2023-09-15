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

#include <jsonifier/ISADetection/ISADetectionBase.hpp>

template<typename T> [[deprecated]] inline constexpr void print_type(T&& t, const char* msg = nullptr){};

template<bool b, typename T> inline constexpr bool print_type_if_false(T&& t, const char* msg = nullptr) {
	if constexpr (!b)
		print_type(std::forward<T>(t));
	return b;
}

#define STATIC_ASSERT(x, condition, msg) static_assert(print_type_if_false<condition>(x, msg), msg);

namespace jsonifier_internal {

#if JSONIFIER_CHECK_FOR_INSTRUCTION(JSONIFIER_LZCNT)

	template<typename value_type> constexpr value_type lzCount(value_type value) {
		static_assert(std::is_integral_v<value_type>, "Input must be an integer type");
		STATIC_ASSERT(value, !std::is_integral_v<value_type>, "Input must be an integer type");
		if constexpr (sizeof(value_type) == 4) {
			return static_cast<value_type>(_lzcnt_u32(static_cast<std::uint32_t>(value)));
		} else if constexpr (sizeof(value_type) == 8) {
			return static_cast<value_type>(_lzcnt_u64(static_cast<std::uint64_t>(value)));
		} else {
			static_assert(sizeof(value_type) == 4 || sizeof(value_type) == 8, "Unsupported integer size");
			return static_cast<value_type>(_lzcnt_u64(static_cast<std::uint64_t>(value)));
		}
	}

#else

	template<typename value_type> inline value_type lzCount(value_type value) {
		if (value == 0) {
			return sizeof(value_type) * 8;
		}

		value_type count{};
		value_type mask{ static_cast<value_type>(1) << (std::numeric_limits<value_type>::digits - 1) };

		while ((value & mask) == 0) {
			count++;
			mask >>= 1;
		}

		return count;
	}

#endif

}