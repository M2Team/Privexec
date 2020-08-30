/////
#ifndef WSUDO_HPP
#define WSUDO_HPP
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <bela/terminal.hpp>
#include <bela/simulator.hpp>
#include <exec.hpp>
#include "env.hpp"

namespace wsudo {
extern bool IsDebugMode;
// DbgPrint added newline
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
}

template <typename... Args> bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  auto str = bela::StringCat(L"\x1b[32m* ", prefix, L" ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}

inline bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[32m", prefix, L" ", msg, L"\x1b[0m\n"));
}

struct App {
  bela::env::Simulator simulator;
  std::vector<std::wstring> argv;
  std::vector<std::wstring> envlist;
  std::wstring path;
  std::wstring cwd;   // --cwd -c
  std::wstring appx;  // --appx -x
  std::wstring appid; // --appid
  wsudo::exec::privilege_t level{wsudo::exec::privilege_t::standard};
  wsudo::exec::visible_t visible{wsudo::exec::visible_t::none};
  bool disablealias{false}; // --disable-alias
  bool lpac{false};
  bool wait{false};
  bool nowait{false};
  bool console{true};
  int AppExecute();
  int Execute();
  int DelegateExecute();
  void SetEnv(std::wstring_view es) {
    auto envstr = bela::WindowsExpandEnv(es);
    std::wstring_view nv(envstr);
    if (auto pos = nv.find(L'='); pos != std::wstring_view::npos) {
      auto name = nv.substr(0, pos);
      if (bela::EqualsIgnoreCase(name, L"Path")) {
        std::vector<std::wstring_view> paths =
            bela::StrSplit(nv.substr(pos + 1), bela::ByChar(L';'), bela::SkipEmpty());
        for (const auto p : paths) {
          simulator.PathPushFront(p);
        }
      } else {
        simulator.SetEnv(name, nv.substr(pos + 1), true);
      }
    }
    // simulator.PutEnv(bela::WindowsExpandEnv(va), true);
    envlist.emplace_back(envstr);
  }
  int ParseArgv(int argc, wchar_t **argv);
  bool SplitArgv();
  // normal
  bool Waitable() const {
    if (nowait) {
      return false;
    }
    if (wait) {
      return true;
    }
    if (!console) {
      return false;
    }
    return visible == wsudo::exec::visible_t::none;
  }
};
int WaitForExit(DWORD pid);
} // namespace wsudo

#endif
