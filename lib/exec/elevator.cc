///
#include "execinternal.hpp"
#include <wtsapi32.h>

namespace wsudo::exec {
// bela::EqualsIgnoreCase
[[maybe_unused]] constexpr std::wstring_view WinLogonName = L"winlogon.exe";
// Get Current Process SessionID and Enable SeDebugPrivilege
bool PrepareElevate(DWORD &sid, bela::error_code &ec) {
  if (!IsUserAdministratorsGroup()) {
    ec = bela::make_error_code(1, L"current process not runing in administrator");
    return false;
  }
  auto hToken = INVALID_HANDLE_VALUE;
  constexpr DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
  if (OpenProcessToken(GetCurrentProcess(), flags, &hToken) != TRUE) {
    ec = bela::make_system_error_code(L"PrepareElevate<OpenProcessToken> ");
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(hToken); });
  DWORD Length = 0;
  if (GetTokenInformation(hToken, TokenSessionId, &sid, sizeof(DWORD), &Length) != TRUE) {
    ec = bela::make_system_error_code(L"PrepareElevate<GetTokenInformation> ");
    return false;
  }
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (::LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid) != TRUE) {

    ec = bela::make_system_error_code(L"PrepareElevate<LookupPrivilegeValueW> ");
    return false;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  // Enable the privilege or disable all privileges.

  if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"PrepareElevate<AdjustTokenPrivileges> ");
    return false;
  }
  if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    ec = bela::make_system_error_code(L"PrepareElevate<AdjustTokenPrivileges> ");
    return false;
  }
  return true;
}

bool PrivilegesEnableAll(HANDLE hToken) {
  if (hToken == INVALID_HANDLE_VALUE) {
    return false;
  }
  DWORD Length = 0;
  GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &Length);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return false;
  }
  auto privs = (PTOKEN_PRIVILEGES)HeapAlloc(GetProcessHeap(), 0, Length);
  if (privs == nullptr) {
    return false;
  }
  auto pfree = bela::finally([&] { HeapFree(GetProcessHeap(), 0, privs); });
  if (GetTokenInformation(hToken, TokenPrivileges, privs, Length, &Length) != TRUE) {
    return false;
  }
  auto end = privs->Privileges + privs->PrivilegeCount;
  for (auto it = privs->Privileges; it != end; it++) {
    it->Attributes = SE_PRIVILEGE_ENABLED;
  }
  return (AdjustTokenPrivileges(hToken, FALSE, privs, 0, nullptr, nullptr) == TRUE);
}

bool PrivilegesEnableView(HANDLE hToken, const PrivilegeView *pv) {
  if (pv == nullptr) {
    return PrivilegesEnableAll(hToken);
  }
  for (const auto lpszPrivilege : pv->privis) {
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (::LookupPrivilegeValueW(nullptr, lpszPrivilege, &luid) != TRUE) {
      continue;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    // Enable the privilege or disable all privileges.

    if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != TRUE) {
      continue;
    }
    if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
      return false;
    }
  }
  return true;
}

bool Elevator::elevateimitate(bela::error_code &ec) {
  HANDLE hExistingToken = INVALID_HANDLE_VALUE;
  auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
  if (hProcess == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"Elevator::elevateimitate<OpenProcess> ");
    return false;
  }
  auto hpdeleter = bela::finally([&] { CloseHandle(hProcess); });
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken) != TRUE) {
    ec = bela::make_system_error_code(L"Elevator::elevateimitate<OpenProcessToken> ");
    return false;
  }
  auto htdeleter = bela::finally([&] { CloseHandle(hExistingToken); });
  if (DuplicateTokenEx(hExistingToken, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenImpersonation, &hToken) !=
      TRUE) {
    ec = bela::make_system_error_code(L"Elevator::elevateimitate<DuplicateTokenEx> ");
    return false;
  }
  return true;
}

bool LookupSystemProcess(DWORD sid, DWORD &syspid) {
  PWTS_PROCESS_INFOW ppi;
  DWORD count;
  if (::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppi, &count) != TRUE) {
    return false;
  }
  auto end = ppi + count;
  for (auto it = ppi; it != end; it++) {
    if (it->SessionId == sid && bela::EqualsIgnoreCase(WinLogonName, it->pProcessName) &&
        IsWellKnownSid(it->pUserSid, WinLocalSystemSid) == TRUE) {
      syspid = it->ProcessId;
      ::WTSFreeMemory(ppi);
      return true;
    }
  }
  ::WTSFreeMemory(ppi);
  return false;
}

bool GetCurrentSessionId(DWORD &dwSessionId) {
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
    return false;
  }
  DWORD Length = 0;
  if (GetTokenInformation(hToken, TokenSessionId, &dwSessionId, sizeof(DWORD), &Length) != TRUE) {
    CloseHandle(hToken);
    return false;
  }
  CloseHandle(hToken);
  return true;
}

// Improve self-generated permissions
bool Elevator::elevate(const PrivilegeView *pv, bela::error_code &ec) {
  if (!PrepareElevate(sid, ec)) {
    return false;
  }
  if (!LookupSystemProcess(sid, pid)) {
    ec = bela::make_error_code(1, L"Elevator::elevate unable lookup winlogon process pid");
    return false;
  }
  if (!elevateimitate(ec)) {
    return false;
  }
  if (!PrivilegesEnableView(hToken, pv)) {
    ec = bela::make_error_code(1, L"Elevator::elevate unable enable privileges: ", pv == nullptr ? L"all" : pv->dump());
    return false;
  }
  if (SetThreadToken(nullptr, hToken) != TRUE) {
    ec = bela::make_error_code(1, L"Elevator::elevate<SetThreadToken> ");
    return false;
  }
  return true;
}

} // namespace wsudo::exec