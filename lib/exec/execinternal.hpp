///
#ifndef WSUDO_EXE_INTERNAL_HPP
#define WSUDO_EXE_INTERNAL_HPP
#include <vector>
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/strcat.hpp>
#include <bela/str_join.hpp>
#include "exec.hpp"

namespace wsudo::exec {

inline void FreeToken(HANDLE &hToken) {
  if (hToken != INVALID_HANDLE_VALUE) {
    CloseHandle(hToken);
    hToken = INVALID_HANDLE_VALUE;
  }
}
struct PrivilegeView {
  std::vector<const wchar_t *> privis;
  std::wstring dump() const {
    auto sz = privis.size();
    std::wstring s = L"[";
    for (size_t i = 0; i < sz; i++) {
      bela::StrAppend(&s, L"\"", privis[i], L"\"");
      if (i != sz - 1) {
        bela::StrAppend(&s, L", ");
      }
    }
    bela::StrAppend(&s, L"]");
    return s;
  }
};

bool GetCurrentSessionId(DWORD &dwSessionId);
// System process elavator
class Elevator {
public:
  Elevator() = default;
  Elevator(const Elevator &) = delete;
  Elevator &operator=(const Elevator &) = delete;
  ~Elevator() { FreeToken(hToken); }
  // System Process PID
  DWORD PID() const { return pid; }
  // Session ID
  DWORD SID() const { return sid; }
  bool elevate(const PrivilegeView *pv, bela::error_code &ec);

private:
  bool elevateimitate(bela::error_code &ec);
  HANDLE hToken{nullptr};
  DWORD sid{0};
  DWORD pid{0};
};
bool execute(command &cmd, bela::error_code &ec);
bool execute(HANDLE hToken, bool desktop, command &cmd, bela::error_code &ec);
bool executeti(command &cmd, bela::error_code &ec);
bool executesystem(command &cmd, bela::error_code &ec);
} // namespace wsudo::exec

#endif