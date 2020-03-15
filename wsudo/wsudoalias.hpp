/////
#ifndef WSUDOALIAS_HPP
#define WSUDOALIAS_HPP
#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <bela/codecvt.hpp>
#include <bela/phmap.hpp>

namespace wsudo {
struct AliasTarget {
  std::wstring target;
  std::wstring desc;
  AliasTarget() = default;
  AliasTarget(std::wstring_view t, std::wstring_view d) : target(t), desc(d) {}
  AliasTarget(std::string_view t, std::string_view d)
      : target(bela::ToWide(t)), desc(bela::ToWide(d)) {}
};

class AliasEngine {
public:
  using value_type = bela::flat_hash_map<std::wstring, AliasTarget>;
  AliasEngine() = default;
  AliasEngine(const AliasEngine &) = delete;
  AliasEngine &operator=(const AliasEngine &) = delete;
  ~AliasEngine();
  bool Initialize(bool verbose = false);
  std::optional<std::wstring> Target(std::wstring_view al);
  bool Set(std::wstring_view key, std::wstring_view target,
           std::wstring_view desc) {
    alias.insert_or_assign(key, AliasTarget(target, desc));
    updated = true;
    return true;
  }
  bool Delete(std::wstring_view key) {
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
