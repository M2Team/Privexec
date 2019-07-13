////
#include "process_fwd.hpp"
#include <accctrl.h>
#include <aclapi.h>

namespace priv {
//
bool AllowNameObjectAccess(PSID appContainerSid, LPWSTR name,
                           SE_OBJECT_TYPE type, ACCESS_MASK accessMask) {
  PACL oldAcl, newAcl = nullptr;
  DWORD status;
  EXPLICIT_ACCESSW ea;
  do {
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = accessMask;
    ea.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ea.Trustee.pMultipleTrustee = nullptr;
    ea.Trustee.ptstrName = (PWSTR)appContainerSid;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    status = GetNamedSecurityInfoW(name, type, DACL_SECURITY_INFORMATION,
                                   nullptr, nullptr, &oldAcl, nullptr, nullptr);
    if (status != ERROR_SUCCESS) {
      return false;
    }
    if (SetEntriesInAclW(1, &ea, oldAcl, &newAcl) != ERROR_SUCCESS) {
      return false;
    }
    status = SetNamedSecurityInfoW(name, type, DACL_SECURITY_INFORMATION,
                                   nullptr, nullptr, newAcl, nullptr);
    if (status != ERROR_SUCCESS) {
      break;
    }
  } while (false);
  if (newAcl) {
    ::LocalFree(newAcl);
  }
  return status == ERROR_SUCCESS;
}
} // namespace priv