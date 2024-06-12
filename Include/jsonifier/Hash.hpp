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

#include <source_location>
#include <unordered_map>
#include <exception>
#include <string>

namespace jsonifier_internal {

	// https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
	// http://www.isthe.com/chongo/tech/comp/fnv/index.html#FNV-param
	static constexpr uint64_t fnv64OffsetBasis{ 0xcbf29ce484222325 };
	static constexpr uint64_t fnv64Prime{ 0x00000100000001B3 };

	struct fnv1a_hash {
		constexpr uint64_t operator()(const char* value, size_t size, uint64_t seed = 0) const {
			uint64_t hash		   = fnv64OffsetBasis * fnv64Prime ^ seed;
			string_view_ptr newPtr = static_cast<string_view_ptr>(value);
			for (uint64_t x = 0; x < size; ++x) {
				hash = (hash ^ static_cast<uint64_t>(static_cast<std::byte>(newPtr[x]))) * fnv64Prime;
			}
			return static_cast<uint64_t>(hash >> 8);
		}
	};
}