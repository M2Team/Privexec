#include "stdafx.h"
#include <vector>
#include <memory>
#include <type_traits>
#include <sddl.h>
#include <Userenv.h>
#include <functional>
#include <ShlObj.h>
#include "pugixml/pugixml.hpp"
#include "Privexec.Core.hpp"

#define APPCONTAINER_PROFILE_NAME L"Privexec.Core.AppContainer"
#define APPCONTAINER_PROFILE_DISPLAYNAME L"Privexec.Core.AppContainer"
#define APPCONTAINER_PROFILE_DESCRIPTION L"Privexec Core AppContainer"

namespace priv {

template <typename T> struct Entry {
  const char *key;
  const T value;

  Entry(const char *t, const T p) : key(t), value(p) {}

  inline bool operator==(const char *okey) const {
    return strcmp(okey, key) == 0;
  }
};

/*
see :
https://docs.microsoft.com/en-us/uwp/schemas/appxpackage/appxmanifestschema/element-capability
internetClient
internetClientServer
privateNetworkClientServer
documentsLibrary
picturesLibrary
videosLibrary
musicLibrary
enterpriseAuthentication
sharedUserCertificates
removableStorage
https://docs.microsoft.com/en-us/windows/desktop/SecAuthZ/well-known-sids
*/
static const Entry<WELL_KNOWN_SID_TYPE> capabilitiesList[] = {
    Entry<WELL_KNOWN_SID_TYPE>(u8"internetClient",
                               WinCapabilityInternetClientSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"internetClientServer",
                               WinCapabilityInternetClientServerSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"privateNetworkClientServer",
                               WinCapabilityPrivateNetworkClientServerSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"documentsLibrary",
                               WinCapabilityDocumentsLibrarySid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"picturesLibrary",
                               WinCapabilityPicturesLibrarySid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"videosLibrary",
                               WinCapabilityPicturesLibrarySid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"musicLibrary", WinCapabilityMusicLibrarySid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"enterpriseAuthentication",
                               WinCapabilityEnterpriseAuthenticationSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"sharedUserCertificates",
                               WinCapabilitySharedUserCertificatesSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"removableStorage",
                               WinCapabilityRemovableStorageSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"appointments", WinCapabilityAppointmentsSid),
    Entry<WELL_KNOWN_SID_TYPE>(u8"contacts", WinCapabilityContactsSid),
};

bool CapabilitiesAdd(std::vector<WELL_KNOWN_SID_TYPE> &caps, const char *key) {
  const auto &e =
      std::find(std::begin(capabilitiesList), std::end(capabilitiesList), key);
  if (e != std::end(capabilitiesList)) {
    caps.push_back(e->value);
  }
  return true;
}

// Lookup Package.Capabilities.rescap:Capability
// Or Package.Capabilities.Capability
bool WellKnownFromAppmanifest(const std::wstring &file,
                              std::vector<WELL_KNOWN_SID_TYPE> &sids) {
  pugi::xml_document doc;
  if (!doc.load_file(file.c_str())) {
    return false;
  }
  auto elem = doc.child("Package").child("Capabilities");
  for (auto it : elem.children("Capability")) {
    CapabilitiesAdd(sids, it.attribute("Name").as_string());
  }
  for (auto it : elem.children("rescap:Capability")) {
    CapabilitiesAdd(sids, it.attribute("Name").as_string());
  }
  return true;
}


bool AppContainerContext::InitializeWithCapabilities(WELL_KNOWN_SID_TYPE *knarr,
                                                     int arraylen) {
  if (knarr == nullptr || arraylen == 0) {
    return false;
  }
  for (int i = 0; i < arraylen; i++) {
    PSID sid_ = HeapAlloc(GetProcessHeap(), 0, SECURITY_MAX_SID_SIZE);
    if (sid_ == nullptr) {
      return false;
    }
    DWORD sidListSize = SECURITY_MAX_SID_SIZE;
    if (::CreateWellKnownSid(knarr[i], NULL, sid, &sidListSize) == FALSE) {
      HeapFree(GetProcessHeap(), 0, sid_);
      continue;
    }
    if (::IsWellKnownSid(sid, knarr[i]) == FALSE) {
      HeapFree(GetProcessHeap(), 0, knarr);
      continue;
    }
    SID_AND_ATTRIBUTES attr;
    attr.Sid = sid;
    attr.Attributes = SE_GROUP_ENABLED;
    ca.push_back(attr);
  }
  auto hr = DeleteAppContainerProfile(APPCONTAINER_PROFILE_NAME);
  if (::CreateAppContainerProfile(
          APPCONTAINER_PROFILE_NAME, APPCONTAINER_PROFILE_DISPLAYNAME,
          APPCONTAINER_PROFILE_DESCRIPTION, (ca.empty() ? NULL : ca.data()),
          (DWORD)ca.size(), &sid) != S_OK) {
    return false;
  }
  return true;
}

bool AppContainerContext::Initialize() {
  WELL_KNOWN_SID_TYPE wslist[] = {
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
  return InitializeWithCapabilities(wslist, _countof(wslist));
}

bool AppContainerContext::InitializeWithSID(const std::wstring &ssid) {
  if (ConvertStringSidToSidW(ssid.c_str(), &sid) != TRUE) {
    return false;
  }
  return true;
}

bool AppContainerContext::InitializeWithAppmanifest(const std::wstring &file) {
  std::vector<WELL_KNOWN_SID_TYPE> sids;
  if (WellKnownFromAppmanifest(file, sids)) {
    return false;
  }
  return InitializeWithCapabilities(sids.data(), (int)sids.size());
}

bool AppContainerContext::Execute(LPWSTR pszComline, DWORD &dwProcessId) {
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
  sc.AppContainerSid = sid;
  sc.Capabilities = (ca.empty() ? NULL : ca.data());
  sc.CapabilityCount = static_cast<DWORD>(ca.size());
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

bool CreateAppContainerProcess(LPWSTR pszComline, DWORD &dwProcessId,
                               const std::wstring &appmanifest) {
  AppContainerContext ctx;
  if (appmanifest.empty()) {
    if (ctx.Initialize()) {
      return false;
    }
  } else {
    if (!ctx.InitializeWithAppmanifest(appmanifest)) {
      return false;
    }
  }
  return ctx.Execute(pszComline, dwProcessId);
}

} // namespace priv
