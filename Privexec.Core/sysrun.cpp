#include "stdafx.h"
#include <WtsApi32.h>
#include <cstdio>
#include "Privexec.Core.hpp"

BOOL IsSystemSid(PSID sid) { return ::IsWellKnownSid(sid, WinLocalSystemSid); }

// find first SYSTEM process with a good token to use
namespace priv {
HANDLE OpenSystemProcessToken() {
  PWTS_PROCESS_INFO pInfo;
  DWORD count;
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    return INVALID_HANDLE_VALUE;
  }
  if (!::WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pInfo,
                               &count)) {
    printf(
        "Error enumerating processes (are you running elevated?) (error=%d)\n",
        ::GetLastError());
    return INVALID_HANDLE_VALUE;
  }

  HANDLE hExistingToken = nullptr;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  for (DWORD i = 0; i < count && !hExistingToken; i++) {
    if (pInfo[i].SessionId == dwSessionId && IsSystemSid(pInfo[i].pUserSid)) {
      auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pInfo[i].ProcessId);
      if (hProcess) {
        ::OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken);
        ::CloseHandle(hProcess);
      }
    }
  }

  ::WTSFreeMemory(pInfo);
  if (hExistingToken != nullptr) {
    DuplicateTokenEx(hExistingToken, MAXIMUM_ALLOWED, nullptr,
                     SecurityIdentification, TokenPrimary, &hToken);
    CloseHandle(hExistingToken);
  }
  return hToken;
}

BOOL SetPrivilege(HANDLE hToken, PCTSTR lpszPrivilege, bool bEnablePrivilege) {
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (!::LookupPrivilegeValue(nullptr, lpszPrivilege, &luid))
    return FALSE;

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  if (bEnablePrivilege)
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  else
    tp.Privileges[0].Attributes = 0;

  // Enable the privilege or disable all privileges.

  if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                               nullptr, nullptr)) {
    return FALSE;
  }

  if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    return FALSE;

  return TRUE;
}

BOOL EnableDebugPrivilege(void) {
  HANDLE hToken;
  BOOL result;
  if (!::OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    return FALSE;
  }
  result = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
  ::CloseHandle(hToken);
  return result;
}

bool CreateSystemProcess(LPWSTR pszCmdline, DWORD &dwProcessId) {
  if (FALSE == EnableDebugPrivilege()) {
    SetLastError(ERROR_ACCESS_DENIED);
    printf("EnableDebugPrivilege failed: Access denied (are you running "
           "elevated?)\n");
    return false;
  }

  auto hToken = OpenSystemProcessToken();
  if (!hToken) {
    SetLastError(ERROR_ACCESS_DENIED);
    printf("OpenSystemProcessToken failed: Access denied (are you running "
           "elevated?)\n");
    return false;
  }

  HANDLE hDupToken, hPrimary;
  ::DuplicateTokenEx(hToken,
                     TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY |
                         TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_PRIVILEGES,
                     nullptr, SecurityImpersonation, TokenImpersonation,
                     &hDupToken);
  ::DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation,
                     TokenPrimary, &hPrimary);
  ::CloseHandle(hToken);

  if (hDupToken == nullptr) {
    printf("Failed to create token (are you running elevated?) (error=%d)\n",
           ::GetLastError());
    return false;
  }
  STARTUPINFO si = {sizeof(si)};
  si.lpDesktop = L"Winsta0\\default";

  PROCESS_INFORMATION pi;

  BOOL impersonated = ::SetThreadToken(nullptr, hDupToken);
  if (!impersonated) {
    printf(
        "SetThreadToken failed: Access denied (are you running elevated?)\n");
    return 1;
  }

  HANDLE hCurrentToken;
  DWORD session = 0, len = sizeof(session);
  ::OpenProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS, &hCurrentToken);
  ::GetTokenInformation(hCurrentToken, TokenSessionId, &session, len, &len);
  ::CloseHandle(hCurrentToken);

  if (!SetPrivilege(hDupToken, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE) ||
      !SetPrivilege(hDupToken, SE_INCREASE_QUOTA_NAME, TRUE)) {
    printf("SetPrivilege failed: Insufficient privileges\n");
    return 1;
  }

  BOOL ok = ::SetTokenInformation(hPrimary, TokenSessionId, &session,
                                  sizeof(session));

  if (!::CreateProcessAsUserW(hPrimary, nullptr, pszCmdline, nullptr, nullptr,
                              FALSE, 0, nullptr, nullptr, &si, &pi)) {
    printf("Failed to create process (error=%d)\n", ::GetLastError());
    return false;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  printf("Process created: %d\n", pi.dwProcessId);
  dwProcessId = pi.dwProcessId;
  return true;
}
} // namespace priv