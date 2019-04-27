///////////
#ifndef PRIVEXEC_SYSTEMTOKEN_HPP
#define PRIVEXEC_SYSTEMTOKEN_HPP
#include "processfwd.hpp"
#include <wtsapi32.h>
#include <cstdio>
#include <vector>
#include <string_view>

namespace priv {
struct PrivilegeView {
  std::vector<const wchar_t *> privis;
};

inline bool GetCurrentSessionId(DWORD &dwSessionId) {
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

inline bool LookupSystemProcessID(DWORD &pid) {
  PWTS_PROCESS_INFOW ppi;
  DWORD count;
  DWORD dwSessionId = 0;
  if (!GetCurrentSessionId(dwSessionId)) {
    return false;
  }
  if (::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppi, &count) !=
      TRUE) {
    return false;
  }
  HANDLE hExistingToken = nullptr;
  auto end = ppi + count;
  for (auto it = ppi; it != end; it++) {
    if (it->SessionId == dwSessionId &&
        _wcsicmp(L"winlogon.exe", it->pProcessName) == 0 &&
        IsWellKnownSid(it->pUserSid, WinLocalSystemSid) == TRUE) {
      pid = it->ProcessId;
      ::WTSFreeMemory(ppi);
      return true;
    }
  }
  ::WTSFreeMemory(ppi);
  return false;
}

inline bool UpdatePrivilege(HANDLE hToken, LPCWSTR lpszPrivilege,
                            bool bEnablePrivilege) {
  TOKEN_PRIVILEGES tp;
  LUID luid;

  if (::LookupPrivilegeValueW(nullptr, lpszPrivilege, &luid) != TRUE) {
    fprintf(stderr, "UpdatePrivilege LookupPrivilegeValue\n");
    return false;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;
  // Enable the privilege or disable all privileges.

  if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                              nullptr, nullptr) != TRUE) {
    fprintf(stderr, "UpdatePrivilege AdjustTokenPrivileges\n");
    return false;
  }

  if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
    return false;
  }
  return true;
}

class SysProcess {
public:
  SysProcess() = default;
  SysProcess(const SysProcess &) = delete;
  SysProcess &operator=(const SysProcess &) = delete;
  ~SysProcess() {
    if (hToken != INVALID_HANDLE_VALUE) {
      CloseHandle(hToken);
    }
  }
  bool Enumerate(DWORD sessionId) {
    PWTS_PROCESS_INFOW ppi;
    DWORD count;
    if (::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppi,
                                 &count) != TRUE) {
      return false;
    }
    HANDLE hExistingToken = nullptr;
    auto end = ppi + count;
    for (auto it = ppi; it != end; it++) {
      if (it->SessionId == sessionId &&
          _wcsicmp(L"winlogon.exe", it->pProcessName) == 0 &&
          IsWellKnownSid(it->pUserSid, WinLocalSystemSid) == TRUE) {
        pid = it->ProcessId;
        ::WTSFreeMemory(ppi);
        return true;
      }
    }
    ::WTSFreeMemory(ppi);
    return false;
  }
  bool SysDuplicateToken(DWORD sessionId) {
    if (hToken != INVALID_HANDLE_VALUE) {
      // duplicate call
      fprintf(stderr, "SysDuplicateToken duplicate call\n");
      return false;
    }
    if (!Enumerate(sessionId)) {
      return false;
    }

    HANDLE hExistingToken = INVALID_HANDLE_VALUE;
    auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
    if (hProcess == INVALID_HANDLE_VALUE) {
      fprintf(stderr, "cannot open system process handle, pid %d\n", pid);
      return false;
    }
    auto hpdeleter = base::finally([&] { CloseHandle(hProcess); });
    if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken) != TRUE) {
      fprintf(stderr, "cannot open system process token pid: %d\n", pid);
      return false;
    }
    auto htdeleter = base::finally([&] { CloseHandle(hExistingToken); });
    if (DuplicateTokenEx(hExistingToken, MAXIMUM_ALLOWED, nullptr,
                         SecurityImpersonation, TokenImpersonation,
                         &hToken) != TRUE) {
      return false;
    }
    return true;
  }
  //
  bool PrivilegesEnableAll() {
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
    auto pfree = base::finally([&] { HeapFree(GetProcessHeap(), 0, privs); });
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

  bool PrivilegesEnableView(const PrivilegeView *pv) {
    if (pv == nullptr) {
      return PrivilegesEnableAll();
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
        fwprintf(stderr, L"AdjustTokenPrivileges %s\n", lpszPrivilege);
        continue;
      }
      if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        return false;
      }
    }
    return true;
  }

  HANDLE Token() { return hToken; }
  DWORD SystemPID() const { return pid; }

private:
  HANDLE hToken{INVALID_HANDLE_VALUE};
  DWORD pid{0};
};

class SysImpersonator {
public:
  SysImpersonator() = default;
  SysImpersonator(const SysImpersonator &) = delete;
  SysImpersonator &operator=(const SysImpersonator &) = delete;
  ~SysImpersonator() = default;
  // query session and enable debug privilege
  bool PreImpersonation() {
    auto hToken = INVALID_HANDLE_VALUE;
    constexpr DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    if (OpenProcessToken(GetCurrentProcess(), flags, &hToken) != TRUE) {
      return false;
    }
    auto closer = base::finally([&] { CloseHandle(hToken); });
    DWORD Length = 0;
    if (GetTokenInformation(hToken, TokenSessionId, &sessionId, sizeof(DWORD),
                            &Length) != TRUE) {
      return false;
    }
    return UpdatePrivilege(hToken, SE_DEBUG_NAME, TRUE);
  }
  // Allowed All
  bool Impersonation(const PrivilegeView *pv) {
    SysProcess sp;
    if (!sp.SysDuplicateToken(sessionId)) {
      return false;
    }
    syspid = sp.SystemPID();
    if (!sp.PrivilegesEnableView(pv)) {
      return false;
    }
    if (SetThreadToken(nullptr, sp.Token()) != TRUE) {
      return false;
    }
    return true;
  }
  DWORD SessionId() const { return sessionId; }
  DWORD SystemPID() const { return syspid; }

private:
  DWORD sessionId{0};
  DWORD syspid{0};
};

} // namespace priv

#endif
