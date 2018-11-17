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
  if (!GetTokenInformation(hToken, TokenSessionId, &dwSessionId, sizeof(DWORD),
                           &Length)) {
    CloseHandle(hToken);
    return false;
  }
  CloseHandle(hToken);
  return true;
}

inline bool LookupSystemProcessID(DWORD pid) {
  PWTS_PROCESS_INFO ppi;
  DWORD count;
  DWORD dwSessionId;
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
    if (it->SessionId != dwSessionId ||
        wcscmp(L"winlogin.exe", it->pProcessName) != 0 ||
        IsWellKnownSid(it->pUserSid, WinLocalLogonSid) != TRUE) {
      continue;
    }
    pid = it->ProcessId;
    ::WTSFreeMemory(ppi);
    return true;
  }

  ::WTSFreeMemory(ppi);
  return false;
}


} // namespace priv

#endif