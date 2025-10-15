#pragma once

#include <cassert>
#include <cstdint>

#include <Windows.h>

inline void waitTil(LARGE_INTEGER until)
{
	// Busy-waits to avoid oversleeping
	LARGE_INTEGER now{};
	do {
		QueryPerformanceCounter(&now);
	} while (now.QuadPart < until.QuadPart);
}

// Adds a number of microseconds to a performance counter value
inline LARGE_INTEGER addMicroseconds(LARGE_INTEGER timeline, int64_t microseconds)
{
	static int64_t s_counts_per_microsecond{};
	if (!s_counts_per_microsecond) {
		LARGE_INTEGER freq{};
		QueryPerformanceFrequency(&freq);
		assert(freq.QuadPart > 1'000'000LL);
		s_counts_per_microsecond = freq.QuadPart / 1'000'000LL;
	}
	return LARGE_INTEGER{ .QuadPart = timeline.QuadPart + (microseconds * s_counts_per_microsecond) };
}

template<typename Func>
inline void startFixedUpdateLoop(int iterations, int64_t period_microseconds, Func&& func)
{
	LARGE_INTEGER timeline{};
	QueryPerformanceCounter(&timeline);
	for (int i = 0; i < iterations; ++i) {

		func(i);

		timeline = addMicroseconds(timeline, period_microseconds);
		waitTil(timeline);

	}
}