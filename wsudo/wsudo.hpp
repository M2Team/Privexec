/////
#ifndef PRIVEXEC_WSUDO_HPP
#define PRIVEXEC_WSUDO_HPP
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <process/process.hpp>
#include "env.hpp"

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
    if (al == ll) {
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
  EnvContext envctx;
  std::wstring message;
  std::vector<std::wstring_view> args;
  std::wstring_view cwd;              // --cwd -c
  std::wstring_view appx;             // --appx -x
  std::wstring_view appname;          // --appname
  int level{priv::ProcessNoElevated}; // -u --user
  bool verbose{false};                // --verbose -V
  bool disablealias{false};           // --disable-alias
  bool wait{false};                   // -w --wait
  bool lpac{false};
  priv::visiblemode_t visible{priv::VisibleNone};
  const wchar_t *Visible() {
    switch (visible) {
    case priv::VisibleNone:
      return L"shared console window";
    case priv::VisibleNewConsole:
      return L"new console window";
    case priv::VisibleHide:
      return L"no console window";
    default:
      break;
    }
    return L"none";
  }
  int ParseArgv(int argc, wchar_t **argv);
  void Verbose();
};

} // namespace wsudo

#endif
