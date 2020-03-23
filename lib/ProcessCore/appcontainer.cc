////
#include "process_fwd.hpp"
#include <bela/base.hpp>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <Userenv.h>

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

using PAttribute = LPPROC_THREAD_ATTRIBUTE_LIST;
#define PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY_EX               \
  ProcThreadAttributeValue(ProcThreadAttributeAllApplicationPackagesPolicy,    \
                           FALSE, TRUE, FALSE)

bool AppContainer::Exec() {
  PWSTR pszstr;
  if (::ConvertSidToStringSidW(appcontainersid, &pszstr) != TRUE) {
    return false;
  }
  strid.assign(pszstr);
  PWSTR pszfolder;
  if (SUCCEEDED(::GetAppContainerFolderPath(pszstr, &pszfolder))) {
    folder = pszfolder;
    if (cwd.empty()) {
      cwd = folder;
    }
    ::CoTaskMemFree(pszfolder);
  }
  ::LocalFree(pszstr);
  PROCESS_INFORMATION pi;
  STARTUPINFOEX siex = {sizeof(STARTUPINFOEX)};
  siex.StartupInfo.cb = sizeof(siex);
  SIZE_T cbAttributeListSize = 0;
  InitializeProcThreadAttributeList(NULL, 3, 0, &cbAttributeListSize);
  siex.lpAttributeList = reinterpret_cast<PAttribute>(
      HeapAlloc(GetProcessHeap(), 0, cbAttributeListSize));
  auto act = bela::finally([&] {
    // delete when func exit.
    DeleteProcThreadAttributeList(siex.lpAttributeList);
  });
  if (InitializeProcThreadAttributeList(siex.lpAttributeList, 3, 0,
                                        &cbAttributeListSize) != TRUE) {
    kmessage = L"InitializeProcThreadAttributeList";
    return false;
  }
  SECURITY_CAPABILITIES sc;
  sc.AppContainerSid = appcontainersid;
  sc.Capabilities = (ca.empty() ? NULL : ca.data());
  sc.CapabilityCount = static_cast<DWORD>(ca.size());
  sc.Reserved = 0;
  if (UpdateProcThreadAttribute(siex.lpAttributeList, 0,
                                PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
                                &sc, sizeof(sc), nullptr, nullptr) != TRUE) {
    kmessage = L"UpdateProcThreadAttribute";
    return false;
  }
  DWORD dwvalue = 1;
  if (lpac) {
    // ProcThreadAttributeAllApplicationPackagesPolicy
    // UpdateProcThreadAttribute(siex.lpAttributeList,0,)
    if (UpdateProcThreadAttribute(
            siex.lpAttributeList, 0,
            PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY_EX, &dwvalue,
            sizeof(dwvalue), nullptr, nullptr) != TRUE) {
      kmessage = L"call UpdateProcThreadAttribute(LPAC) failed";
      return false;
    }
  }
  for (auto &f : alloweddirs) {
    AllowNameObjectAccess(appcontainersid, f.data(), SE_FILE_OBJECT,
                          FILE_ALL_ACCESS);
  }

  for (auto &r : registries) {
    AllowNameObjectAccess(appcontainersid, r.data(), SE_REGISTRY_KEY,
                          KEY_ALL_ACCESS);
  }

  DWORD createflags = EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT;
  if (visible == VisibleMode::NewConsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (visible == VisibleMode::Hide) {
    createflags |= CREATE_NO_WINDOW;
  }

  if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, createflags,
                     nullptr, EmptyNull(cwd),
                     reinterpret_cast<STARTUPINFOW *>(&siex), &pi) != TRUE) {
    kmessage = L"call CreateProcessW (appcontainer) failed";
    return false;
  }
  pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

} // namespace priv
