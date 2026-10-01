#pragma once

// Host platform identification for the runtime. Every macro is always defined
// as 0 or 1, so platform code tests `#if LO_PLATFORM_POSIX` rather than raw
// compiler macros. Use LO_PLATFORM_POSIX for code that relies only on POSIX
// APIs; use a specific platform only for APIs unique to it (memfd, Mach VM).
#if defined(_WIN32)
#define LO_PLATFORM_WINDOWS 1
#define LO_PLATFORM_LINUX 0
#define LO_PLATFORM_MACOS 0
#elif defined(__linux__)
#define LO_PLATFORM_WINDOWS 0
#define LO_PLATFORM_LINUX 1
#define LO_PLATFORM_MACOS 0
#elif defined(__APPLE__)
#define LO_PLATFORM_WINDOWS 0
#define LO_PLATFORM_LINUX 0
#define LO_PLATFORM_MACOS 1
#else
#error "Unsupported host platform"
#endif

#define LO_PLATFORM_POSIX (LO_PLATFORM_LINUX || LO_PLATFORM_MACOS)
