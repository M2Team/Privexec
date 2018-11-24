/////
#ifndef WSUDOALIAS_HPP
#define WSUDOALIAS_HPP
#pragma once
#include <string>
#include <optional>

namespace wsudo {
//
class AliasEngine {
public:
  AliasEngine() = default;
  AliasEngine(const AliasEngine &) = delete;
  AliasEngine &operator=(const AliasEngine &) = delete;
  bool Initialize();
  std::optional<std::wstring> Target(const std::wstring &al);

private:
  bool updated{false};
};
} // namespace wsudo

#endif
