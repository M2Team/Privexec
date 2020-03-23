///
#ifndef PRIVEXEC_APPUTILS_HPP
#define PRIVEXEC_APPUTILS_HPP
#include <bela/path.hpp>

namespace priv {
inline std::optional<std::wstring> ExecutablePath(bela::error_code &ec) {
  auto exe = bela::Executable(ec);
  if (!exe) {
    return std::nullopt;
  }
  std::wstring path;
  if (!bela::LookupRealPath(*exe, path)) {
    bela::PathStripName(*exe);
    return std::make_optional(std::move(*exe));
  }
  bela::PathStripName(path);
  return std::make_optional(std::move(path));
}
}; // namespace priv

#endif
