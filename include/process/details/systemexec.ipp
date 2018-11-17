// system run
#ifndef PRIVE_SYSTEMEXEC_IPP
#define PRIVE_SYSTEMEXEC_IPP
#include "processfwd.hpp"
#include <wtsapi32.h>
#include <cstdio>

namespace priv {

inline HANDLE OpenSystemProcessToken() {
  PWTS_PROCESS_INFO pInfo;
  DWORD count;
  DWORD dwSessionId;
  if (!GetCurrentSessionId(dwSessionId)) {
    return INVALID_HANDLE_VALUE;
  }
  if (!::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pInfo,
                                &count)) {
    return INVALID_HANDLE_VALUE;
  }

  HANDLE hExistingToken = nullptr;
  HANDLE hToken = INVALID_HANDLE_VALUE;
  for (DWORD i = 0; i < count && !hExistingToken; i++) {
    if (pInfo[i].SessionId == dwSessionId &&
        IsWellKnownSid(pInfo[i].pUserSid, WinLocalSystemSid)) {
      auto hProcess = ::OpenProcess(MAXIMUM_ALLOWED, FALSE, pInfo[i].ProcessId);
      if (hProcess) {
        ::OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hExistingToken);
        ::CloseHandle(hProcess);
      }
    }
  }

  ::WTSFreeMemory(pInfo);
  if (hExistingToken != nullptr) {
    DuplicateTokenEx(hExistingToken, MAXIMUM_ALLOWED, nullptr,
                     SecurityIdentification, TokenPrimary, &hToken);
    CloseHandle(hExistingToken);
  }
  return hToken;
}

bool process::systemexec() {
  //
  return true;
}
} // namespace priv

#endif