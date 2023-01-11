#pragma once

#ifdef DEBUG
#ifdef _WINDOWS
#define breakpoint() __debugbreak()
#elif defined(__linux__)
#include <signal.h>
		#define DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
#endif
#else
#define breakpoint()
#endif

#ifdef DEBUG
#define rect_assert(x, msg) if (!(x)) { \
    throw std::runtime_error(msg); \
}
#else
#define rect_assert(x, msg)
#endif
