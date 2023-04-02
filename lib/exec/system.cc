//
#include "execinternal.hpp"

namespace wsudo::exec {
inline void FreeSrv(SC_HANDLE &sch) {
  if (sch != nullptr) {
    CloseServiceHandle(sch);
    sch = nullptr;
  }
}

class TiElevator {
public:
  TiElevator() = default;
  TiElevator(const TiElevator &) = delete;
  TiElevator &operator=(const TiElevator &) = delete;
  ~TiElevator();
  bool Elevate(bela::error_code &ec);
  bool duplicate(PHANDLE phToken, bela::error_code &ec);

private:
  SC_HANDLE hscm{nullptr};
  SC_HANDLE hsrv{nullptr};
  HANDLE hProcess{INVALID_HANDLE_VALUE};
  HANDLE hToken{INVALID_HANDLE_VALUE};
  SERVICE_STATUS_PROCESS ssp;
};

TiElevator::~TiElevator() {
  FreeSrv(hscm);
  FreeSrv(hsrv);
  FreeToken(hToken);
  FreeToken(hProcess);
}

bool TiElevator::Elevate(bela::error_code &ec) {
  // https://docs.microsoft.com/zh-cn/windows/win32/api/winsvc/nf-winsvc-openscmanagera
  // Establishes a connection to the service control manager on the specified
  // computer and opens the specified service control manager database.
  if (hscm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT); hscm == nullptr) {
    ec = bela::make_system_error_code(L"TiElevator::Elevate<OpenSCManagerW> ");
    return false;
  }
  if (hsrv = OpenServiceW(hscm, L"TrustedInstaller", SERVICE_QUERY_STATUS | SERVICE_START); hsrv == nullptr) {
    ec = bela::make_system_error_code(L"TiElevator::Elevate<OpenServiceW> ");
    return false;
  }
  DWORD dwNeed = 0;
  DWORD nOldCheckPoint = 0;
  ULONGLONG tick = 0;
  ULONGLONG LastTick = 0;
  bool sleeped = false;
  bool startcallonce = false;
  for (;;) {
    if (QueryServiceStatusEx(hsrv, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &dwNeed) !=
        TRUE) {
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
    if (ssp.dwCurrentState != SERVICE_STOP_PENDING && ssp.dwCurrentState != SERVICE_START_PENDING) {
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
  ec = bela::make_system_error_code(L"start trustedinstaller service ");
  return false;
}

bool TiElevator::duplicate(PHANDLE phToken, bela::error_code &ec) {
  if (hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, ssp.dwProcessId); hProcess == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"TiElevator::duplicate<OpenProcess> ");
    return false;
  }
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
    ec = bela::make_system_error_code(L"TiElevator::duplicate<OpenProcessToken> ");
    return false;
  }
  if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, nullptr, SecurityIdentification, TokenPrimary, phToken) != TRUE) {
    ec = bela::make_system_error_code(L"TiElevator::duplicate<DuplicateTokenEx> ");
    return false;
  }
  return true;
}

//
bool execute_with_system(command &cmd, bela::error_code &ec) {
  privilege_entries pv(SE_TCB_NAME, SE_ASSIGNPRIMARYTOKEN_NAME, SE_INCREASE_QUOTA_NAME);
  Elavator eo;
  if (!eo.ImpersonationSystemPrivilege(&pv, ec)) {
    return false;
  }
  auto hProcess{INVALID_HANDLE_VALUE};
  auto hToken{INVALID_HANDLE_VALUE};
  auto hPrimary{INVALID_HANDLE_VALUE};
  auto deleter = bela::finally([&] {
    FreeToken(hProcess);
    FreeToken(hToken);
    FreeToken(hPrimary);
  });

  auto impersonation_system_token_all = [&]() -> bool {
    for (auto pid : eo.SystemProcesses()) {
      if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid)) == nullptr) {
        ec = bela::make_system_error_code(L"execute_with_system<OpenProcess> ");
        continue;
      }
      if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
        ec = bela::make_system_error_code(L"execute_with_system<OpenProcessToken> ");
        continue;
      }
      if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation, TokenPrimary, &hPrimary) != TRUE) {
        ec = bela::make_system_error_code(L"execute_with_system<DuplicateTokenEx> ");
        continue;
      }
      return true;
    }
    return false;
  };
  if (!impersonation_system_token_all()) {
    return false;
  }
  auto session = eo.SessionID();
  if (SetTokenInformation(hPrimary, TokenSessionId, &session, sizeof(session)) != TRUE) {
    ec = bela::make_system_error_code(L"execute_with_system<SetTokenInformation> ");
    return false;
  }
  return execute_with_token(cmd, hPrimary, true, nullptr, ec);
}

bool execute_with_ti(command &cmd, bela::error_code &ec) {
  Elavator eo;
  if (!eo.ImpersonationSystemPrivilege(nullptr, ec)) {
    return false;
  }
  TiElevator te;
  if (!te.Elevate(ec)) {
    return false;
  }
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!te.duplicate(&hToken, ec)) {
    return false;
  }
  auto closer = bela::finally([&] { FreeToken(hToken); });
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    ec = bela::make_system_error_code(L"execute_with_ti<GetCurrentSessionId> ");
    return false;
  }
  if (SetTokenInformation(hToken, TokenSessionId, (PVOID)&dwSessionId, sizeof(DWORD)) != TRUE) {
    ec = bela::make_system_error_code(L"execute_with_ti<SetTokenInformation> ");
    return false;
  }
  return execute_with_token(cmd, hToken, true, nullptr, ec);
}

} // namespace wsudo::exec