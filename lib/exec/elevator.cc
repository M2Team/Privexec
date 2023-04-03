///
#include <thread>
#include "execinternal.hpp"
#include <wtsapi32.h>

namespace wsudo::exec {

bool IsUserAdministratorsGreater(bela::error_code &ec) {
  HANDLE hToken{nullptr};
  auto closer = bela::finally([&] {
    if (hToken != nullptr) {
      CloseHandle(hToken);
    }
  });
  if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken) != TRUE) {
    ec = bela::make_system_error_code(L"OpenProcessToken");
    return false;
  }
  TOKEN_ELEVATION info{0};
  DWORD len = sizeof(info);
  if (::GetTokenInformation(hToken, TOKEN_INFORMATION_CLASS::TokenElevation, &info, len, &len) != TRUE) {
    ec = bela::make_system_error_code(L"GetTokenInformation");
    return false;
  }
  return info.TokenIsElevated != 0;
}

constexpr DWORD INVALID_SESSION_ID = 0xFFFFFFFF;
bool GetCurrentSessionToken(PHANDLE hNewToken, bela::error_code &ec) {
  HANDLE hImpersonationToken{nullptr};
  DWORD activeSessionId{INVALID_SESSION_ID};
  DWORD sc = 0;
  PWTS_SESSION_INFOW si{nullptr};
  auto closer = bela::finally([&] {
    if (si != nullptr) {
      WTSFreeMemory(si);
    }
    if (hImpersonationToken != nullptr) {
      CloseHandle(hImpersonationToken);
    }
  });
  if (WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &si, &sc) != 0) {
    for (DWORD i = 0; i < sc; i++) {
      if (si[i].State == WTS_CONNECTSTATE_CLASS::WTSActive) {
        activeSessionId = si[i].SessionId;
      }
    }
  }
  if (activeSessionId == INVALID_SESSION_ID) {
    activeSessionId = WTSGetActiveConsoleSessionId();
  }
  if (WTSQueryUserToken(activeSessionId, &hImpersonationToken) != TRUE) {
    ec = bela::make_system_error_code(L"WTSQueryUserToken() ");
    return false;
  }
  if (DuplicateTokenEx(hImpersonationToken, 0, nullptr, SecurityImpersonation, TokenPrimary, hNewToken) != TRUE) {
    ec = bela::make_system_error_code(L"WTSQueryUserToken() ");
    return false;
  }
  return true;
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

// taskhostw.exe

std::once_flag once_elevated;
bool _InitializeAsSystem(bela::error_code &ec) {
  if (!IsUserAdministratorsGreater(ec)) {
    ec = bela::make_error_code(L"current process not runing in administrator");
    return false;
  }
  return true;
}

bool InitializeAsSystem(bela::error_code &ec) {
  bool result{true};
  std::call_once(once_elevated, [&] { result = _InitializeAsSystem(ec); });
  return result;
}

// bela::EqualsIgnoreCase
[[maybe_unused]] constexpr std::wstring_view LsassName = L"lsass.exe";
[[maybe_unused]] constexpr std::wstring_view WinLogonName = L"winlogon.exe";

inline auto IsSystemProcessName(std::wstring_view name) {
  return bela::EqualsIgnoreCase(name, LsassName) || bela::EqualsIgnoreCase(name, WinLogonName);
}

constexpr DWORD INVALID_PROCESS_ID = 0xFFFFFFFF;
DWORD LookupSystemProcess() {
  PWTS_PROCESS_INFOW pi{nullptr};
  DWORD count{0};
  if (::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pi, &count) != TRUE) {
    return INVALID_PROCESS_ID;
  }
  auto closer = bela::finally([&] {
    if (pi != nullptr) {
      WTSFreeMemory(pi);
    }
  });
  auto end = pi + count;
  for (auto it = pi; it != end; it++) {
    if (it->SessionId == 0 && IsSystemProcessName(it->pProcessName) &&
        IsWellKnownSid(it->pUserSid, WinLocalSystemSid) == TRUE) {
      return it->ProcessId;
    }
  }
  return INVALID_PROCESS_ID;
}

// Get Current Process SessionID and Enable SeDebugPrivilege
bool EnableSeDebugPrivilege(DWORD &currentSessionId, bela::error_code &ec) {
  if (!IsUserAdministratorsGroup()) {
    ec = bela::make_error_code(1, L"current process not runing in administrator");
    return false;
  }
  auto hToken = INVALID_HANDLE_VALUE;
  DWORD Length = 0;
  TOKEN_PRIVILEGES tp;
  LUID luid;
  auto closer = bela::finally([&] { FreeToken(hToken); });
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) != TRUE) {
    ec = bela::make_system_error_code(L"EnableSeDebugPrivilege<OpenProcessToken> ");
    return false;
  }

  if (GetTokenInformation(hToken, TokenSessionId, &currentSessionId, sizeof(DWORD), &Length) != TRUE) {
    ec = bela::make_system_error_code(L"EnableSeDebugPrivilege<GetTokenInformation> ");
    return false;
  }

  if (::LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid) != TRUE) {
    ec = bela::make_system_error_code(L"EnableSeDebugPrivilege<LookupPrivilegeValueW> ");
    return false;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  // Enable the privilege or disable all privileges.

  if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"EnableSeDebugPrivilege<AdjustTokenPrivileges> ");
    return false;
  }
  if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    ec = bela::make_system_error_code(L"EnableSeDebugPrivilege<AdjustTokenPrivileges> ");
    return false;
  }
  return true;
}

bool privileges_enable_all(HANDLE hToken) {
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

bool privileges_view_enabled(HANDLE hToken, const privilege_entries *pv) {
  if (pv == nullptr) {
    return privileges_enable_all(hToken);
  }
  for (const auto lpszPrivilege : pv->entries) {
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

bool Elavator::impersonation_system_token(bela::error_code &ec) {
  HANDLE hExistingToken = INVALID_HANDLE_VALUE;
  auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, systemProcessId);
  if (hProcess == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(
        bela::StringCat(L"Elavator::impersonation_system_token<OpenProcess> ", systemProcessId, L" "));
    return false;
  }
  auto hpdeleter = bela::finally([&] { CloseHandle(hProcess); });
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken) != TRUE) {
    ec = bela::make_system_error_code(
        bela::StringCat(L"Elavator::impersonation_system_token<OpenProcessToken> ", systemProcessId, L" "));
    return false;
  }
  auto htdeleter = bela::finally([&] { CloseHandle(hExistingToken); });
  if (DuplicateTokenEx(hExistingToken, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenImpersonation, &hToken) !=
      TRUE) {
    ec = bela::make_system_error_code(L"Elavator::impersonation_system_token<DuplicateTokenEx> ");
    return false;
  }
  return true;
}

// Improve self-generated permissions
bool Elavator::ImpersonationSystemPrivilege(const privilege_entries *pv, bela::error_code &ec) {
  if (!EnableSeDebugPrivilege(currentSessionId, ec)) {
    return false;
  }
  if (systemProcessId = LookupSystemProcess(); systemProcessId == INVALID_PROCESS_ID) {
    ec = bela::make_error_code(1, L"Elevator::ImpersonationSystemPrivilege unable lookup system process pid");
    return false;
  }
  if (!impersonation_system_token(ec)) {
    return false;
  }
  if (!privileges_view_enabled(hToken, pv)) {
    ec = bela::make_error_code(1, L"Elevator::ImpersonationSystemPrivilege unable enable privileges: ",
                               pv == nullptr ? L"all" : pv->format());
    return false;
  }
  if (SetThreadToken(nullptr, hToken) != TRUE) {
    ec = bela::make_error_code(1, L"Elevator::ImpersonationSystemPrivilege<SetThreadToken> ");
    return false;
  }
  return true;
}

} // namespace wsudo::exec