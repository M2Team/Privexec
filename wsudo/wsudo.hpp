/////
#ifndef PRIVEXEC_WSUDO_HPP
#define PRIVEXEC_WSUDO_HPP
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <process/process.hpp>

namespace wsudo {

enum AppModeResult {
  AppFatal = -1, // Fatal
  AppNone = 0,
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
  value = L"";
  auto al = wcslen(arg);
  auto sl = wcslen(s);
  if (sl <= al && wcsncmp(arg, s, sl) == 0) {
    if (sl == al) {
      return true;
    }
    value = arg + sl; /// -uappcontainer , -csomedir
    return true;
  }
  auto ll = wcslen(l);
  if (ll <= al && wcsncmp(arg, l, ll) == 0) {
    if (sl == ll) {
      return true;
    }
    /// --user=appcontainer
    if (arg[ll] != L'=' || ll + 1 >= al) {
      return false;
    }
    value = arg + ll + 1;
    return true;
  }
  return false;
}

struct AppMode {
  std::wstring message;
  std::vector<std::wstring_view> args;
  std::wstring_view cwd;              // --cwd -c
  std::wstring_view appx;             // --appx -x
  int level{priv::ProcessNoElevated}; // -u --user
  bool verbose{false};                // --verbose -V
  bool disablealias{false};           // --disable-alias
  bool wait{false};                   // -w --wait
  bool newconsole{false};             // -n --new-console
  bool IsAppLevel(const wchar_t *arg);
  bool IsAppLevelKey(std::wstring_view k);
  int ParseArgv(int argc, wchar_t **argv);
  void Verbose();
};

} // namespace wsudo

#endif