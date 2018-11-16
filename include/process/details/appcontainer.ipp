// appcontainer
#ifndef PRIVEXEC_APPCONTAINER_IPP
#define PRIVEXEC_APPCONTAINER_IPP
#include <sddl.h>
#include <Userenv.h>
#include <functional>
#include <ShlObj.h>
#include "processfwd.hpp"

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

inline bool capabilitiesadd(widcontainer &caps, const char *key) {
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
                                 WinCapabilityVideosLibrarySid),
      Entry<WELL_KNOWN_SID_TYPE>(u8"musicLibrary",
                                 WinCapabilityMusicLibrarySid),
      Entry<WELL_KNOWN_SID_TYPE>(u8"enterpriseAuthentication",
                                 WinCapabilityEnterpriseAuthenticationSid),
      Entry<WELL_KNOWN_SID_TYPE>(u8"sharedUserCertificates",
                                 WinCapabilitySharedUserCertificatesSid),
      Entry<WELL_KNOWN_SID_TYPE>(u8"removableStorage",
                                 WinCapabilityRemovableStorageSid),
      Entry<WELL_KNOWN_SID_TYPE>(u8"appointments",
                                 WinCapabilityAppointmentsSid),
      Entry<WELL_KNOWN_SID_TYPE>(u8"contacts", WinCapabilityContactsSid),
  };
  const auto &e =
      std::find(std::begin(capabilitiesList), std::end(capabilitiesList), key);
  if (e != std::end(capabilitiesList)) {
    caps.push_back(e->value);
  }
  return true;
}

bool appcontainer::initialize(const wid_t *begin, const wid_t *end) {
  if (!ca.empty()) {
    return false;
  }
  if (begin == nullptr || end == nullptr) {
    return false;
  }
  for (auto it = begin; it != end; it++) {
    PSID psid = HeapAlloc(GetProcessHeap(), 0, SECURITY_MAX_SID_SIZE);
    if (psid == nullptr) {
      return false;
    }
    DWORD sidlistsize = SECURITY_MAX_SID_SIZE;
    if (::CreateWellKnownSid(*it, NULL, psid, &sidlistsize) != TRUE) {
      HeapFree(GetProcessHeap(), 0, psid);
      continue;
    }
    if (::IsWellKnownSid(psid, *it) != TRUE) {
      HeapFree(GetProcessHeap(), 0, psid);
      continue;
    }
    SID_AND_ATTRIBUTES attr;
    attr.Sid = psid;
    attr.Attributes = SE_GROUP_ENABLED;
    ca.push_back(attr);
  }
  constexpr const wchar_t *appid = L"Privexec.Core.AppContainer.v2";
  auto hr = DeleteAppContainerProfile(appid);
  if (hr != S_OK) {
    return false;
  }
  if ((hr = ::CreateAppContainerProfile(
           appid, appid, appid, (ca.empty() ? NULL : ca.data()),
           (DWORD)ca.size(), &appcontainersid)) != S_OK) {
    return false;
  }
  return true;
}
bool appcontainer::initialize(const std::wstring &appxml) {
  //
  return true;
}
bool appcontainer::initializessid(const std::wstring &ssid) {
  // unimpl
  return false;
}
bool appcontainer::execute(error_code &ec, int level) {
  //
  return true;
}
} // namespace priv

#endif