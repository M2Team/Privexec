// Generate code by cmake, don't modify
#ifndef PRIVEXEC_VERSION_H
#define PRIVEXEC_VERSION_H

#define PRIVEXEC_VERSION_MAJOR ${PRIVEXEC_VERSION_MAJOR}
#define PRIVEXEC_VERSION_MINOR ${PRIVEXEC_VERSION_MINOR}
#define PRIVEXEC_VERSION_PATCH ${PRIVEXEC_VERSION_PATCH}
#define PRIVEXEC_VERSION_BUILD 0

#define PRIVEXEC_VERSION L"${PRIVEXEC_VERSION_MAJOR}.${PRIVEXEC_VERSION_MINOR}.${PRIVEXEC_VERSION_PATCH}"
#define PRIVEXEC_REVISION L"${PRIVEXEC_REVISION}"
#define PRIVEXEC_REFNAME L"${PRIVEXEC_REFNAME}"
#define PRIVEXEC_BUILD_TIME L"${PRIVEXEC_BUILD_TIME}"
#define PRIVEXEC_REMOTE_URL L"${PRIVEXEC_REMOTE_URL}"

#define PRIVEXEC_APPLINK                                                                              \
  L"For more information about this tool. \nVisit: <a "                                            \
  L"href=\"https://github.com/M2Team/Privexec\">Privexec</"                                               \
  L"a>\nVisit: <a "                                                                                \
  L"href=\"https://forcemz.net/\">forcemz.net</a>"

#define PRIVEXEC_APPLINKE                                                                             \
  L"Ask for help with this issue. \nVisit: <a "                                                    \
  L"href=\"https://github.com/M2Team/Privexec/issues\">Privexec "                                         \
  L"Issues</a>"

#define PRIVEXEC_APPVERSION                                                                           \
  L"Version: ${PRIVEXEC_VERSION_MAJOR}.${PRIVEXEC_VERSION_MINOR}.${PRIVEXEC_VERSION_PATCH}\nCopyright "     \
  L"\xA9 ${PRIVEXEC_COPYRIGHT_YEAR}, Privexec contributors."

#define PRIVEXECUI_COPYRIGHT L"Copyright \xA9 ${PRIVEXEC_COPYRIGHT_YEAR}, Privexec contributors."

#endif
