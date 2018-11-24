/////
#ifndef PRIVEXEC_WSUDO_HPP
#define PRIVEXEC_WSUDO_HPP
#include <cstring>
#include <string>
#include <vector>
#include <string_view>
#include <process/process.hpp>

namespace wsudo {

enum AppModeResult {
  AppFatal = -1, // Fatal
  AppNode = 0,
  AppHelp,
  AppVersion
};

inline bool Match(const wchar_t *arg, const wchar_t *l) {
  return (wcscmp(arg, l) == 0);
}

inline bool Match(const wchar_t *arg, const wchar_t *s, const wchar_t *l) {
  return (wcscmp(arg, s) == 0 || wcscmp(arg, l) == 0);
}

inline bool Match(const wchar_t *arg, const wchar_t *s1, const wchar_t *s2,
                  const wchar_t *l) {
  return (wcscmp(arg, s1) == 0 || wcscmp(arg, s2) == 0 || wcscmp(arg, l) == 0);
}

inline bool MatchEx(const wchar_t *arg, const wchar_t *s, const wchar_t *l,
                    std::wstring_view &value) {
  auto al = wcslen(arg);
  auto sl = wcslen(s);
  if (sl <= al && wcsncmp(arg, s, sl) == 0) {
    if (sl == al) {
      return true;
    }
    value = arg + al; /// -uappcontainer , -csomedir
    return true;
  }

  auto ll = wcslen(l);

  return false;
}

struct AppMode {
  std::vector<std::wstring_view> args;
  std::wstring_view cwd;              // --cwd -c
  int level{priv::ProcessNoElevated}; // -u --user
  bool disablealias{true};            // --disable-alias
  int ParseArgv(int argc, wchar_t **argv);
};

//
inline int AppMode::ParseArgv(int argc, wchar_t **argv) {
  int i = 1;
  for (; i < argc; i++) {
    auto arg = argv[i];
    if (arg[0] != '-') {
      break;
    }
    if (Match(arg, L"-?", L"-h", L"--help")) {
      return AppHelp;
    }
    if (Match(arg, L"-v", L"--version")) {
      return AppVersion;
    }
    if (Match(arg, L"--disable-alias")) {
      disablealias = true;
      continue;
    }
  }
  for (; i < argc; i++) {
    args.push_back(argv[i]);
  }
  return 0;
}

} // namespace wsudo

#endif