#pragma once
#include <cstdint>

/// Define to enable Vulkan API usage (eventually make this configurable by build system)
#define USE_VULKAN true
#define USE_DRIECTX false
#define USE_OPENGL false

#if _WIN32 || _WIN64
    #define DLL_EXPORT __declspec(dllexport)
    #define PLATFORM_WINDOWS true
    #define PLATFORM_UNIX false
    #define PLATFORM_MACOSX false
#elif __unix__ || __linux__
    #define DLL_EXPORT __attribute__((visibility("default")))
    #define PLATFORM_WINDOWS false
    #define PLATFORM_UNIX true
    #define PLATFORM_MACOSX false
#else
    #error "Unsupported platform"
#endif

typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef float float32;
typedef double float64;
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;

static_assert(1 == sizeof(uint8));
static_assert(2 == sizeof(uint16));
static_assert(4 == sizeof(uint32));
static_assert(8 == sizeof(uint64));
static_assert(4 == sizeof(float32));
static_assert(8 == sizeof(float64));