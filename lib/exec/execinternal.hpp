///
#ifndef WSUDO_EXE_INTERNAL_HPP
#define WSUDO_EXE_INTERNAL_HPP
#include <vector>
#include <span>
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/str_cat.hpp>
#include <bela/str_join.hpp>
#include "exec.hpp"

namespace wsudo::exec {

inline void FreeToken(HANDLE &hToken) {
  if (hToken != INVALID_HANDLE_VALUE) {
    CloseHandle(hToken);
    hToken = INVALID_HANDLE_VALUE;
  }
}
struct privilege_entries {
  template <typename... Args> privilege_entries(Args... a) { entries = std::initializer_list{a...}; }
  std::vector<const wchar_t *> entries;
  std::wstring format() const {
    auto sz = entries.size();
    std::wstring s = L"[";
    for (size_t i = 0; i < sz; i++) {
      bela::StrAppend(&s, L"\"", entries[i], L"\"");
      if (i != sz - 1) {
        bela::StrAppend(&s, L", ");
      }
    }
    bela::StrAppend(&s, L"]");
    return s;
  }
};

bool GetCurrentSessionToken(PHANDLE hNewToken, bela::error_code &ec);
bool GetCurrentSessionId(DWORD &dwSessionId);

// System process elavator
class Elavator {
public:
  Elavator() = default;
  Elavator(const Elavator &) = delete;
  Elavator &operator=(const Elavator &) = delete;
  ~Elavator() { FreeToken(hToken); }
  // Session ID
  DWORD SessionID() const { return currentSessionId; }
  const auto &SystemProcesses() const { return systemProcesses; }
  bool ImpersonationSystemPrivilege(const privilege_entries *pv, bela::error_code &ec);

private:
  bool impersonation_system_token(DWORD systemProcessId, bela::error_code &ec);
  HANDLE hToken{nullptr};
  DWORD currentSessionId{0};
  // system process list
  std::vector<DWORD> systemProcesses;
};
bool execute_basic(command &cmd, bela::error_code &ec);
bool execute_with_ti(command &cmd, bela::error_code &ec);
bool execute_with_system(command &cmd, bela::error_code &ec);
bool execute_with_token(command &cmd, HANDLE hToken, bool desktop, LPVOID lpEnvironment, bela::error_code &ec);
} // namespace wsudo::exec

#endif