// system run
#ifndef PRIVE_SYSTEMEXEC_IPP
#define PRIVE_SYSTEMEXEC_IPP
#include "processfwd.hpp"
#include <cstdio>
#include "systemhelper.hpp"

namespace priv {

inline bool process::systemexec() {
  SysImpersonator impersonator;
  if (!impersonator.PreImpersonation()) {
    return false;
  }
  PrivilegeView pv;
  pv.privis.push_back(SE_TCB_NAME);                ///
  pv.privis.push_back(SE_ASSIGNPRIMARYTOKEN_NAME); //
  pv.privis.push_back(SE_INCREASE_QUOTA_NAME);     //
  if (!impersonator.Impersonation(&pv)) {
    return false;
  }
  auto hProcess = INVALID_HANDLE_VALUE;
  auto hToken = INVALID_HANDLE_VALUE;
  auto hPrimary = INVALID_HANDLE_VALUE;
  auto deleter = base::finally([&] {
    CloseHandleEx(hProcess);
    CloseHandleEx(hToken);
    CloseHandleEx(hPrimary);
  });
  if ((hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE,
                              impersonator.SystemPID())) == nullptr) {
    kmessage = L"systemexec<OpenProcess>";
    return false;
  }
  if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) != TRUE) {
    kmessage = L"systemexec<OpenProcessToken>";
    return false;
  }
  if (DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation,
                       TokenPrimary, &hPrimary) != TRUE) {
    kmessage = L"systemexec<DuplicateTokenEx>";
    return false;
  }
  auto session = impersonator.SessionId();
  if (SetTokenInformation(hPrimary, TokenSessionId, &session,
                          sizeof(session)) != TRUE) {
    return false;
  }
  return execwithtoken(hPrimary, true);
}
} // namespace priv

#endif
