/// uac no elevated
#ifndef PRIVEXEC_NOELEVATED_HPP
#define PRIVEXEC_NOELEVATED_HPP
#include <string>
#include <comdef.h>
#include <Sddl.h>
#include <Shlwapi.h>
#include "processfwd.hpp"
#include "systemhelper.hpp"

namespace priv {
inline bool process::noelevatedexec() {
  if (!IsUserAdministratorsGroup()) {
    return execute();
  }
  /// Enable Privilege TODO
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
  auto hToken = INVALID_HANDLE_VALUE;
  auto hTokenDup = INVALID_HANDLE_VALUE;
  auto deleter = base::finally([&] {
    CloseHandleEx(hToken);
    CloseHandleEx(hTokenDup);
  });
  // get user login token
  if (WTSQueryUserToken(impersonator.SessionId(), &hToken) != TRUE) {
    return false;
  }

  if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification,
                       TokenPrimary, &hTokenDup) != TRUE) {
    return false;
  }

  return execwithtoken(hToken);
}

} // namespace priv

#endif
