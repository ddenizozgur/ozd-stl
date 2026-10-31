#pragma once

#if !defined(_WIN32)
#error Only Windows is supported.
#endif

#if defined(__clang__)
#define IS_COMPILER_CLANG 1
#if defined(__amd64__) || defined(__amd64) || \
    defined(__x86_64__) || defined(__x86_64)
#define IS_ARCH_X64 1
#elif defined(__aarch64__)
#define IS_ARCH_ARM64 1
#else
#error Architecture not supported.
#endif

#elif defined(_MSC_VER)
#define IS_COMPILER_MSVC 1
#if defined(_M_AMD64)
#define IS_ARCH_X64 1
#elif defined(_M_ARM64)
#define IS_ARCH_ARM64 1
#else
#error Architecture not supported.
#endif

#else
#error Compiler not supported.
#endif

// release builds define NDEBUG
#if defined(NDEBUG)
#define IS_DEBUG 0
#else
#define IS_DEBUG 1
#endif

// only exist for -Wundef
#if !defined(IS_ARCH_X64)
#define IS_ARCH_X64 0
#endif
#if !defined(IS_ARCH_ARM64)
#define IS_ARCH_ARM64 0
#endif
#if !defined(IS_COMPILER_MSVC)
#define IS_COMPILER_MSVC 0
#endif
#if !defined(IS_COMPILER_CLANG)
#define IS_COMPILER_CLANG 0
#endif

#if IS_COMPILER_CLANG
#if __has_warning("-Wmissing-designated-field-initializers")
#pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"
#endif
#endif
