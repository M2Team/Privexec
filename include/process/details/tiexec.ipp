// trusted installer
#ifndef PRIVEXEC_TRUSTEDEXEC_HPP
#define PRIVEXEC_TRUSTEDEXEC_HPP
#include "processfwd.hpp"
#include "systemhelper.hpp"

namespace priv {

inline void SafeCloseSO(SC_HANDLE hSCObject) {
  if (hSCObject != nullptr) {
    CloseServiceHandle(hSCObject);
  }
}

class TiImpersonator {
public:
  TiImpersonator() = default;
  TiImpersonator(const TiImpersonator &) = delete;
  TiImpersonator &operator=(const TiImpersonator &) = delete;
  ~TiImpersonator() {
    SafeCloseSO(hSCM);
    SafeCloseSO(hService);
    CloseHandleEx(hToken);
    CloseHandleEx(hProcess);
  }
  bool Initialize() {
    if ((hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT)) ==
        nullptr) {
      return false;
    }
    if ((hService = OpenServiceW(hSCM, L"TrustedInstaller",
                                 SERVICE_QUERY_STATUS | SERVICE_START)) ==
        nullptr) {
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
    if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, ssp.dwProcessId)) ==
        nullptr) {
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

inline bool process::tiexec() {
  SysImpersonator impersonator;
  if (!impersonator.PreImpersonation()) {
    return false;
  }
  if (!impersonator.Impersonation(nullptr)) {
    return false;
  }
  TiImpersonator tip;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!tip.DuplicateTiToken(&hToken)) {
    return false;
  }
  auto deleter = base::finally([hToken] { CloseHandle(hToken); });
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