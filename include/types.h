#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

/* Platform Detection */
#if defined(_WIN32) || defined(_WIN64)
    #define ENGINE_PLATFORM_WIN32
    #define ENGINE_PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define ENGINE_PLATFORM_LINUX
    #define ENGINE_PLATFORM_NAME "Linux"
#elif defined(__APPLE__) && defined(__MACH__)
    #define ENGINE_PLATFORM_MACOS
    #define ENGINE_PLATFORM_NAME "macOS"
#else
    #error "Unsupported platform"
#endif

/* Standard includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Boolean type definitions (if stdbool not available) */
#ifndef __cplusplus
    #ifndef bool
        typedef int bool;
        #define true 1
        #define false 0
    #endif
#endif

/* Common integer types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef float    f32;
typedef double   f64;

/* Result codes */
typedef enum {
    ENGINE_SUCCESS = 0,
    ENGINE_ERROR = -1,
    ENGINE_ERROR_INVALID_PARAM = -2,
    ENGINE_ERROR_OUT_OF_MEMORY = -3,
    ENGINE_ERROR_PLATFORM_INIT_FAILED = -4,
    ENGINE_ERROR_WINDOW_CREATION_FAILED = -5,
    ENGINE_ERROR_NOT_INITIALIZED = -6,
} engine_result_t;

/* Common macros */
#define ENGINE_NULL ((void*)0)

#ifndef NULL
    #define NULL ENGINE_NULL
#endif

/* API export/import macros */
#ifdef ENGINE_BUILD_SHARED
    #ifdef ENGINE_PLATFORM_WIN32
        #ifdef ENGINE_EXPORTS
            #define ENGINE_API __declspec(dllexport)
        #else
            #define ENGINE_API __declspec(dllimport)
        #endif
    #else
        #define ENGINE_API __attribute__((visibility("default")))
    #endif
#else
    #define ENGINE_API
#endif

/* Utility macros */
#define ENGINE_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ENGINE_UNUSED(x) ((void)(x))
#define ENGINE_MIN(a, b) ((a) < (b) ? (a) : (b))
#define ENGINE_MAX(a, b) ((a) > (b) ? (a) : (b))
#define ENGINE_CLAMP(x, min, max) (ENGINE_MIN(ENGINE_MAX(x, min), max))

/* Assertions */
#ifdef ENGINE_DEBUG
    #include <assert.h>
    #define ENGINE_ASSERT(expr) assert(expr)
#else
    #define ENGINE_ASSERT(expr) ((void)0)
#endif

/* Logging macros */
#ifdef ENGINE_ENABLE_LOGGING
    #include <stdio.h>
    #define ENGINE_LOG_INFO(fmt, ...) \
        fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)
    #define ENGINE_LOG_WARN(fmt, ...) \
        fprintf(stdout, "[WARN] " fmt "\n", ##__VA_ARGS__)
    #define ENGINE_LOG_ERROR(fmt, ...) \
        fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
    #define ENGINE_LOG_INFO(fmt, ...)
    #define ENGINE_LOG_WARN(fmt, ...)
    #define ENGINE_LOG_ERROR(fmt, ...)
#endif

#endif /* ENGINE_TYPES_H */
