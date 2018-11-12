#include "stdafx.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <WtsApi32.h>
#include "Privexec.Core.hpp"

class TrustedInstallerProxy {
public:
  ~TrustedInstallerProxy() {
    if (hSCM) {
      CloseServiceHandle(hSCM);
    }
    if (hService) {
      CloseServiceHandle(hService);
    }
    if (hToken != INVALID_HANDLE_VALUE) {
      CloseHandle(hToken);
    }
    if (hProcess != INVALID_HANDLE_VALUE) {
      CloseHandle(hProcess);
    }
  }
  bool Initialize() {
    hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (hSCM == nullptr) {
      return false;
    }
    hService = OpenServiceW(hSCM, L"TrustedInstaller",
                            SERVICE_QUERY_STATUS | SERVICE_START);
    if (hService == nullptr) {
      return false;
    }
    DWORD dwNeed;
    DWORD nOldCheckPoint;
    ULONGLONG tick;
    ULONGLONG LastTick;
    bool sleeped = false;
    bool startsrvcall = false;
    while (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
                                reinterpret_cast<LPBYTE>(&ssr), sizeof(ssr),
                                &dwNeed)) {
      switch (ssr.dwCurrentState) {
      case SERVICE_STOPPED: {
        if (!startsrvcall) {
          if (StartServiceW(hService, 0, nullptr)) {
            startsrvcall = true;
          }
        }
      } break;
      case SERVICE_STOP_PENDING:
      case SERVICE_START_PENDING: {
        tick = GetTickCount64();
        if (!sleeped) {
          LastTick = tick;
          nOldCheckPoint = ssr.dwCheckPoint;

          sleeped = true;

          SleepEx(250, FALSE);
        } else {
          if (ssr.dwCheckPoint > nOldCheckPoint) {
            sleeped = false;
          } else {
            ULONGLONG nDiff = tick - LastTick;
            if (nDiff > ssr.dwWaitHint) {
              SetLastError(ERROR_TIMEOUT);
              return false;
            }
            sleeped = false;
          }
        }
      } break;
      default:
        return true;
      }
    }
    return false;
  }
  bool DuplicateTiToken(PHANDLE phToken) {
    if (!Initialize()) {
      return false;
    }
    hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, ssr.dwProcessId);
    if (hProcess == nullptr) {
      printf("%d", __LINE__);
      return false;
    }
    if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
      printf("%d", __LINE__);
      return false;
    }
    return DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, nullptr,
                            SecurityIdentification, TokenPrimary,
                            phToken) == TRUE;
  }

private:
  SC_HANDLE hSCM{nullptr};
  SC_HANDLE hService{nullptr};
  HANDLE hProcess{INVALID_HANDLE_VALUE};
  HANDLE hToken{INVALID_HANDLE_VALUE};
  SERVICE_STATUS_PROCESS ssr;
};

namespace priv {
bool GetCurrentSessionId(DWORD &dwSessionId) {
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
    return false;
  }
  DWORD Length = 0;
  if (!GetTokenInformation(hToken, TokenSessionId, &dwSessionId, sizeof(DWORD),
                           &Length)) {
    CloseHandle(hToken);
    return false;
  }
  CloseHandle(hToken);
  return true;
}

BOOL WINAPI NSudoSetTokenAllPrivileges(_In_ HANDLE hExistingToken,
                                       _In_ bool bEnable) {
  BOOL result = FALSE;
  PTOKEN_PRIVILEGES privs = nullptr;
  DWORD Length = 0;
  GetTokenInformation(hExistingToken, TokenPrivileges, nullptr, 0, &Length);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return false;
  }
  privs = (PTOKEN_PRIVILEGES)malloc(Length);
  assert(privs);
  result = (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
  result = GetTokenInformation(hExistingToken, TokenPrivileges, privs, Length,
                               &Length);
  if (result) {
    for (DWORD i = 0; i < privs->PrivilegeCount; i++) {
      privs->Privileges[i].Attributes =
          (DWORD)(bEnable ? SE_PRIVILEGE_ENABLED : 0);
    }
    AdjustTokenPrivileges(hExistingToken, FALSE, privs, 0, nullptr, nullptr);
    result = (GetLastError() == ERROR_SUCCESS);
  }
  free(privs);
  return result;
}

BOOL WINAPI NSudoDuplicateProcessToken(
    _In_ DWORD dwProcessID, _In_ DWORD dwDesiredAccess,
    _In_opt_ LPSECURITY_ATTRIBUTES lpTokenAttributes,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_ TOKEN_TYPE TokenType, _Outptr_ PHANDLE phToken) {
  HANDLE hProcess = nullptr;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcessID)) ==
      nullptr) {
    return FALSE;
  }
  if (!OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken)) {
    CloseHandle(hProcess);
    return FALSE;
  }
  auto result = DuplicateTokenEx(hToken, dwDesiredAccess, lpTokenAttributes,
                                 ImpersonationLevel, TokenType, phToken);
  CloseHandle(hProcess);
  CloseHandle(hToken);
  return result;
}

BOOL WINAPI
NSudoDuplicateSystemToken(_In_ DWORD dwDesiredAccess,
                          _In_opt_ LPSECURITY_ATTRIBUTES lpTokenAttributes,
                          _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                          _In_ TOKEN_TYPE TokenType, _Outptr_ PHANDLE phToken) {
  DWORD dwWinLogonPID = (DWORD)-1;
  DWORD dwSessionID = (DWORD)-1;
  if (!GetCurrentSessionId(dwSessionID)) {
    return false;
  }
  PWTS_PROCESS_INFOW pProcesses = nullptr;
  DWORD dwProcessCount = 0;

  if (WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcesses,
                             &dwProcessCount)) {
    for (DWORD i = 0; i < dwProcessCount; ++i) {
      PWTS_PROCESS_INFOW pProcess = &pProcesses[i];
      if (pProcess->SessionId != dwSessionID)
        continue;
      if (pProcess->pProcessName == nullptr)
        continue;
      if (wcscmp(L"winlogon.exe", pProcess->pProcessName) == 0) {
        dwWinLogonPID = pProcess->ProcessId;
        break;
      }
    }
    WTSFreeMemory(pProcesses);
  }

  if (dwWinLogonPID == -1) {
    SetLastError(ERROR_NOT_FOUND);
    return false;
  }
  return NSudoDuplicateProcessToken(dwWinLogonPID, dwDesiredAccess,
                                    lpTokenAttributes, ImpersonationLevel,
                                    TokenType, phToken);
}

BOOL WINAPI NSudoImpersonateAsSystem() {
  BOOL result = FALSE;
  HANDLE hToken = nullptr;
  if (!NSudoDuplicateSystemToken(MAXIMUM_ALLOWED, nullptr,
                                 SecurityImpersonation, TokenImpersonation,
                                 &hToken)) {
    return false;
  }
  if (NSudoSetTokenAllPrivileges(hToken, true)) {
    auto result = SetThreadToken(nullptr, hToken);
    CloseHandle(hToken);
    return result;
  }
  return false;
}

bool CreateTiProcess(LPWSTR pszCmdline, DWORD &dwProcessId,
                     LPWSTR lpCurrentDirectory) {
  if (!IsUserAdministratorsGroup()) {
    SetLastError(ERROR_ACCESS_DENIED);
    return false;
  }
  if (!NSudoImpersonateAsSystem()) {
    printf("%d", __LINE__);
    return false;
  }
  TrustedInstallerProxy tip;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!tip.DuplicateTiToken(&hToken)) {
    printf("%d", __LINE__);
    return false;
  }
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    CloseHandle(hToken);
    return false;
  }
  SetTokenInformation(hToken, TokenSessionId, (PVOID)&dwSessionId,
                      sizeof(DWORD));
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  si.lpDesktop = L"WinSta0\\Default";
  auto result =
      CreateProcessAsUserW(hToken, nullptr, pszCmdline, nullptr, nullptr, FALSE,
                           0, nullptr, lpCurrentDirectory, &si, &pi);
  if (result) {
    dwProcessId = pi.dwProcessId;
    CloseHandle(hToken);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
  }
  CloseHandle(hToken);
  return false;
}
} // namespace priv