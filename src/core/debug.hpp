#pragma once

#include <iostream>

// Debug logging macro that compiles out in release builds
// NDEBUG is defined automatically in release builds by most compilers
#ifndef NDEBUG
#define LOG_DEBUG(msg) std::cerr << msg
#else
#define LOG_DEBUG(msg) ((void)0)
#endif
