//// AppContainer capabilities
#include <bela/base.hpp>
#include <bela/codecvt.hpp>
//#include <bela/phmap.hpp>
#include <pugixml.hpp>
#include <sddl.h>
#include <Userenv.h>
#include "process_fwd.hpp"

namespace priv {
//
template <typename T> struct Entry {
  std::string_view key;
  const T value;
  Entry(const char *t, const T p) : key(t), value(p) {}
  inline bool operator==(const char *okey) const { return key == okey; }
};

bool MergeFromAppManifest(std::wstring_view file,
                          std::vector<std::wstring> &caps) {
  pugi::xml_document doc;
  if (!doc.load_file(file.data())) {
    return false;
  }
  auto CapAppend = [&](const std::wstring &c) {
    if (std::find(std::begin(caps), std::end(caps), c) == std::end(caps)) {
      caps.emplace_back(c);
    }
  };
  auto elem = doc.child("Package").child("Capabilities");
  for (auto it : elem.children("Capability")) {
    auto n = it.attribute("Name").as_string();
    CapAppend(bela::ToWide(n));
  }
  for (auto it : elem.children("rescap:Capability")) {
    auto n = it.attribute("Name").as_string();
    CapAppend(bela::ToWide(n));
  }
  return true;
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

bool AppContainer::InitialzieFile(std::wstring_view file) {
  std::vector<std::wstring> caps;
  if (!MergeFromAppManifest(file, caps)) {
    return false;
  }
  // Convert to bela::Span
  return Initialzie({caps.data(), caps.size()});
}

bool AppContainer::Initialzie(const bela::Span<std::wstring> caps) {
  //
  auto _DeriveCapabilitySidsFromName =
      (DeriveCapabilitySidsFromNameImpl)GetProcAddress(
          GetModuleHandle(L"KernelBase.dll"), "DeriveCapabilitySidsFromName");
  if (_DeriveCapabilitySidsFromName == nullptr) {
    kmessage = L"DeriveCapabilitySidsFromName Not Found in KernelBase.dll";
    return false;
  }
  for (const auto &n : caps) {
    DWORD dwn = 0, dwca = 0;
    SidArray capability_group_sids;
    SidArray capability_sids;
    if (!_DeriveCapabilitySidsFromName(
            n.c_str(), capability_group_sids.sids_ptr(),
            capability_group_sids.count_ptr(), capability_sids.sids_ptr(),
            capability_sids.count_ptr())) {
      //   auto ec = base::make_system_error_code();
      //   fwprintf(stderr, L"DeriveCapabilitySidsFromName: %s\n",
      //            ec.message.c_str());
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
  if (name.empty()) {
    name = appid;
  }
  DeleteAppContainerProfile(name.data()); // ignore error
  if (CreateAppContainerProfile(name.data(), name.data(), name.data(),
                                (ca.empty() ? NULL : ca.data()),
                                (DWORD)ca.size(), &appcontainersid) != S_OK) {
    kmessage = L"CreateAppContainerProfile error";
    return false;
  }
  return true;
}

bool AppContainer::Initialize(const bela::Span<wid_t> wids) {
  //
  return false;
}

bool AppContainer::InitializeNone() {
  wid_t wids[] = {
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
  return Initialize(wids);
}

} // namespace priv