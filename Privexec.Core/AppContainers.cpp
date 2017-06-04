#include "stdafx.h"
#include <vector>
#include <memory>
#include <type_traits>
#include <sddl.h>
#include <Userenv.h>
#include <functional>
#include <ShlObj.h>
#include "Privexec.Core.hpp"

#define APPCONTAINER_PROFILE_NAME L"Privexec.Core.AppContainer"
#define APPCONTAINER_PROFILE_DISPLAYNAME L"Privexec.Core.AppContainer"
#define APPCONTAINER_PROFILE_DESCRIPTION L"Privexec Core AppContainer"

class AppContainerContext {
public:
  typedef std::vector<SID_AND_ATTRIBUTES> TCapabilitiesList;
  AppContainerContext() : appContainerSid(nullptr) {}
  ~AppContainerContext() {
    ///
    if (appContainerSid) {
      FreeSid(appContainerSid);
    }
    for (auto &c : mCapabilities) {
      if (c.Sid) {
        HeapFree(GetProcessHeap(), 0, c.Sid);
      }
    }
  }
  bool AppContainerContextInitialize() {
    if (!MakeWellKnownSIDAttributes()) {
      return false;
    }
    auto hr = DeleteAppContainerProfile(APPCONTAINER_PROFILE_NAME);
    if (::CreateAppContainerProfile(
            APPCONTAINER_PROFILE_NAME, APPCONTAINER_PROFILE_DISPLAYNAME,
            APPCONTAINER_PROFILE_DESCRIPTION,
            (mCapabilities.empty() ? NULL : mCapabilities.data()),
            (DWORD)mCapabilities.size(), &appContainerSid) != S_OK) {
      return false;
    }
    return true;
  }
  PSID GetAppContainerSid() const { return appContainerSid; }
  TCapabilitiesList &Capabilitis() { return mCapabilities; }

private:
  bool MakeWellKnownSIDAttributes() {
    const WELL_KNOWN_SID_TYPE capabilitiyTypeList[] = {
        WinCapabilityInternetClientSid,
        WinCapabilityInternetClientServerSid,
        WinCapabilityPrivateNetworkClientServerSid,
        WinCapabilityPicturesLibrarySid,
        WinCapabilityVideosLibrarySid,
        WinCapabilityMusicLibrarySid,
        WinCapabilityDocumentsLibrarySid,
        WinCapabilitySharedUserCertificatesSid,
        WinCapabilityEnterpriseAuthenticationSid,
        WinCapabilityRemovableStorageSid,
    };
    for (auto c : capabilitiyTypeList) {
      PSID sid = HeapAlloc(GetProcessHeap(), 0, SECURITY_MAX_SID_SIZE);
      if (sid == nullptr) {
        return false;
      }
      DWORD sidListSize = SECURITY_MAX_SID_SIZE;
      if (::CreateWellKnownSid(c, NULL, sid, &sidListSize) == FALSE) {
        HeapFree(GetProcessHeap(), 0, sid);
        continue;
      }
      if (::IsWellKnownSid(sid, c) == FALSE) {
        HeapFree(GetProcessHeap(), 0, sid);
        continue;
      }
      SID_AND_ATTRIBUTES attr;
      attr.Sid = sid;
      attr.Attributes = SE_GROUP_ENABLED;
      mCapabilities.push_back(attr);
    }
    return true;
  }
  PSID appContainerSid;
  TCapabilitiesList mCapabilities;
};

bool CreateAppContainerProcess(LPWSTR pszComline, DWORD &dwProcessId) {
  AppContainerContext context;
  if (!context.AppContainerContextInitialize()) {
    return false;
  }
  PROCESS_INFORMATION pi;
  STARTUPINFOEX siex = {sizeof(STARTUPINFOEX)};
  siex.StartupInfo.cb = sizeof(siex);
  SIZE_T cbAttributeListSize = 0;
  InitializeProcThreadAttributeList(NULL, 3, 0, &cbAttributeListSize);
  siex.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
      GetProcessHeap(), 0, cbAttributeListSize);
  BOOL bReturn = TRUE;
  if ((bReturn = InitializeProcThreadAttributeList(
           siex.lpAttributeList, 3, 0, &cbAttributeListSize)) == FALSE) {
    return FALSE;
  }
  SECURITY_CAPABILITIES sc;
  sc.AppContainerSid = context.GetAppContainerSid();
  sc.Capabilities =
      (context.Capabilitis().empty() ? NULL : context.Capabilitis().data());
  sc.CapabilityCount = static_cast<DWORD>(context.Capabilitis().size());
  sc.Reserved = 0;
  if ((bReturn = UpdateProcThreadAttribute(
           siex.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
           &sc, sizeof(sc), NULL, NULL)) == FALSE) {
    DeleteProcThreadAttributeList(siex.lpAttributeList);
    return FALSE;
  }
  bReturn = CreateProcessW(
      nullptr, pszComline, nullptr, nullptr, FALSE,
      EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, nullptr,
      nullptr, reinterpret_cast<STARTUPINFOW *>(&siex), &pi);
  if (bReturn) {
    dwProcessId = pi.dwProcessId;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  DeleteProcThreadAttributeList(siex.lpAttributeList);
  return (bReturn != FALSE);
}