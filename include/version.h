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

#define PRIVEXEC_MAJOR_VERSION 2
#define PRIVEXEC_MINOR_VERSION 5
#define PRIVEXEC_PATCH_VERSION 0

#define PRIV_MAJOR TOSTR(PRIVEXEC_MAJOR_VERSION)
#define PRIV_MINOR TOSTR(PRIVEXEC_MINOR_VERSION)
#define PRIV_PATCH TOSTR(PRIVEXEC_PATCH_VERSION)

#define PRIV_VERSION_MAIN PRIV_MAJOR L"." PRIV_MINOR
#define PRIV_VERSION_FULL PRIV_VERSION_MAIN L"." PRIV_PATCH

#ifdef APPVEYOR_BUILD_NUMBER
#define PRIVEXEC_BUILD_VERSION PRIV_VERSION_FULL L"." PRIVSUBVER L" (appveyor)"
#else
#define PRIVEXEC_BUILD_VERSION PRIV_VERSION_FULL L"." PRIVSUBVER L" (dev)"
#endif

#define PRIVEXEC_APPLINK                                                       \
  L"For more information about this tool. \nVisit: <a "                        \
  L"href=\"https://github.com/M2Team/Privexec\">Privexec</a>\nVisit: <a "      \
  L"href=\"https://forcemz.net/\">forcemz.net</a>"

#define PRIVEXEC_APPLINKE                                                      \
  L"Ask for help with this issue. \nVisit: <a "                                \
  L"href=\"https://github.com/M2Team/Privexec/issues\">Privexec Issues</a>"

#define PRIVEXEC_COPYRIGHT                                                     \
  L"Copyright \xA9 2020, Force Charlie. All Rights Reserved."

#define PRIVEXEC_APPVERSION                                                    \
  L"Version: " PRIV_VERSION_FULL L"\nCopyright \xA9 2020, Force "              \
  L"Charlie. All Rights Reserved."

#endif
