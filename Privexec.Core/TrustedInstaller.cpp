#include "stdafx.h"
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
	if (hToken!= INVALID_HANDLE_VALUE) {
		CloseHandle(hToken);
	}
	if (hProcess!= INVALID_HANDLE_VALUE) {
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
		  return false;
	  }
	  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
		  return false;
	  }
	  return DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, nullptr, SecurityIdentification, TokenPrimary, phToken) == TRUE;
  }
private:
  SC_HANDLE hSCM{nullptr};
  SC_HANDLE hService{nullptr};
  HANDLE hProcess{ INVALID_HANDLE_VALUE };
  HANDLE hToken{ INVALID_HANDLE_VALUE };
  SERVICE_STATUS_PROCESS ssr;
};

bool CreateTiProcess(LPWSTR pszCmdline, DWORD &dwProcessId) {
  if (!IsUserAdministratorsGroup()) {
    SetLastError(ERROR_ACCESS_DENIED);
    return false;
  }
  TrustedInstallerProxy tip;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!tip.DuplicateTiToken(&hToken)) {
	  return false;
  }
  return true;
}