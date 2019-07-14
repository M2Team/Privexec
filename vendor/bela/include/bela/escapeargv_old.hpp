/// EscapeArgv
#ifndef BELA_ESCAPE_ARGV_HPP
#define BELA_ESCAPE_ARGV_HPP
#pragma once
#include <string>
#include <string_view>

namespace bela {
namespace argv_internal {
inline std::wstring escape_argument(std::wstring_view ac) {
  if (ac.empty()) {
    return L"\"\"";
  }
  bool hasspace = false;
  auto n = ac.size();
  for (auto c : ac) {
    switch (c) {
    case L'"':
      [[fallthrough]];
    case L'\\':
      n++;
      break;
    case ' ':
      [[fallthrough]];
    case '\t':
      hasspace = true;
      break;
    default:
      break;
    }
  }
  if (hasspace) {
    n += 2;
  }
  if (n == ac.size()) {
    return std::wstring{ac};
  }
  std::wstring buf;
  buf.reserve(n + 1);
  if (hasspace) {
    buf.push_back(L'"');
  }
  size_t slashes = 0;
  for (auto c : ac) {
    switch (c) {
    case L'\\':
      slashes++;
      buf.push_back(L'\\');
      break;
    case L'"': {
      for (; slashes > 0; slashes--) {
        buf.push_back(L'\\');
      }
      buf.push_back(L'\\');
      buf.push_back(c);
    } break;
    default:
      slashes = 0;
      buf.push_back(c);
      break;
    }
  }
  if (hasspace) {
    for (; slashes > 0; slashes--) {
      buf.push_back(L'\\');
    }
    buf.push_back(L'"');
  }
  return buf;
}
} // namespace argv_internal

class EscapeArgv {
public:
  EscapeArgv() = default;
  EscapeArgv(const EscapeArgv &) = delete;
  EscapeArgv &operator=(const EscapeArgv &) = delete;
  EscapeArgv &Assign(std::wstring_view arg0) {
    saver_.assign(argv_internal::escape_argument(arg0));
    return *this;
  }
  EscapeArgv &AssignNoEscape(std::wstring_view arg0) {
    saver_.assign(arg0);
    return *this;
  }
  EscapeArgv &Append(std::wstring_view aN) {
    if (saver_.empty()) {
      saver_.assign(argv_internal::escape_argument(aN));
      return *this;
    }
    auto ea = argv_internal::escape_argument(aN);
    saver_.reserve(saver_.size() + ea.size() + 1);
    saver_.append(L" ").append(ea);
    return *this;
  }
  std::wstring_view view() const { return std::wstring_view(saver_); }
  wchar_t *data() { return saver_.data(); } // CreateProcessW require
  const wchar_t *data() const { return saver_.data(); }
  size_t size() const { return saver_.size(); }

private:
  std::wstring saver_;
};

} // namespace bela

#endif
