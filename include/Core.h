#pragma once

#ifdef DEBUG

#ifdef _WIN32
#define breakpoint() __debugbreak()
#define VULKAN
#endif

#ifdef _WIN64
#define breakpoint() __debugbreak()
#define VULKAN
#endif

#ifdef __linux__
#include <signal.h>
#define breakpoint() raise(SIGTRAP)
#define VULKAN
#endif // __linux__

#else
#define breakpoint()

#endif // DEBUG

#ifdef DEBUG
#include <cstdio>
#define rect_assert(x, msg, ...) if (!(x)) { \
    printf(msg, __VA_ARGS__);                                         \
    breakpoint(); \
}
#else
#define rect_assert(x, msg) if (!(x)) { \
    throw std::runtime_error(msg);                                        \
}
#endif // DEBUG

#ifdef VULKAN
#include <vulkan/vulkan.h>
#endif

#if DEBUG
#define VALIDATION_LAYERS
#endif

// Primitives

#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;