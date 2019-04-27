// appcontainer
#ifndef PRIVEXEC_APPCONTAINER_IPP
#define PRIVEXEC_APPCONTAINER_IPP
#include <sddl.h>
#include <Userenv.h>
#include <functional>
#include <ShlObj.h>
#include <memory>
#include <xml.hpp>
#include "processfwd.hpp"
#include "acl.ipp"

#pragma comment(lib, "Ntdll.lib")

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
  static const Entry<wid_t> capabilitiesList[] = {
      Entry<wid_t>(u8"internetClient", WinCapabilityInternetClientSid),
      Entry<wid_t>(u8"internetClientServer",
                   WinCapabilityInternetClientServerSid),
      Entry<wid_t>(u8"privateNetworkClientServer",
                   WinCapabilityPrivateNetworkClientServerSid),
      Entry<wid_t>(u8"documentsLibrary", WinCapabilityDocumentsLibrarySid),
      Entry<wid_t>(u8"picturesLibrary", WinCapabilityPicturesLibrarySid),
      Entry<wid_t>(u8"videosLibrary", WinCapabilityVideosLibrarySid),
      Entry<wid_t>(u8"musicLibrary", WinCapabilityMusicLibrarySid),
      Entry<wid_t>(u8"enterpriseAuthentication",
                   WinCapabilityEnterpriseAuthenticationSid),
      Entry<wid_t>(u8"sharedUserCertificates",
                   WinCapabilitySharedUserCertificatesSid),
      Entry<wid_t>(u8"removableStorage", WinCapabilityRemovableStorageSid),
      Entry<wid_t>(u8"appointments", WinCapabilityAppointmentsSid),
      Entry<wid_t>(u8"contacts", WinCapabilityContactsSid),
  };
  const auto &e =
      std::find(std::begin(capabilitiesList), std::end(capabilitiesList), key);
  if (e != std::end(capabilitiesList)) {
    caps.push_back(e->value);
  }
  return true;
}

// Lookup Package.Capabilities.rescap:Capability
// Or Package.Capabilities.Capability
inline bool WellKnownFromAppmanifest(const std::wstring &file,
                                     widcontainer &sids) {
  pugi::xml_document doc;
  if (!doc.load_file(file.c_str())) {
    return false;
  }
  auto elem = doc.child("Package").child("Capabilities");
  for (auto it : elem.children("Capability")) {
    capabilitiesadd(sids, it.attribute("Name").as_string());
  }
  for (auto it : elem.children("rescap:Capability")) {
    capabilitiesadd(sids, it.attribute("Name").as_string());
  }
  return true;
}

inline std::wstring u8w(std::string_view str) {
  std::wstring wstr;
  auto N =
      MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
  if (N > 0) {
    wstr.resize(N);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], N);
  }
  return wstr;
}

inline bool MergeFromAppmanifest(const std::wstring &file,
                                 std::vector<std::wstring> &cans) {
  pugi::xml_document doc;
  if (!doc.load_file(file.c_str())) {
    return false;
  }
  auto Append = [&](const std::wstring &c) {
    if (std::find(std::begin(cans), std::end(cans), c) == std::end(cans)) {
      cans.push_back(c);
    }
  };

  auto elem = doc.child("Package").child("Capabilities");
  for (auto it : elem.children("Capability")) {
    auto n = it.attribute("Name").as_string();
    Append(u8w(n));
  }
  for (auto it : elem.children("rescap:Capability")) {
    auto n = it.attribute("Name").as_string();
    Append(u8w(n));
  }
  return true;
}

inline bool FromAppmanifest(std::wstring_view file,
                            std::vector<std::wstring> &cans) {
  pugi::xml_document doc;
  if (!doc.load_file(file.data())) {
    return false;
  }
  auto elem = doc.child("Package").child("Capabilities");
  for (auto it : elem.children("Capability")) {
    auto n = it.attribute("Name").as_string();
    cans.push_back(u8w(n));
  }
  for (auto it : elem.children("rescap:Capability")) {
    auto n = it.attribute("Name").as_string();
    cans.push_back(u8w(n));
  }
  return true;
}

inline bool appcontainer::initialize(const wid_t *begin, const wid_t *end) {
  if (!ca.empty()) {
    kmessage.assign(L"capabilities is initialized");
    return false;
  }
  if (begin == nullptr || end == nullptr) {
    kmessage.assign(L"capabilities well known sid is empty");
    return false;
  }
  for (auto it = begin; it != end; it++) {
    PSID psid = HeapAlloc(GetProcessHeap(), 0, SECURITY_MAX_SID_SIZE);
    if (psid == nullptr) {
      kmessage.assign(L"alloc psid failed");
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
  constexpr const wchar_t *appid = L"Privexec.AppContainer.WellKnownSid";
  if (name_.empty()) {
    name_ = appid;
  }
  DeleteAppContainerProfile(name_.c_str()); // ignore error
  if (CreateAppContainerProfile(name_.c_str(), name_.c_str(), name_.c_str(),
                                (ca.empty() ? NULL : ca.data()),
                                (DWORD)ca.size(), &appcontainersid) != S_OK) {
    kmessage.assign(L"CreateAppContainerProfile");
    return false;
  }
  return true;
}

inline bool appcontainer::initialize() {
  wid_t wslist[] = {
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
  return initialize(wslist, wslist + _countof(wslist));
}
//
// typedef NTSTATUS(NTAPI *PRtlDeriveCapabilitySidsFromName)(
//     PCUNICODE_STRING CapName, PSID CapabilityGroupSid, PSID CapabilitySid);

class SidArray {
public:
  SidArray() : count_(0), sids_(nullptr) {}

  ~SidArray() {
    if (sids_) {
      for (size_t index = 0; index < count_; ++index) {
        ::LocalFree(sids_[index]);
      }
      ::LocalFree(sids_);
    }
  }

  DWORD count() { return count_; }
  PSID *sids() { return sids_; }
  PDWORD count_ptr() { return &count_; }
  PSID **sids_ptr() { return &sids_; }

private:
  DWORD count_;
  PSID *sids_;
};

typedef BOOL(WINAPI *DeriveCapabilitySidsFromNameImpl)(
    LPCWSTR CapName, PSID **CapabilityGroupSids, DWORD *CapabilityGroupSidCount,
    PSID **CapabilitySids, DWORD *CapabilitySidCount);

inline bool appcontainer::initialize(const std::vector<std::wstring> &names) {
  //
  auto _DeriveCapabilitySidsFromName =
      (DeriveCapabilitySidsFromNameImpl)GetProcAddress(
          GetModuleHandle(L"KernelBase.dll"), "DeriveCapabilitySidsFromName");
  if (_DeriveCapabilitySidsFromName == nullptr) {
    fwprintf(stderr,
             L"DeriveCapabilitySidsFromName Not Found in KernelBase.dll\n");
    return false;
  }
  for (const auto &n : names) {
    DWORD dwn = 0, dwca = 0;
    SidArray capability_group_sids;
    SidArray capability_sids;
    if (!_DeriveCapabilitySidsFromName(
            n.c_str(), capability_group_sids.sids_ptr(),
            capability_group_sids.count_ptr(), capability_sids.sids_ptr(),
            capability_sids.count_ptr())) {
      auto ec = base::make_system_error_code();
      fwprintf(stderr, L"DeriveCapabilitySidsFromName: %s\n",
               ec.message.c_str());
      continue;
    }
    if (capability_sids.count() < 1) {
      continue;
    }
    auto csid = HeapAlloc(GetProcessHeap(), 0, SECURITY_MAX_SID_SIZE);
    if (csid == nullptr) {
      return false;
    }
    ::CopySid(SECURITY_MAX_SID_SIZE, csid, capability_sids.sids()[0]);
    SID_AND_ATTRIBUTES attr;
    attr.Sid = csid;
    attr.Attributes = SE_GROUP_ENABLED;
    ca.push_back(attr);
  }
  constexpr const wchar_t *appid =
      L"Privexec.AppContainer.DeriveCapabilitySidsFromName";
  if (name_.empty()) {
    name_ = appid;
  }
  DeleteAppContainerProfile(name_.c_str()); // ignore error
  if (CreateAppContainerProfile(name_.c_str(), name_.c_str(), name_.c_str(),
                                (ca.empty() ? NULL : ca.data()),
                                (DWORD)ca.size(), &appcontainersid) != S_OK) {
    kmessage.assign(L"CreateAppContainerProfile");
    return false;
  }
  return true;
}

inline bool appcontainer::initialize(const std::wstring &appxml) {
  std::vector<std::wstring> cans;
  if (!FromAppmanifest(appxml, cans)) {
    return false;
  }
  return initialize(cans);
}

inline bool appcontainer::initializessid(const std::wstring &ssid) {
  if (ConvertStringSidToSidW(ssid.c_str(), &appcontainersid) != TRUE) {
    return false;
  }
  return true;
}

// PAWapper is
using PAttribute = LPPROC_THREAD_ATTRIBUTE_LIST;
#define PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY_EX               \
  ProcThreadAttributeValue(ProcThreadAttributeAllApplicationPackagesPolicy,    \
                           FALSE, TRUE, FALSE)

inline bool appcontainer::execute() {
  PWSTR pszstr;
  if (::ConvertSidToStringSidW(appcontainersid, &pszstr) != TRUE) {
    return false;
  }
  strid_.assign(pszstr);
  PWSTR pszfolder;
  if (SUCCEEDED(::GetAppContainerFolderPath(pszstr, &pszfolder))) {
    folder_ = pszfolder;
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
  auto act = base::finally([&] {
    // delete when func exit.
    DeleteProcThreadAttributeList(siex.lpAttributeList);
  });
  if (InitializeProcThreadAttributeList(siex.lpAttributeList, 3, 0,
                                        &cbAttributeListSize) != TRUE) {
    kmessage.assign(L"InitializeProcThreadAttributeList");
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
    kmessage.assign(L"UpdateProcThreadAttribute");
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
      kmessage.assign(L"UpdateProcThreadAttribute(LPAC)");
      return false;
    }
  }
  for (auto &f : alloweddirs_) {
    AllowNameObjectAccess(appcontainersid, f.data(), SE_FILE_OBJECT,
                          FILE_ALL_ACCESS);
  }

  for (auto &r : registries_) {
    AllowNameObjectAccess(appcontainersid, r.data(), SE_REGISTRY_KEY,
                          KEY_ALL_ACCESS);
  }

  DWORD createflags = EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT;
  if (visible == VisibleNewConsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (visible == VisibleHide) {
    createflags |= CREATE_NO_WINDOW;
  }

  if (CreateProcessW(nullptr, &cmd_[0], nullptr, nullptr, FALSE, createflags,
                     nullptr, Castwstr(cwd_),
                     reinterpret_cast<STARTUPINFOW *>(&siex), &pi) != TRUE) {
    kmessage.assign(L"CreateProcessW");
    return false;
  }
  pid_ = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}
} // namespace priv

#endif