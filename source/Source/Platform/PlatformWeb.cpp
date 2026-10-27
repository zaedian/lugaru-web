/*
Copyright (C) 2003, 2010 - Wolfire Games
Copyright (C) 2010-2017 - Lugaru contributors (see AUTHORS file)

This file is part of Lugaru, maintained as part of the Loupgarenne fork.
See README and AUTHORS for project details.

Lugaru is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Lugaru is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef __EMSCRIPTEN__

#include "Platform/Platform.hpp"

#include <assert.h>
#include <emscripten.h>

// Type definitions for timing
typedef long long __int64;
typedef __int64 LARGE_INTEGER;

/**
 * QueryPerformanceFrequency equivalent for Emscripten
 * Returns the frequency in Hz (1000 for milliseconds)
 */
static int QueryPerformanceFrequency(LARGE_INTEGER* liptr)
{
    assert(sizeof(__int64) == 8);
    assert(sizeof(LARGE_INTEGER) == 8);
    // emscripten_get_now() returns milliseconds, so frequency is 1000 Hz
    *liptr = 1000;
    return (1);
}

/**
 * QueryPerformanceCounter equivalent for Emscripten
 * Uses emscripten_get_now() which returns high-resolution time in milliseconds
 * since the page loaded (or the runtime started)
 */
static int QueryPerformanceCounter(LARGE_INTEGER* liptr)
{
    // emscripten_get_now() returns milliseconds as a double
    double now = emscripten_get_now();
    *liptr = (LARGE_INTEGER)now;
    return (1);
}

/**
 * AppTime class stores the base counter value at application startup
 * This allows us to return time relative to app start rather than page load
 */
class AppTime
{
public:
    AppTime()
    {
        counterRate = 1;
        baseCounter = 0;
        QueryPerformanceFrequency((LARGE_INTEGER*)&counterRate);
        QueryPerformanceCounter((LARGE_INTEGER*)&baseCounter);
    }
    __int64 counterRate; // LARGE_INTEGER type has no math functions so use int64
    __int64 baseCounter;
};

// Global instance initialized at program start
static AppTime g_appTime;

/**
 * Returns time since the app started, not page load
 * Returns an AbsoluteTime structure containing 64-bit counter value
 */
AbsoluteTime UpTime()
{
    __int64 counter;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);

    // Subtract the base counter to get time since app start
    counter -= g_appTime.baseCounter;

    AbsoluteTime time;
    time.lo = (unsigned long)counter;
    time.hi = (unsigned long)(counter >> 32);
    return time;
}

/**
 * Calculates the duration between two AbsoluteTime values
 * Returns the result in milliseconds, or microseconds if less than 1ms
 *
 * @param a The later time
 * @param b The earlier time
 * @return Duration in milliseconds (positive) or microseconds (negative)
 */
Duration AbsoluteDeltaToDuration(AbsoluteTime& a, AbsoluteTime& b)
{
    // Reconstruct 64-bit values from hi/lo parts
    __int64 value = a.hi;
    value <<= 32;
    value |= a.lo;
    __int64 value2 = b.hi;
    value2 <<= 32;
    value2 |= b.lo;
    value -= value2;

    // If time difference is zero or negative, return immediately
    if (value <= 0) {
        return durationImmediate;
    }

    // Calculate fractional part for sub-millisecond precision
    __int64 frac = value % g_appTime.counterRate;
    value /= g_appTime.counterRate;

    Duration time;

    // If less than 1 millisecond, return as microseconds (negative value)
    if (value == 0) {
        frac *= -1000000;
        frac /= g_appTime.counterRate;
        time = (Duration)frac;
    } else {
        // Otherwise return as milliseconds
        frac *= 1000;
        frac /= g_appTime.counterRate;
        value *= 1000;
        value += frac;
        time = (Duration)value;
    }

    return time;
}

#endif // __EMSCRIPTEN__
