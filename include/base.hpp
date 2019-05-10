#ifndef CLANGBUILDER_BASE_HPP
#define CLANGBUILDER_BASE_HPP
#pragma once
#include <SDKDDKVer.h>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <windows.h>
#endif
#include <optional>
#include <string>
#include <string_view>
#include <vector>
//
#include "strcat.hpp"
#include "string.hpp"

namespace base {
// final_act
// https://github.com/Microsoft/GSL/blob/ebe7ebfd855a95eb93783164ffb342dbd85cbc27\
// /include/gsl/gsl_util#L85-L89

template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

  final_act(final_act &&other) noexcept
      : f_(std::move(other.f_)), invoke_(other.invoke_) {
    other.invoke_ = false;
  }

  final_act(const final_act &) = delete;
  final_act &operator=(const final_act &) = delete;

  ~final_act() noexcept {
    if (invoke_)
      f_();
  }

private:
  F f_;
  bool invoke_;
};

// finally() - convenience function to generate a final_act
template <class F> inline final_act<F> finally(const F &f) noexcept {
  return final_act<F>(f);
}

template <class F> inline final_act<F> finally(F &&f) noexcept {
  return final_act<F>(std::forward<F>(f));
}
struct error_code {
  std::wstring message;
  long code{NO_ERROR};
  const wchar_t *data() const { return message.data(); }
  explicit operator bool() const noexcept { return code != NO_ERROR; }
};

inline error_code make_error_code(std::wstring_view msg, int val = -1) {
  return error_code{std::wstring(msg), val};
}

template <typename... Args>
inline error_code strcat_error_code(std::wstring_view v0,
                                    const Args &... args) {
  return error_code{strings_internal::CatPieces(
                        {v0, static_cast<const AlphaNum &>(args).Piece()...}),
                    -1};
}

inline std::wstring system_error_dump(DWORD ec) {
  LPWSTR buf = nullptr;
  auto rl = FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, ec,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&buf, 0, nullptr);
  if (rl == 0) {
    return L"FormatMessageW error";
  }
  std::wstring msg(buf, rl);
  LocalFree(buf);
  return msg;
}

inline error_code make_system_error_code() {
  error_code ec;
  ec.code = GetLastError();
  ec.message = system_error_dump(ec.code);
  return ec;
}

// ToNarrow UTF-16 to UTF-8
inline std::string ToNarrow(std::wstring_view uw) {
  auto l = WideCharToMultiByte(CP_UTF8, 0, uw.data(), (int)uw.size(), nullptr,
                               0, nullptr, nullptr);
  std::string ustr;
  ustr.resize(l + 1);
  auto N = WideCharToMultiByte(CP_UTF8, 0, uw.data(), (int)uw.size(),
                               ustr.data(), l + 1, nullptr, nullptr);
  ustr.resize(N);
  return ustr;
}

// ToWide UTF-8 to UTF-16
inline std::wstring ToWide(std::string_view u8) {
  std::wstring wstr;
  auto N =
      MultiByteToWideChar(CP_UTF8, 0, u8.data(), (DWORD)u8.size(), nullptr, 0);
  if (N > 0) {
    wstr.resize(N);
    MultiByteToWideChar(CP_UTF8, 0, u8.data(), (DWORD)u8.size(), &wstr[0], N);
  }
  return wstr;
}

} // namespace base

#endif
