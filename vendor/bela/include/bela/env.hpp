/// Env helper
#ifndef BELA_ENV_HPP
#define BELA_ENV_HPP
#include <string>
#include <string_view>
#include <shared_mutex>
#include "phmap.hpp"
#include "base.hpp"

namespace bela {
// GetEnv You should set the appropriate size for the initial allocation
// according to your needs.
// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-getenvironmentvariable
template <size_t Len = 256> std::wstring GetEnv(std::wstring_view val) {
  std::wstring s;
  s.resize(Len);
  auto len = GetEnvironmentVariableW(val.data(), s.data(), Len);
  if (len == 0) {
    return L"";
  }
  if (len < Len) {
    s.resize(len);
    return s;
  }
  s.resize(len);
  auto nlen = GetEnvironmentVariableW(val.data(), s.data(), len);
  if (nlen == 0 || nlen > len) {
    return L"";
  }
  s.resize(nlen);
  return s;
}
std::wstring ExpandEnv(std::wstring_view sv);
std::wstring PathUnExpand(std::wstring_view sv);

namespace env {
// Derivator Expand Env buitin. upper
class Derivator {
public:
  Derivator() = default;
  Derivator(const Derivator &) = delete;
  Derivator &operator=(const Derivator &) = delete;
  bool AddBashCompatible(int argc, wchar_t *const *argv);
  bool EraseEnv(std::wstring_view key);
  bool SetEnv(std::wstring_view key, std::wstring_view value,
              bool force = false);
  bool PutEnv(std::wstring_view nv, bool force = false);
  [[nodiscard]] std::wstring_view GetEnv(std::wstring_view key) const;
  // ExpandEnv POSIX style ${KEY}. if not enable disableos, use
  // GetEnvironmentVariableW if key not exists envblock
  bool ExpandEnv(std::wstring_view raw, std::wstring &w,
                 bool disableos = false) const;

private:
  bool AppendEnv(std::wstring_view key, std::wstring &s) const;
  bela::flat_hash_map<std::wstring, std::wstring> envblock;
};

class DerivatorMT {
public:
  DerivatorMT() = default;
  DerivatorMT(const DerivatorMT &) = delete;
  DerivatorMT &operator=(const DerivatorMT &) = delete;
  bool AddBashCompatible(int argc, wchar_t *const *argv);
  bool EraseEnv(std::wstring_view key);
  bool SetEnv(std::wstring_view key, std::wstring_view value,
              bool force = false);
  bool PutEnv(std::wstring_view nv, bool force = false);
  [[nodiscard]] std::wstring GetEnv(std::wstring_view key);
  bool ExpandEnv(std::wstring_view raw, std::wstring &w,
                 bool disableos = false);

private:
  bool AppendEnv(std::wstring_view key, std::wstring &s);
  bela::parallel_flat_hash_map<std::wstring, std::wstring> envblock;
};

} // namespace env

} // namespace bela

#endif
