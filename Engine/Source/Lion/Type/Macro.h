#pragma once

// -------------------------------------------------------------------------
// Lion Engine - Macros and Compiler Directives
//
// Prefix Convention:
//   - LN_   : Used for preprocessor-level macros (build, platform, config)
//   - LION_ : Used for general-purpose engine macros within source code
// -------------------------------------------------------------------------

// Formats text using std::format (C++20).
#define LION_FORMAT_TEXT(message, ...) std::format(message, __VA_ARGS__)

#ifdef LN_DEBUG
    // Runtime assertion macro enabled only in debug builds.
    #define LION_ASSERT(condition, message)                                 \
        do {                                                                \
            if (!(condition)) {                                             \
                std::cerr << "Assertion failed: " << #condition << "\n"     \
                          << "Message: " << message << "\n"                 \
                          << "File: " << __FILE__ << "\n"                   \
                          << "Line: " << __LINE__ << std::endl;             \
                std::abort();                                               \
            }                                                               \
        } while (false)
#else
    #define LION_ASSERT(condition, message) do { (void)sizeof(condition); } while (false)
#endif
