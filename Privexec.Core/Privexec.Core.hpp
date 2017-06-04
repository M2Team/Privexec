#ifndef PRIVEXEC_CORE_HPP
#define PRIVEXEC_CORE_HPP
#pragma once

#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif

enum PrivexecUsersLevel {
  kAppContainer = 0,
  kMandatoryIntegrityControl,
  kUACElevated,
  kAdministrator,
  kSystem,
  kTrustedInstaller
};

BOOL EnableDebugPrivilege(void);
HANDLE OpenSystemProcessToken();
bool GetCurrentSessionId(DWORD &dwSessionId);

bool CreateAppContainerProcess(LPWSTR pszComline, DWORD &dwProcessId);
bool CreateLowlevelProcess(LPWSTR pszCmdline, DWORD &dwProcessId);
bool CreateNoElevatedProcess(LPWSTR pszCmdline, DWORD &dwProcessId);
bool CreateAdministratorsProcess(LPWSTR pszCmdline, DWORD &dwProcessId);
bool CreateSystemProcess(LPWSTR pszCmdline, DWORD &dwProcessId);
bool CreateTiProcess(LPWSTR pszCmdline, DWORD &dwProcessId);
bool PrivCreateProcess(int level, LPWSTR pszCmdline, DWORD &dwProcessId);

class ErrorMessage {
public:
  ErrorMessage(DWORD errid) : lastError(errid) {
    if (FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, errid, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (LPWSTR)&buf, 0, nullptr) == 0) {
      buf = nullptr;
    }
  }
  ~ErrorMessage() {
    if (buf != nullptr) {
      LocalFree(buf);
    }
  }
  const wchar_t *message() const { return buf == nullptr ? L"unknwon" : buf; }
  DWORD LastError() const { return lastError; }

private:
  DWORD lastError;
  LPWSTR buf{nullptr};
};

inline bool IsUserAdministratorsGroup()
/*++
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token.
Arguments: None.
Return Value:
TRUE - Caller has Administrators local group.
FALSE - Caller does not have Administrators local group. --
*/
{
  BOOL b;
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &AdministratorsGroup);
  if (b) {
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
      b = FALSE;
    }
    FreeSid(AdministratorsGroup);
  }

  return b == TRUE;
}

#endif
