// low level
#ifndef PRIVEXEC_LOWLEVEL_HPP
#define PRIVEXEC_LOWLEVEL_HPP
#include "processfwd.hpp"
#include <Sddl.h>

namespace priv {

inline bool process::lowlevelexec() {
  HANDLE hToken;
  HANDLE hNewToken;
  LPCWSTR szIntegritySid = L"S-1-16-4096";
  PSID pIntegritySid = NULL;
  TOKEN_MANDATORY_LABEL TIL = {0};
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
    kmessage = L"lowlevelexec<OpenProcessToken>";
    return false;
  }
  if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
                        TokenPrimary, &hNewToken)) {
    kmessage = L"lowlevelexec<DuplicateTokenEx>";
    CloseHandle(hToken);
    return false;
  }
  if (!ConvertStringSidToSidW(szIntegritySid, &pIntegritySid)) {
    CloseHandle(hToken);
    CloseHandle(hNewToken);
    kmessage = L"lowlevelexec<ConvertStringSidToSidW>";
    return false;
  }

  TIL.Label.Attributes = SE_GROUP_INTEGRITY;
  TIL.Label.Sid = pIntegritySid;
  auto deleter = base::finally([&] {
    CloseHandle(hToken);
    CloseHandle(hNewToken);
    LocalFree(pIntegritySid);
  });
  // Set process integrity levels
  if (!SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
                           sizeof(TOKEN_MANDATORY_LABEL) +
                               GetLengthSid(pIntegritySid))) {
    kmessage = L"lowlevelexec<SetTokenInformation>";
    return false;
  }
  return execwithtoken(hNewToken);
}
} // namespace priv

#endif