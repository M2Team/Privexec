#ifndef WSUDO_ENV_HPP
#define WSUDO_ENV_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>
#include <bela/ascii.hpp>
#include <bela/env.hpp>
#include <functional>

namespace wsudo {

class Derivator {
public:
  using value_type =
      bela::flat_hash_map<std::wstring, std::wstring, bela::env::StringCaseInsensitiveHash,
                          bela::env::StringCaseInsensitiveEq>;
  using apply_t = std::function<void(std::wstring_view, std::wstring_view)>;
  Derivator() = default;
  Derivator(const Derivator &) = delete;
  Derivator &operator=(const Derivator &) = delete;
  bool Append(std::wstring_view s) {
    auto pos = s.find(L'=');
    if (pos == std::wstring_view::npos) {
      em.insert_or_assign(s, std::wstring());
      return true;
    }
    auto k = s.substr(0, pos);
    if (k.empty()) {
      return false;
    }
    auto v = s.substr(pos + 1);
    em.insert_or_assign(k, bela::ExpandEnv(v));
    return true;
  }
  // Normal
  bool IsAppend(std::wstring_view ws) {
    auto pos = ws.find(L'=');
    if (pos == std::wstring_view::npos || pos == 0) {
      return false;
    }
    auto k = ws.substr(0, pos);
    auto v = ws.substr(pos + 1);
    em.insert_or_assign(k, bela::ExpandEnv(v));
    return true;
  }
  bool Apply(apply_t fn) const {
    if (em.empty()) {
      return true;
    }
    for (const auto &e : em) {
      fn(e.first, e.second); /// callback
      SetEnvironmentVariableW(e.first.data(), e.second.empty() ? nullptr : e.second.data());
    }
    return true;
  }
  const value_type &Items() const { return em; }

private:
  value_type em;
};
} // namespace wsudo

#endif
