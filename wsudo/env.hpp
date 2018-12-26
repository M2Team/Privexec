#ifndef WSUDO_ENV_HPP
#define WSUDO_ENV_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>

namespace wsudo {
template <typename T> std::wstring Toupper(const T &t) {
  std::wstring s;
  s.reserve(t.size());
  for (const auto c : t) {
    s.push_back(::toupper(c));
  }
  return s;
}

inline std::wstring ExpandEnv(std::wstring_view s) {
  auto len = ExpandEnvironmentStringsW(s.data(), nullptr, 0);
  if (len <= 0) {
    return std::wstring(s.data(), s.size());
  }
  std::wstring s2(len + 1, L'\0');
  auto N = ExpandEnvironmentStringsW(s.data(), &s2[0], len + 1);
  s2.resize(N - 1);
  return s2;
}

class EnvContext {
public:
  using value_type = std::unordered_map<std::wstring, std::wstring>;
  EnvContext() = default;
  EnvContext(const EnvContext &) = delete;
  EnvContext &operator=(const EnvContext &) = delete;
  bool Append(std::wstring_view s) {
    auto pos = s.find(L'=');
    if (pos == std::wstring_view::npos) {
      em.insert_or_assign(Toupper(s), std::wstring());
      return true;
    }
    auto k = s.substr(0, pos);
    if (k.empty()) {
      return false;
    }
    auto v = s.substr(pos + 1);
    em.insert_or_assign(Toupper(k), ExpandEnv(v));
    return true;
  }
  // Normal
  bool Appendable(std::wstring_view ws) {
    auto pos = ws.find(L'=');
    if (pos == std::wstring_view::npos || pos == 0) {
      return false;
    }
    auto k = ws.substr(0, pos);
    auto v = ws.substr(pos + 1);
    em.insert_or_assign(Toupper(k), ExpandEnv(v));
    return true;
  }
  bool Apply(std::function<void(const std::wstring&, const std::wstring&)>
                 callback) const {
    if (em.empty()) {
      return true;
    }
    for (const auto &e : em) {
      callback(e.first, e.second); /// callback
      SetEnvironmentVariableW(e.first.c_str(),
                              e.second.empty() ? nullptr : e.second.c_str());
    }
    return true;
  }

private:
  value_type em;
};
} // namespace wsudo

#endif
