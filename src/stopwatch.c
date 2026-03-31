#include "stopwatch.h"

#include <string.h>
#include <time.h>

// UNIX headers
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#include <unistd.h> // Needed for _POSIX_TIMERS definition & usleep()
#endif

// Windows headers
#if defined(_WIN32)
#include <windows.h>
#endif

int sw_gettime(struct SwTimestamp *stamp)
{
    memset(stamp, 0, sizeof(struct SwTimestamp));

#if defined(_WIN32)
    // Microsoft Windows (32-bit or 64-bit)

    LARGE_INTEGER tick;
    int check = QueryPerformanceCounter(&tick);
    if (check == 0)
    {
        return -1;
    } // QueryPerformanceCounter() returns 0 on failure

    stamp->val.tick_win = tick;
    stamp->val_member = TICK_WIN;

#elif defined(__APPLE__) && defined(__MACH__)
    // Apple OSX and iOS (Darwin)

    unsigned long long tick = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    if (tick == 0)
    {
        return -1;
    } // clock_gettime_nsec_np() returns 0 on failure

    stamp->val.tick_apple = tick;
    stamp->val_member = TICK_APPLE;

#elif defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
    // Available on some POSIX systems (preferable to gettimeofday() below)

    struct timespec tick;
    int check = clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    if (check == -1)
    {
        return -1;
    } // clock_gettime() returns -1 on failure

    stamp->val.tick_spec = tick;
    stamp->val_member = TICK_SPEC;

#elif defined(__unix__)
    // Should work almost anywhere on Unix, but is not monotonic

    struct timeval tick;
    int check = gettimeofday(&tick, NULL);
    if (check != 0)
    {
        return -1;
    } // gettimeofday() returns non-zero on failure

    stamp->val.tick_val = tick;
    stamp->val_member = TICK_VAL;

#else

    return -1; // Give up

#endif

    return 0;
}

int sw_timediff_usec(struct SwTimestamp end, struct SwTimestamp begin, unsigned long long *diff)
{
    *diff = 0;

    // Ensure unions have the same member set
    if (end.val_member != begin.val_member)
    {
        return -1;
    }

    switch (end.val_member)
    {
    case TICK_WIN: {
#if defined(_WIN32)
        LARGE_INTEGER frequency; // ticks per second
        int check = QueryPerformanceFrequency(&frequency);
        if (check == 0)
        {
            return -1; // QueryPerformanceFrequency failed
        }
        *diff = (unsigned long long)(end.val.tick_win.QuadPart - begin.val.tick_win.QuadPart) * 1000000ULL / frequency.QuadPart;
#else
        return -1; // Unsupported on this platform
#endif
        break;
    }

    case TICK_APPLE: {
#if defined(__APPLE__) && defined(__MACH__)
        *diff = (end.val.tick_apple - begin.val.tick_apple) / 1000ULL; // ns to us
#else
        return -1; // Unsupported on this platform
#endif
        break;
    }

    case TICK_SPEC: {
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
        long sec_diff = end.val.tick_spec.tv_sec - begin.val.tick_spec.tv_sec;
        long nsec_diff = end.val.tick_spec.tv_nsec - begin.val.tick_spec.tv_nsec;

        if (nsec_diff < 0)
        {
            sec_diff -= 1;
            nsec_diff += 1000000000L; // Adjust for nanosecond underflow
        }

        *diff = (unsigned long long)sec_diff * 1000000ULL + (unsigned long long)nsec_diff / 1000ULL;
#else
        return -1; // Unsupported on this platform
#endif
        break;
    }

    case TICK_VAL: {
#if defined(__unix__)
        *diff = (unsigned long long)(end.val.tick_val.tv_sec - begin.val.tick_val.tv_sec) * 1000000ULL;
        *diff += (unsigned long long)(end.val.tick_val.tv_usec - begin.val.tick_val.tv_usec);
#else
        return -1; // Unsupported on this platform
#endif
        break;
    }

    default:
        return -1; // Unsupported timestamp type
    }

    return 0;
}

int sw_sleep(unsigned long long microseconds)
{
#if defined(_WIN32)
    // Microsoft Windows (32-bit or 64-bit)

    unsigned long milliseconds = (unsigned long)((double)microseconds / 1.0E3);
    Sleep(milliseconds);

#else
    // Everything else. usleep() should be fairly portable

    int check = usleep(microseconds);
    if (check == -1)
    {
        return -1;
    } // usleep() returns -1 on failure

#endif

    return 0;
}
