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

struct AppMode {
  Derivator envctx;
  std::wstring message;
  std::vector<std::wstring_view> args;
  std::wstring_view cwd;     // --cwd -c
  std::wstring_view appx;    // --appx -x
  std::wstring_view appname; // --appname
  // int level{priv::ProcessNoElevated}; // -u --user
  priv::ExecLevel level{priv::ExecLevel::NoElevated};
  bool verbose{false};      // --verbose -V
  bool disablealias{false}; // --disable-alias
  bool wait{false};         // -w --wait
  bool lpac{false};
  void Verbose(const wchar_t *fmt) const {
    if (verbose) {
      bela::terminal::WriteAuto(stderr, fmt);
    }
  }
  template <typename... Args>
  void Verbose(const wchar_t *fmt, Args... args) const {
    if (verbose) {
      const bela::format_internal::FormatArg arg_array[] = {args...};
      auto str = bela::format_internal::StrFormatInternal(fmt, arg_array,
                                                          sizeof...(args));
      bela::terminal::WriteAuto(stderr, str);
    }
  }

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
