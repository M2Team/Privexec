/// Env helper
#ifndef BELA_ENV_HPP
#define BELA_ENV_HPP
#include <string>
#include <string_view>
#include <shared_mutex>
#include "match.hpp"
#include "str_split.hpp"
#include "span.hpp"
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
constexpr const wchar_t Separator = L';';

struct StringCaseInsensitiveHash {
  using is_transparent = void;
  std::size_t operator()(std::wstring_view wsv) const noexcept {
    /// See Wikipedia
    /// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
#if defined(__x86_64) || defined(_WIN64)
    static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
    constexpr size_t kFNVOffsetBasis = 14695981039346656037ULL;
    constexpr size_t kFNVPrime = 1099511628211ULL;
#else
    static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
    constexpr size_t kFNVOffsetBasis = 2166136261U;
    constexpr size_t kFNVPrime = 16777619U;
#endif
    size_t val = kFNVOffsetBasis;
    std::string_view sv = {reinterpret_cast<const char *>(wsv.data()),
                           wsv.size() * 2};
    for (auto c : sv) {
      val ^= static_cast<size_t>(bela::ascii_tolower(c));
      val *= kFNVPrime;
    }
    return val;
  }
};

struct StringCaseInsensitiveEq {
  using is_transparent = void;
  bool operator()(std::wstring_view wlhs, std::wstring_view wrhs) const {
    return bela::EqualsIgnoreCase(wlhs, wrhs);
  }
};

// Derivator Environment variable derivation container
class Derivator {
public:
  using value_type =
      bela::flat_hash_map<std::wstring, std::wstring, StringCaseInsensitiveHash,
                          StringCaseInsensitiveEq>;
  Derivator() = default;
  Derivator(const Derivator &) = delete;
  Derivator &operator=(const Derivator &) = delete;
  bool AddBashCompatible(int argc, wchar_t *const *argv);
  bool EraseEnv(std::wstring_view key);
  bool SetEnv(std::wstring_view key, std::wstring_view value,
              bool force = false);
  bool PutEnv(std::wstring_view nv, bool force = false);
  [[nodiscard]] std::wstring_view GetEnv(std::wstring_view key) const;
  // ExpandEnv POSIX style ${KEY}. if not enable strict, use
  // GetEnvironmentVariableW if key not exists envb
  bool ExpandEnv(std::wstring_view raw, std::wstring &w,
                 bool strict = false) const;
  std::wstring Encode() const;

private:
  bool AppendEnv(std::wstring_view key, std::wstring &s) const;
  value_type envb;
};

class DerivatorMT {
public:
  using value_type = bela::parallel_flat_hash_map<std::wstring, std::wstring,
                                                  StringCaseInsensitiveHash,
                                                  StringCaseInsensitiveEq>;
  DerivatorMT() = default;
  DerivatorMT(const DerivatorMT &) = delete;
  DerivatorMT &operator=(const DerivatorMT &) = delete;
  bool AddBashCompatible(int argc, wchar_t *const *argv);
  bool EraseEnv(std::wstring_view key);
  bool SetEnv(std::wstring_view key, std::wstring_view value,
              bool force = false);
  bool PutEnv(std::wstring_view nv, bool force = false);
  [[nodiscard]] std::wstring GetEnv(std::wstring_view key);
  // ExpandEnv
  bool ExpandEnv(std::wstring_view raw, std::wstring &w, bool strict = false);
  std::wstring Encode();

private:
  bool AppendEnv(std::wstring_view key, std::wstring &s);
  value_type envb;
};

inline std::wstring JoinEnvInternal(std::wstring_view key, bool append,
                                    const bela::Span<std::wstring_view> args) {
  auto val = bela::GetEnv(key);
  std::vector<std::wstring_view> pvv =
      bela::StrSplit(val, bela::ByChar(Separator), bela::SkipEmpty());
  std::wstring s;
  size_t i = val.size();
  for (auto a : args) {
    i += a.size() + 1;
  }
  s.reserve(i);
  if (append) {
    for (auto v : pvv) {
      if (!s.empty()) {
        s.push_back(Separator);
      }
      s.append(v);
    }
    for (auto a : args) {
      if (!s.empty()) {
        s.push_back(Separator);
      }
      s.append(a);
    }
  } else {
    for (auto a : args) {
      if (!s.empty()) {
        s.push_back(Separator);
      }
      s.append(a);
    }
    for (auto v : pvv) {
      if (!s.empty()) {
        s.push_back(Separator);
      }
      s.append(v);
    }
  }
  return s;
}

template <typename... Args>
std::wstring AppendEnv(std::wstring_view key, Args... arg) {
  std::wstring_view svv[] = {arg...};
  return JoinEnvInternal(key, true, svv);
}

template <typename... Args>
std::wstring InsertEnv(std::wstring_view key, Args... arg) {
  std::wstring_view svv[] = {arg...};
  return JoinEnvInternal(key, false, svv);
}

} // namespace env

} // namespace bela

#endif
