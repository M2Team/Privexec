// system run
#ifndef PRIVE_SYSTEMEXEC_IPP
#define PRIVE_SYSTEMEXEC_IPP
#include "processfwd.hpp"
#include <wtsapi32.h>
#include <cstdio>
#include "systemhelper.hpp"

namespace priv {

inline HANDLE OpenSystemProcessToken() {
  DWORD pid = 0;
  if (!LookupSystemProcessID(pid)) {
    return INVALID_HANDLE_VALUE;
  }
  HANDLE hExistingToken = INVALID_HANDLE_VALUE;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
  if (hProcess == INVALID_HANDLE_VALUE) {
    return false;
  }
  ::OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken);
  ::CloseHandle(hProcess);
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

inline void CloseHandleEx(HANDLE &h) {
  if (h != INVALID_HANDLE_VALUE) {
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }
}

bool process::systemexec() {
  if (!IsUserAdministratorsGroup()) {
    SetLastError(ERROR_ACCESS_DENIED);
    return false;
  }
  if (EnableDebugPrivilege() != TRUE) {
    SetLastError(ERROR_ACCESS_DENIED);
    fprintf(stderr, "EnableDebugPrivilege failed: Access denied (are you "
                    "running elevated?)\n");
    return false;
  }
  auto hToken = OpenSystemProcessToken();
  if (hToken == INVALID_HANDLE_VALUE) {
    SetLastError(ERROR_ACCESS_DENIED);
    fprintf(stderr, "OpenSystemProcessToken failed: Access denied (are you "
                    "running elevated?)\n");
    return false;
  }

  HANDLE hDupToken = INVALID_HANDLE_VALUE;
  HANDLE hPrimary = INVALID_HANDLE_VALUE;
  auto deleter = finally([&] {
    CloseHandleEx(hDupToken);
    CloseHandleEx(hPrimary);
    CloseHandleEx(hToken);
  });
  if (DuplicateTokenEx(hToken,
                       TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY |
                           TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_PRIVILEGES,
                       nullptr, SecurityImpersonation, TokenImpersonation,
                       &hDupToken)) {
    return false;
  }
  if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation,
                       TokenPrimary, &hPrimary) != TRUE) {
    return false;
  }
  if (SetThreadToken(nullptr, hDupToken) != TRUE) {
    return false;
  }

  HANDLE hCurrentToken = INVALID_HANDLE_VALUE;
  DWORD session = 0, len = sizeof(session);
  ::OpenProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS, &hCurrentToken);
  ::GetTokenInformation(hCurrentToken, TokenSessionId, &session, len, &len);
  ::CloseHandle(hCurrentToken);

  if (SetPrivilege(hDupToken, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE) != TRUE ||
      SetPrivilege(hDupToken, SE_INCREASE_QUOTA_NAME, TRUE) != TRUE) {
    fprintf(stderr, "SetPrivilege failed: Insufficient privileges\n");
    return false;
  }

  if (SetTokenInformation(hPrimary, TokenSessionId, &session,
                          sizeof(session)) != TRUE) {
    return false;
  }
  return execwithtoken(hPrimary, true);
}
} // namespace priv

#endif