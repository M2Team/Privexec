////////////
#ifndef PRIVEXEC_ELEVATOR_HPP
#define PRIVEXEC_ELEVATOR_HPP
#include <vector>
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/strcat.hpp>

namespace priv {
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

inline bool IsUserAdministratorsGroup() {
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                &AdministratorsGroup)) {
    return false;
  }
  BOOL b = FALSE;
  if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
    b = FALSE;
  }
  FreeSid(AdministratorsGroup);
  return b == TRUE;
}

bool GetCurrentSessionId(DWORD &dwSessionId);
// System process elavator
class Elevator {
public:
  Elevator() = default;
  Elevator(const Elevator &) = delete;
  Elevator &operator=(const Elevator &) = delete;
  ~Elevator() {
    if (hToken != INVALID_HANDLE_VALUE) {
      ::CloseHandle(hToken);
    }
  }
  // System Process PID
  DWORD PID() const { return pid; }
  // Session ID
  DWORD SID() const { return sid; }
  bool Elevate(const PrivilegeView *pv, std::wstring &klast);

private:
  bool ElevateImitate(std::wstring &klast);
  HANDLE hToken{nullptr};
  DWORD sid{0};
  DWORD pid{0};
};
} // namespace priv

#endif