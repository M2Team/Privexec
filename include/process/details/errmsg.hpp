//
#ifndef PRIVEXEC_ERRMES_HPP
#define PRIVEXEC_ERRMES_HPP
#include <string_view>
#include <string>
#include <optional>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
namespace priv {
struct error_code {
  long code{S_OK};
  bool operator()() { return code == S_OK; }
  std::wstring message;
  static error_code lasterror() {
    error_code ec;
    ec.code = GetLastError();
    LPWSTR buf = nullptr;
    auto rl = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr,
        ec.code, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&buf, 0,
        nullptr);
    if (rl == 0) {
      ec.message = L"unknown error";
      return ec;
    }
    ec.message.assign(buf, rl);
    LocalFree(buf);
    return ec;
  }
};

} // namespace priv

#endif