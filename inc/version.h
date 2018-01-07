////
#ifndef PRIVEXEC_VERSION_H
#define PRIVEXEC_VERSION_H

#include "config.h"

#ifdef APPVEYOR_BUILD_NUMBER
#define PRIVEXEC_BUILD_NUMBER APPVEYOR_BUILD_NUMBER
#else
#define PRIVEXEC_BUILD_NUMBER 1
#endif

#define TOSTR_1(x) L#x
#define TOSTR(x) TOSTR_1(x)

#define PRIVSUBVER TOSTR(PRIVEXEC_BUILD_NUMBER)

#ifdef APPVEYOR_BUILD_NUMBER
#define PRIVEXEC_BUILD_VERSION L"1.1.0." PRIVSUBVER L" (appveyor)"
#else
#define PRIVEXEC_BUILD_VERSION L"1.1.0." PRIVSUBVER L" (dev)"
#endif

#endif
