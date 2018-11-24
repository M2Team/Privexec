/////
#ifndef WSUDOALIAS_HPP
#define WSUDOALIAS_HPP
#pragma once
#include <string>
#include <optional>
#include <unordered_map>

namespace wsudo {

struct AliasTarget {
  std::wstring target;
  std::wstring desc;
};

class AliasEngine {
public:
  using value_type = std::unordered_map<std::wstring, AliasTarget>;
  AliasEngine() = default;
  AliasEngine(const AliasEngine &) = delete;
  AliasEngine &operator=(const AliasEngine &) = delete;
  ~AliasEngine();
  bool Initialize();
  std::optional<std::wstring> Target(const std::wstring &al);
  bool Set(const wchar_t *key, const wchar_t *target, const wchar_t *desc) {
    alias[key] = AliasTarget{target, desc};
    updated = true;
    return true;
  }
  bool Delete(const wchar_t *key) {
    auto it = alias.find(key);
    if (it == alias.end()) {
      return false;
    }
    alias.erase(key);
    updated = true;
    return true;
  }
  /// prevent apply
  void Prevent() { updated = false; }

private:
  bool updated{false};
  bool Apply();
  value_type alias;
};
int AliasSubcmd(const std::vector<std::wstring_view> &argv, bool verbose);
} // namespace wsudo

#endif
