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

#include <jsonifier/TypeEntities.hpp>
#include <jsonifier/StringView.hpp>
#include <jsonifier/Tuple.hpp>
#include <jsonifier/Pair.hpp>
#include <functional>
#include <optional>
#include <atomic>

namespace jsonifier_internal {

	template<typename... Args> struct false_t : std::false_type {};

	struct random_core_type;

	template<> struct false_t<random_core_type> : std::true_type {};

	template<typename... Args> jsonifier_constexpr bool falseV = false_t<Args...>::value;

	template<typename = void, size_t... Indices> jsonifier_constexpr auto indexer(std::index_sequence<Indices...>) {
		return [](auto&& f) -> decltype(auto) {
			return decltype(f)(f)(std::integral_constant<size_t, Indices>{}...);
		};
	}

	template<size_t n> jsonifier_constexpr auto indexer() {
		return indexer(std::make_index_sequence<n>{});
	}

	template<size_t n, typename Func> jsonifier_constexpr auto forEach(Func&& f) {
		return indexer<n>()([&](auto&&... i) {
			(std::forward<jsonifier::concepts::unwrap<Func>>(f)(i), ...);
		});
	}

	template<ctime_array newArr> struct make_static {
		static jsonifier_constexpr auto value = newArr;
	};

	template<const jsonifier::string_view&... strings> jsonifier_constexpr jsonifier::string_view join() {
		jsonifier_constexpr auto joinedArr = []() {
			jsonifier_constexpr size_t len = (strings.size() + ... + 0);
			ctime_array<char, len + 1> arr{};
			auto append = [i = 0, &arr](const auto& s) mutable {
				for (auto c: s)
					arr[static_cast<uint64_t>(i++)] = c;
			};
			(append(strings), ...);
			arr[len] = 0;
			return arr;
		}();
		auto& staticArr = make_static<joinedArr>::value;
		return { staticArr.data(), staticArr.maxSize() - 1 };
	}

	template<const jsonifier::string_view&... strings> jsonifier_constexpr auto JoinV = join<strings...>();

	jsonifier_inline decltype(auto) getMember(auto&& value, auto& member_ptr) {
		using value_type = jsonifier::concepts::unwrap<decltype(member_ptr)>;
		if jsonifier_constexpr (std::is_member_object_pointer_v<value_type>) {
			return value.*member_ptr;
		} else if jsonifier_constexpr (std::is_member_function_pointer_v<value_type>) {
			return member_ptr;
		} else if jsonifier_constexpr (std::invocable<value_type, decltype(value)>) {
			return std::invoke(member_ptr, value);
		} else if jsonifier_constexpr (std::is_pointer_v<value_type>) {
			return *member_ptr;
		} else {
			return member_ptr;
		}
	}

	template<typename value_type, typename mptr_t> using member_t = decltype(getMember(std::declval<value_type>(), std::declval<jsonifier::concepts::unwrap<mptr_t>&>()));

	template<jsonifier::concepts::time_type value_type> class stop_watch {
	  public:
		using hr_clock = std::chrono::high_resolution_clock;

		jsonifier_inline stop_watch(uint64_t newTime) {
			totalNumberOfTimeUnits.store(value_type{ newTime }, std::memory_order_release);
		}

		jsonifier_inline stop_watch(value_type newTime) {
			totalNumberOfTimeUnits.store(newTime, std::memory_order_release);
		}

		jsonifier_inline stop_watch& operator=(stop_watch&& other) {
			totalNumberOfTimeUnits.store(other.totalNumberOfTimeUnits.load(std::memory_order_acquire), std::memory_order_release);
			startTimeInTimeUnits.store(other.startTimeInTimeUnits.load(std::memory_order_acquire), std::memory_order_release);
			return *this;
		}

		jsonifier_inline stop_watch(stop_watch&& other) {
			*this = std::move(other);
		}

		jsonifier_inline stop_watch& operator=(const stop_watch& other) {
			totalNumberOfTimeUnits.store(other.totalNumberOfTimeUnits.load(std::memory_order_acquire), std::memory_order_release);
			startTimeInTimeUnits.store(other.startTimeInTimeUnits.load(std::memory_order_acquire), std::memory_order_release);
			return *this;
		}

		jsonifier_inline stop_watch(const stop_watch& other) {
			*this = other;
		}

		jsonifier_inline bool hasTimeElapsed() {
			if (std::chrono::duration_cast<value_type>(hr_clock::now().time_since_epoch()) - startTimeInTimeUnits.load(std::memory_order_acquire) >=
				totalNumberOfTimeUnits.load(std::memory_order_acquire)) {
				return true;
			} else {
				return false;
			}
		}

		jsonifier_inline void reset(value_type newTimeValue = value_type{}) {
			if (newTimeValue != value_type{}) {
				totalNumberOfTimeUnits.store(newTimeValue, std::memory_order_release);
				startTimeInTimeUnits.store(std::chrono::duration_cast<value_type>(hr_clock::now().time_since_epoch()), std::memory_order_release);
			} else {
				startTimeInTimeUnits.store(std::chrono::duration_cast<value_type>(hr_clock::now().time_since_epoch()), std::memory_order_release);
			}
		}

		jsonifier_inline value_type getTotalWaitTime() const {
			return totalNumberOfTimeUnits.load(std::memory_order_acquire);
		}

		jsonifier_inline value_type totalTimeElapsed() {
			return std::chrono::duration_cast<value_type>(hr_clock::now().time_since_epoch()) - startTimeInTimeUnits.load(std::memory_order_acquire);
		}

	  protected:
		std::atomic<value_type> totalNumberOfTimeUnits{};
		std::atomic<value_type> startTimeInTimeUnits{};
	};

	template<jsonifier::concepts::time_type value_type> stop_watch(value_type) -> stop_watch<value_type>;
}// namespace jsonifier_internal

namespace jsonifier {

	jsonifier_constexpr auto createArray(auto&&... args) {
		return array{ jsonifier_internal::copyTuple(args...) };
	}

	template<typename... Args> jsonifier_constexpr auto createObject(Args&&... args) {
		if jsonifier_constexpr (sizeof...(args) == 0) {
			return object{ std::tuple<>{} };
		} else {
			auto newTuple	 = jsonifier_internal::copyTuple(args...);
			using tuple_type = decltype(newTuple);
			return object{ jsonifier_internal::GroupBuilder<tuple_type>::op(std::move(newTuple)) };
		}
	}

}// namespace jsonifier