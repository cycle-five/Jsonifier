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
/// https://github.com/RealTimeChris/Jsonifier
/// Feb 20, 2023
#pragma once

#include <iterator>

namespace JsonifierInternal {

	template<typename ValueType, size_t N> class RawVector {
	  public:
		using value_type = ValueType;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using iterator = pointer;
		using const_iterator = const_pointer;
		using size_type = size_t;
		using difference_type = std::ptrdiff_t;

		inline constexpr RawVector() noexcept = default;
		inline constexpr RawVector& operator=(RawVector&&) noexcept = default;
		inline constexpr RawVector(RawVector&&) noexcept = default;
		inline constexpr RawVector& operator=(const RawVector&) noexcept = default;
		inline constexpr RawVector(const RawVector&) noexcept = default;

		inline constexpr RawVector(size_type count, const auto& value) : dsize(count) {
			for (size_t x = 0; x < N; ++x)
				data[x] = value;
		}

		inline constexpr iterator begin() noexcept {
			return data;
		}

		inline constexpr iterator end() noexcept {
			return data + dsize;
		}

		inline constexpr size_type size() const noexcept {
			return dsize;
		}

		inline constexpr reference operator[](size_t index) {
			return data[index];
		}

		inline constexpr const_reference operator[](size_t index) const noexcept {
			return data[index];
		}

		inline constexpr reference back() {
			return data[dsize - 1];
		}

		inline constexpr const_reference back() const noexcept {
			return data[dsize - 1];
		}

		inline constexpr void push_back(const ValueType& a) {
			data[dsize++] = a;
		}

		inline constexpr void push_back(ValueType&& a) {
			data[dsize++] = std::move(a);
		}

		inline constexpr void pop_back() {
			--dsize;
		}

		inline constexpr void clear() {
			dsize = 0;
		}

	  protected:
		ValueType data[N] = {};
		size_t dsize = 0;
	};

}