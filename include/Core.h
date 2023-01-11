#pragma once

#ifdef DEBUG

#ifdef _WIN32
#define breakpoint() __debugbreak()
#endif

#ifdef _WIN64
#define breakpoint() __debugbreak()
#endif

#ifdef __linux__
#include <signal.h>
#define breakpoint() raise(SIGTRAP)
#endif // __linux__

#else
#define breakpoint()

#endif // DEBUG

#ifdef DEBUG
#define rect_assert(x, msg, ...) if (!(x)) { \
    printf(msg, __VA_ARGS__);                                         \
    breakpoint(); \
}
#else
#define rect_assert(x, msg)
#endif // DEBUG
