///
#include "process_fwd.hpp"
#include "elevator.hpp"
#include <wtsapi32.h>
#include <bela/base.hpp>

namespace priv {
// bela::EqualsIgnoreCase
[[maybe_unused]] constexpr std::wstring_view WinLogonName = L"winlogon.exe";
namespace system_internal {
inline void safeclosesch(SC_HANDLE sch) {
  if (sch != nullptr) {
    CloseServiceHandle(sch);
  }
}
// TrustInstaller Elevator
class TiElevator {
public:
  TiElevator() = default;
  TiElevator(const TiElevator &) = delete;
  TiElevator &operator=(const TiElevator &) = delete;
  ~TiElevator();
  bool Elevate();
  bool Duplicate(PHANDLE phToken);

private:
  SC_HANDLE hscm{nullptr};
  SC_HANDLE hsrv{nullptr};
  HANDLE hProcess{INVALID_HANDLE_VALUE};
  HANDLE hToken{INVALID_HANDLE_VALUE};
  SERVICE_STATUS_PROCESS ssp;
};
//

TiElevator::~TiElevator() {
  safeclosesch(hscm);
  safeclosesch(hsrv);
  CloseHandleEx(hToken);
  CloseHandleEx(hProcess);
}

bool TiElevator::Elevate() {
  // https://docs.microsoft.com/zh-cn/windows/win32/api/winsvc/nf-winsvc-openscmanagera
  // Establishes a connection to the service control manager on the specified
  // computer and opens the specified service control manager database.
  hscm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (hscm == nullptr) {
    return false;
  }
  hsrv = OpenServiceW(hscm, L"TrustedInstaller",
                      SERVICE_QUERY_STATUS | SERVICE_START);
  if (hsrv == nullptr) {
    // unable start TrustedInstaller service
    return false;
  }
  DWORD dwNeed = 0;
  DWORD nOldCheckPoint = 0;
  ULONGLONG tick = 0;
  ULONGLONG LastTick = 0;
  bool sleeped = false;
  bool startcallonce = false;
  for (;;) {
    if (!QueryServiceStatusEx(hsrv, SC_STATUS_PROCESS_INFO,
                              reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp),
                              &dwNeed)) {
      return false;
    }
    if (ssp.dwCurrentState == SERVICE_STOPPED) {
      if (startcallonce) {
        continue;
      }
      if (StartServiceW(hsrv, 0, nullptr) == TRUE) {
        startcallonce = true;
      }
      continue;
    }
    if (ssp.dwCurrentState != SERVICE_STOP_PENDING &&
        ssp.dwCurrentState != SERVICE_START_PENDING) {
      return true;
    }
    tick = GetTickCount64();
    if (sleeped) {
      if (ssp.dwCheckPoint > nOldCheckPoint) {
        sleeped = false;
        continue;
      }
      ULONGLONG nDiff = tick - LastTick;
      if (nDiff > ssp.dwWaitHint) {
        SetLastError(ERROR_TIMEOUT);
        return false;
      }
      sleeped = false;
      continue;
    }
    LastTick = tick;
    nOldCheckPoint = ssp.dwCheckPoint;
    sleeped = true;
    SleepEx(250, FALSE);
  }
  return false;
}

bool TiElevator::Duplicate(PHANDLE phToken) {
  hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, ssp.dwProcessId);
  if (hProcess == INVALID_HANDLE_VALUE) {
    return false;
  }
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
    return false;
  }
  return DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, nullptr,
                          SecurityIdentification, TokenPrimary,
                          phToken) == TRUE;
}

} // namespace system_internal

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
  if (GetTokenInformation(hToken, TokenPrivileges, privs, Length, &Length) !=
      TRUE) {
    return false;
  }
  auto end = privs->Privileges + privs->PrivilegeCount;
  for (auto it = privs->Privileges; it != end; it++) {
    it->Attributes = SE_PRIVILEGE_ENABLED;
  }
  return (AdjustTokenPrivileges(hToken, FALSE, privs, 0, nullptr, nullptr) ==
          TRUE);
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

    if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                                nullptr, nullptr) != TRUE) {
      continue;
    }
    if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
      return false;
    }
  }
  return true;
}

bool GetCurrentSessionId(DWORD &dwSessionId) {
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
    return false;
  }
  DWORD Length = 0;
  if (GetTokenInformation(hToken, TokenSessionId, &dwSessionId, sizeof(DWORD),
                          &Length) != TRUE) {
    CloseHandle(hToken);
    return false;
  }
  CloseHandle(hToken);
  return true;
}

bool LookupSystemProcess(DWORD sid, DWORD &syspid) {
  PWTS_PROCESS_INFOW ppi;
  DWORD count;
  if (::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppi, &count) !=
      TRUE) {
    return false;
  }
  auto end = ppi + count;
  for (auto it = ppi; it != end; it++) {
    if (it->SessionId == sid &&
        bela::EqualsIgnoreCase(WinLogonName, it->pProcessName) &&
        IsWellKnownSid(it->pUserSid, WinLocalSystemSid) == TRUE) {
      syspid = it->ProcessId;
      ::WTSFreeMemory(ppi);
      return true;
    }
  }
  ::WTSFreeMemory(ppi);
  return false;
}

// Get Current Process SessionID and Enable SeDebugPrivilege
bool PrepareElevate(DWORD &sid, std::wstring &klast) {
  if (!IsUserAdministratorsGroup()) {
    klast = L"not runing in administrator or group";
    return false;
  }
  auto hToken = INVALID_HANDLE_VALUE;
  constexpr DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
  if (OpenProcessToken(GetCurrentProcess(), flags, &hToken) != TRUE) {
    klast = L"unable open current process token";
    return false;
  }
  auto closer = bela::finally([&] { CloseHandle(hToken); });
  DWORD Length = 0;
  if (GetTokenInformation(hToken, TokenSessionId, &sid, sizeof(DWORD),
                          &Length) != TRUE) {
    klast = L"unable resolve current process session id";
    return false;
  }
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (::LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid) != TRUE) {
    klast = L"unable lookup SeDebugPrivilege";
    return false;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  // Enable the privilege or disable all privileges.

  if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                              nullptr, nullptr) != TRUE) {
    klast = L"unable enable process SeDebugPrivilege";
    return false;
  }
  if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    return false;
  }
  return true;
}

bool Elevator::ElevateImitate(std::wstring &klast) {
  HANDLE hExistingToken = INVALID_HANDLE_VALUE;
  auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
  if (hProcess == INVALID_HANDLE_VALUE) {
    klast = L"unable open winlogon.exe process";
    return false;
  }
  auto hpdeleter = bela::finally([&] { CloseHandle(hProcess); });
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken) != TRUE) {
    klast = L"unable open winlogon.exe process token";
    return false;
  }
  auto htdeleter = bela::finally([&] { CloseHandle(hExistingToken); });
  if (DuplicateTokenEx(hExistingToken, MAXIMUM_ALLOWED, nullptr,
                       SecurityImpersonation, TokenImpersonation,
                       &hToken) != TRUE) {
    klast = L"unable duplicate winlogon.exe process token";
    return false;
  }
  return true;
}

// Improve self-generated permissions
bool Elevator::Elevate(const PrivilegeView *pv, std::wstring &klast) {
  if (!PrepareElevate(sid, klast)) {
    return false;
  }
  if (!LookupSystemProcess(sid, pid)) {
    klast = L"unable lookup winlogon process pid";
    return false;
  }
  if (!ElevateImitate(klast)) {
    return false;
  }
  if (!PrivilegesEnableView(hToken, pv)) {
    klast = L"unable enable privileges: ";
    if (pv != nullptr) {
      klast.append(pv->dump());
    } else {
      klast.append(L"all");
    }
    return false;
  }
  if (SetThreadToken(nullptr, hToken) != TRUE) {
    klast = L"unable set current thread token to imitated system token";
    return false;
  }
  return true;
}

bool Process::ExecSystem() {
  PrivilegeView pv = {
      {SE_TCB_NAME, SE_ASSIGNPRIMARYTOKEN_NAME, SE_INCREASE_QUOTA_NAME}};
  Elevator eo;
  if (!eo.Elevate(&pv, kmessage)) {
    return false;
  }
  auto hProcess = INVALID_HANDLE_VALUE;
  auto hToken = INVALID_HANDLE_VALUE;
  auto hPrimary = INVALID_HANDLE_VALUE;
  auto deleter = bela::finally([&] {
    CloseHandleEx(hProcess);
    CloseHandleEx(hToken);
    CloseHandleEx(hPrimary);
  });
  if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, eo.PID())) == nullptr) {
    kmessage = L"systemexec<OpenProcess>";
    return false;
  }
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
    kmessage = L"systemexec<OpenProcessToken>";
    return false;
  }
  if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation,
                       TokenPrimary, &hPrimary) != TRUE) {
    kmessage = L"systemexec<DuplicateTokenEx>";
    return false;
  }
  auto session = eo.SID();
  if (SetTokenInformation(hPrimary, TokenSessionId, &session,
                          sizeof(session)) != TRUE) {
    return false;
  }
  return ExecWithToken(hPrimary, true);
}

bool Process::ExecTI() {
  Elevator eo;
  if (!eo.Elevate(nullptr, kmessage)) {
    return false;
  }
  system_internal::TiElevator te;
  if (!te.Elevate()) {
    return false;
  }
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!te.Duplicate(&hToken)) {
    return false;
  }
  auto deleter = bela::finally([hToken] { CloseHandle(hToken); });
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    return false;
  }
  if (SetTokenInformation(hToken, TokenSessionId, (PVOID)&dwSessionId,
                          sizeof(DWORD)) != TRUE) {
    return false;
  }
  return ExecWithToken(hToken, true);
}
} // namespace priv
