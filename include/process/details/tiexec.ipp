// trusted installer
#ifndef PRIVEXEC_TRUSTEDEXEC_HPP
#define PRIVEXEC_TRUSTEDEXEC_HPP
#include "processfwd.hpp"
#include "systemhelper.hpp"

namespace priv {
class TrustedInstallerProxy {
public:
  TrustedInstallerProxy() = default;
  TrustedInstallerProxy(const TrustedInstallerProxy &) = delete;
  TrustedInstallerProxy &operator=(const TrustedInstallerProxy &) = delete;
  ~TrustedInstallerProxy() {
    if (hSCM != nullptr) {
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
                                reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp),
                                &dwNeed)) {
      switch (ssp.dwCurrentState) {
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
          nOldCheckPoint = ssp.dwCheckPoint;

          sleeped = true;

          SleepEx(250, FALSE);
        } else {
          if (ssp.dwCheckPoint > nOldCheckPoint) {
            sleeped = false;
          } else {
            ULONGLONG nDiff = tick - LastTick;
            if (nDiff > ssp.dwWaitHint) {
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
    hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, ssp.dwProcessId);
    if (hProcess == nullptr) {
      return false;
    }
    if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
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
  SERVICE_STATUS_PROCESS ssp;
};

inline bool EnableAllTokenPrivileges(_In_ HANDLE hExistingToken,
                                     _In_ bool bEnable) {
  PTOKEN_PRIVILEGES privs = nullptr;
  DWORD Length = 0;
  GetTokenInformation(hExistingToken, TokenPrivileges, nullptr, 0, &Length);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return false;
  }
  auto privs = (PTOKEN_PRIVILEGES)HeapAlloc(GetProcessHeap(), 0, Length);
  if (privs == nullptr) {
    return false;
  }
  auto pfree = finally([privs] { HeapFree(GetProcessHeap(), 0, privs); });
  if (GetTokenInformation(hExistingToken, TokenPrivileges, privs, Length,
                          &Length) != TRUE) {
    return false;
  }
  auto end = privs->Privileges + privs->PrivilegeCount;
  for (auto it = privs->Privileges; it != end; it++) {
    it->Attributes = (DWORD)(bEnable ? SE_PRIVILEGE_ENABLED : 0);
  }
  return (AdjustTokenPrivileges(hExistingToken, FALSE, privs, 0, nullptr,
                                nullptr) == TRUE);
}

inline bool
DuplicateProcessTokenEx(_In_ DWORD dwProcessID, _In_ DWORD dwDesiredAccess,
                        _In_opt_ LPSECURITY_ATTRIBUTES lpTokenAttributes,
                        _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                        _In_ TOKEN_TYPE TokenType, _Outptr_ PHANDLE phToken) {
  HANDLE hProcess = nullptr;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwProcessID)) ==
      nullptr) {
    return false;
  }
  if (!OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken)) {
    CloseHandle(hProcess);
    return false;
  }
  auto result = DuplicateTokenEx(hToken, dwDesiredAccess, lpTokenAttributes,
                                 ImpersonationLevel, TokenType, phToken);
  CloseHandle(hProcess);
  CloseHandle(hToken);
  return (result == TRUE);
}

inline bool ImpersonateSystem() {
  DWORD pid = 0;
  if (!LookupSystemProcessID(pid)) {
    return false;
  }
  HANDLE hToken = nullptr;
  if (!DuplicateProcessTokenEx(pid, MAXIMUM_ALLOWED, nullptr,
                               SecurityImpersonation, TokenImpersonation,
                               &hToken)) {
    return false;
  }
  if (EnableAllTokenPrivileges(hToken, true)) {
    auto result = SetThreadToken(nullptr, hToken);
    CloseHandle(hToken);
    return result == TRUE;
  }
  return false;
}

bool process::tiexec() {
  if (!IsUserAdministratorsGroup()) {
    SetLastError(ERROR_ACCESS_DENIED);
    return false;
  }
  TrustedInstallerProxy tip;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!tip.DuplicateTiToken(&hToken)) {
    return false;
  }
  auto deleter = finally([hToken] { CloseHandle(hToken); });
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    return false;
  }
  if (SetTokenInformation(hToken, TokenSessionId, (PVOID)&dwSessionId,
                          sizeof(DWORD)) != TRUE) {
    return false;
  }
  return execwithtoken(hToken, true);
}

} // namespace priv

#endif