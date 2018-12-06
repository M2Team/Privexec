///////////
#ifndef PRIVEXEC_SYSTEMTOKEN_HPP
#define PRIVEXEC_SYSTEMTOKEN_HPP
#include "processfwd.hpp"
#include <WtsApi32.h>
#include <cstdio>
#include <vector>

namespace priv {
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

class SysImpersonator {
public:
  SysImpersonator() = default;
  SysImpersonator(const SysImpersonator &) = delete;
  SysImpersonator &operator=(const SysImpersonator &) = delete;
  ~SysImpersonator() {
    if (hToken != INVALID_HANDLE_VALUE) {
      CloseHandle(hToken);
    }
  }
  // query session and enable debug privilege
  bool PreImpersonation() {
    auto hCurrentToken = INVALID_HANDLE_VALUE;
    constexpr DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    if (OpenProcessToken(GetCurrentProcess(), flags, &hCurrentToken) != TRUE) {
      return false;
    }
    auto closer = finally([&] { CloseHandle(hCurrentToken); });
    DWORD Length = 0;
    if (GetTokenInformation(hCurrentToken, TokenSessionId, &sessionId,
                            sizeof(DWORD), &Length) != TRUE) {
      return false;
    }
    return UpdatePrivilege(hToken, SE_DEBUG_NAME, TRUE);
  }
  // Allowed All
  bool Impersonation(bool allowedall = false) {
    //
    return false;
  }

private:
  DWORD sessionId{0};
  DWORD winlogonid{0};
  HANDLE hToken{nullptr};
};

// Enable System Privilege
inline bool EnableSystemToken(std::vector<LPCWSTR> &las) {
  //
  return true;
}

} // namespace priv

#endif