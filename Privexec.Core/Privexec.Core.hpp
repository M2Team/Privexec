#ifndef PRIVEXEC_CORE_HPP
#define PRIVEXEC_CORE_HPP
#pragma once

#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <string>
#include <vector>

enum PrivexecUsersLevel {
  kAppContainer = 0,
  kMandatoryIntegrityControl,
  kUACElevated,
  kAdministrator,
  kSystem,
  kTrustedInstaller
};

namespace priv {
struct error_code {
  std::wstring msg;
  int ec{0};
  bool operator()() { return ec == 0; }
  const std::wstring &message() const { return msg; }
};
// Add key todo
bool CapabilitiesAdd(std::vector<WELL_KNOWN_SID_TYPE> &caps, const char *key);
bool WellKnownFromAppmanifest(const std::wstring &file,
                              std::vector<WELL_KNOWN_SID_TYPE> &sids);

class AppContainerContext {
public:
  using capabilities = std::vector<SID_AND_ATTRIBUTES>;
  AppContainerContext() = default;
  AppContainerContext(const AppContainerContext &) = delete;
  AppContainerContext &operator=(const AppContainerContext &) = delete;
  ~AppContainerContext() {
    for (auto &c : ca) {
      if (c.Sid) {
        HeapFree(GetProcessHeap(), 0, c.Sid);
      }
    }
    if (appcontainersid != nullptr) {
      FreeSid(appcontainersid);
    }
  }
  bool InitializeWithCapabilities(WELL_KNOWN_SID_TYPE *knarr, int arraylen);
  bool InitializeWithSID(const std::wstring &ssid);
  bool InitializeWithAppmanifest(const std::wstring &file);
  bool Initialize();
  capabilities &Capabilitis() { return ca; }
  PSID AppContainerSid() const { return appcontainersid; }
  bool Execute(LPWSTR pszComline, DWORD &dwProcessId,
               LPWSTR lpCurrentDirectory);

private:
  PSID appcontainersid{nullptr};
  capabilities ca;
};

BOOL EnableDebugPrivilege(void);
HANDLE OpenSystemProcessToken();
bool GetCurrentSessionId(DWORD &dwSessionId);

// AppContainer support SID
bool CreateAppContainerProcess(LPWSTR pszComline, DWORD &dwProcessId,
                               LPWSTR lpCurrentDirectory,
                               const std::wstring &appmanifest = L"");
bool CreateLowlevelProcess(LPWSTR pszCmdline, DWORD &dwProcessId,
                           LPWSTR lpCurrentDirectory);
bool CreateNoElevatedProcess(LPWSTR pszCmdline, DWORD &dwProcessId,
                             LPWSTR lpCurrentDirectory);
bool CreateAdministratorsProcess(LPWSTR pszCmdline, DWORD &dwProcessId,
                                 LPWSTR lpCurrentDirectory);
bool CreateSystemProcess(LPWSTR pszCmdline, DWORD &dwProcessId,
                         LPWSTR lpCurrentDirectory);
bool CreateTiProcess(LPWSTR pszCmdline, DWORD &dwProcessId,
                     LPWSTR lpCurrentDirectory);
bool PrivCreateProcess(int level, LPWSTR pszCmdline, DWORD &dwProcessId,
                       LPWSTR lpCurrentDirectory);

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

} // namespace priv

#endif
