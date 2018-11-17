///////////
#ifndef PRIVEXEC_SYSTEMTOKEN_HPP
#define PRIVEXEC_SYSTEMTOKEN_HPP
#include "processfwd.hpp"
#include <WtsApi32.h>
#include <cstdio>

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
  PWTS_PROCESS_INFO ppi;
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
  fprintf(stderr, "unable found any system process\n");
  ::WTSFreeMemory(ppi);
  return false;
}

} // namespace priv

#endif