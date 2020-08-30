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
  bool elevate(bela::error_code &ec);
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

bool TiElevator::elevate(bela::error_code &ec) {
  // https://docs.microsoft.com/zh-cn/windows/win32/api/winsvc/nf-winsvc-openscmanagera
  // Establishes a connection to the service control manager on the specified
  // computer and opens the specified service control manager database.
  if (hscm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT); hscm == nullptr) {
    ec = bela::make_system_error_code(L"TiElevator::elevate<OpenSCManagerW> ");
    return false;
  }
  if (hsrv = OpenServiceW(hscm, L"TrustedInstaller", SERVICE_QUERY_STATUS | SERVICE_START); hsrv == nullptr) {
    ec = bela::make_system_error_code(L"TiElevator::elevate<OpenServiceW> ");
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
bool executesystem(command &cmd, bela::error_code &ec) {
  PrivilegeView pv = {{SE_TCB_NAME, SE_ASSIGNPRIMARYTOKEN_NAME, SE_INCREASE_QUOTA_NAME}};
  Elevator eo;
  if (!eo.elevate(&pv, ec)) {
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
  if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, eo.PID())) == nullptr) {
    ec = bela::make_system_error_code(L"executesystem<OpenProcess> ");
    return false;
  }
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
    ec = bela::make_system_error_code(L"executesystem<OpenProcessToken> ");
    return false;
  }
  if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation, TokenPrimary, &hPrimary) != TRUE) {
    ec = bela::make_system_error_code(L"executesystem<DuplicateTokenEx> ");
    return false;
  }
  auto session = eo.SID();
  if (SetTokenInformation(hPrimary, TokenSessionId, &session, sizeof(session)) != TRUE) {
    ec = bela::make_system_error_code(L"executesystem<SetTokenInformation> ");
    return false;
  }
  return execute(hPrimary, true, cmd, ec);
}

bool executeti(command &cmd, bela::error_code &ec) {
  Elevator eo;
  if (!eo.elevate(nullptr, ec)) {
    return false;
  }
  TiElevator te;
  if (!te.elevate(ec)) {
    return false;
  }
  HANDLE hToken = INVALID_HANDLE_VALUE;
  if (!te.duplicate(&hToken, ec)) {
    return false;
  }
  auto closer = bela::finally([&] { FreeToken(hToken); });
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    ec = bela::make_system_error_code(L"executeti<GetCurrentSessionId> ");
    return false;
  }
  if (SetTokenInformation(hToken, TokenSessionId, (PVOID)&dwSessionId, sizeof(DWORD)) != TRUE) {
    ec = bela::make_system_error_code(L"executeti<SetTokenInformation> ");
    return false;
  }
  return execute(hToken, true, cmd, ec);
}

} // namespace wsudo::exec