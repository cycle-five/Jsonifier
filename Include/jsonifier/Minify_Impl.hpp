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

	template<typename derived_type> struct minify_impl {
		template<const minify_options_internal<derived_type>& options, jsonifier::concepts::string_t string_type, typename iterator_type>
		JSONIFIER_INLINE static void impl(iterator_type&& iter, iterator_type&& end, string_type& out, uint64_t& index) noexcept {
			auto stringStart = iter.getRootPtr();
			auto stringEnd	 = stringStart;
			while (iter != end && stringStart && stringEnd) {
				stringEnd = iter.operator string_view_ptr();
				std::copy(stringStart, stringEnd, out.data() + index);
				index += stringEnd - stringStart;
				stringStart = iter.operator string_view_ptr() + 1;
				++iter;
			}
			if (stringEnd) {
				while (whitespaceTable[*stringEnd]) {
					++stringEnd;
				}
				out[index] = *stringEnd;
				++index;
			}
		}
	};

}// namespace jsonifier_internal