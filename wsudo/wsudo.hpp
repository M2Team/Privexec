/////
#ifndef PRIVEXEC_WSUDO_HPP
#define PRIVEXEC_WSUDO_HPP
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <bela/terminal.hpp>
#include <process.hpp>
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

template <typename... Args>
bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt, Args... args) {
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
  return bela::terminal::WriteAuto(stderr,
                                   bela::StringCat(L"\x1b[32m", prefix, L" ", msg, L"\x1b[0m\n"));
}

struct AppMode {
  Derivator envctx;
  std::wstring message;
  std::vector<std::wstring_view> args;
  std::wstring_view cwd;     // --cwd -c
  std::wstring_view appx;    // --appx -x
  std::wstring_view appname; // --appname
  // int level{priv::ProcessNoElevated}; // -u --user
  priv::ExecLevel level{priv::ExecLevel::NoElevated};
  bool disablealias{false}; // --disable-alias
  bool wait{false};         // -w --wait
  bool lpac{false};
  priv::VisibleMode visible{priv::VisibleMode::None};
  const wchar_t *Visible() {
    switch (visible) {
    case priv::VisibleMode::None:
      return L"shared console window";
    case priv::VisibleMode::NewConsole:
      return L"new console window";
    case priv::VisibleMode::Hide:
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
