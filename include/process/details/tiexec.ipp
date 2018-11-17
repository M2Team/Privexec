// trusted installer
#ifndef PRIVEXEC_TRUSTEDEXEC_HPP
#define PRIVEXEC_TRUSTEDEXEC_HPP
#include "processfwd.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <WtsApi32.h>

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

bool process::tiexec() {
  if (!IsUserAdministratorsGroup()) {
    SetLastError(ERROR_ACCESS_DENIED);
    return false;
  }
  return true;
}

} // namespace priv

#endif