//
#include "exec.hpp"
#include <bela/base.hpp>
#include <bela/codecvt.hpp>
#include <bela/escapeargv.hpp>
//#include <bela/phmap.hpp>
#define PUGIXML_HEADER_ONLY 1
#include <pugixml.hpp>
#include <sddl.h>
#include <Userenv.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <Userenv.h>

namespace wsudo::exec {

using wid_t = WELL_KNOWN_SID_TYPE;
using widcontainer_t = std::vector<wid_t>;
using capabilities_t = std::vector<SID_AND_ATTRIBUTES>;
//
template <typename T> struct Entry {
  std::string_view key;
  const T value;
  Entry(const char *t, const T p) : key(t), value(p) {}
  inline bool operator==(const char *okey) const { return key == okey; }
};

bool LoadAppx(std::wstring_view file, std::vector<std::wstring> &caps, bela::error_code &ec) {
  pugi::xml_document doc;
  if (!doc.load_file(file.data())) {
    ec = bela::make_system_error_code(L"xml_document load ");
    return false;
  }
  auto addCapability = [&](std::wstring &&sv) {
    for (const auto &s : caps) {
      if (bela::EqualsIgnoreCase(sv, s)) {
        return;
      }
    }
    caps.emplace_back(std::move(sv));
  };
  // Capability
  // rescap:Capability
  // uap:Capability
  // uap3:Capability
  // uap6:Capability
  // wincap:Capability
  // DeviceCapability
  for (auto it : doc.child("Package").child("Capabilities")) {
    addCapability(bela::ToWide(it.attribute("Name").as_string()));
  }
  return true;
}

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

typedef BOOL(WINAPI *DeriveCapabilitySidsFromNameImpl)(LPCWSTR CapName, PSID **CapabilityGroupSids,
                                                       DWORD *CapabilityGroupSidCount, PSID **CapabilitySids,
                                                       DWORD *CapabilitySidCount);

bool appcommand::initialize(bela::error_code &ec) {
  if (!appmanifest.empty()) {
    if (!LoadAppx(appmanifest, caps, ec)) {
      return false;
    }
  }
  return true;
}

bool MakeDeriveSID(PSID &appcontainersid, capabilities_t &cas, appcommand &cmd, bela::error_code &ec) {
  auto _DeriveCapabilitySidsFromName = (DeriveCapabilitySidsFromNameImpl)GetProcAddress(
      GetModuleHandle(L"KernelBase.dll"), "DeriveCapabilitySidsFromName");
  if (_DeriveCapabilitySidsFromName == nullptr) {
    ec = bela::make_system_error_code(L"MakeDeriveSID<DeriveCapabilitySidsFromName> ");
    return false;
  }
  for (const auto &n : cmd.caps) {
    DWORD dwn = 0, dwca = 0;
    SidArray capability_group_sids;
    SidArray capability_sids;
    if (!_DeriveCapabilitySidsFromName(n.c_str(), capability_group_sids.sids_ptr(), capability_group_sids.count_ptr(),
                                       capability_sids.sids_ptr(), capability_sids.count_ptr())) {
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
    cas.push_back(attr);
  }
  constexpr const wchar_t *appid = L"Privexec.AppContainer.DeriveCapabilitySidsFromName";
  if (cmd.appid.empty()) {
    cmd.appid = appid;
  }
  DeleteAppContainerProfile(cmd.appid.data()); // ignore error
  if (CreateAppContainerProfile(cmd.appid.data(), cmd.appid.data(), cmd.appid.data(), (cas.empty() ? NULL : cas.data()),
                                (DWORD)cas.size(), &appcontainersid) != S_OK) {
    ec = bela::make_system_error_code(L"MakeDeriveSID<CreateAppContainerProfile> ");
    return false;
  }
  return true;
}

bool MakeSID(PSID &appcontainersid, capabilities_t &cas, appcommand &cmd, bela::error_code &ec) {
  if (!cmd.caps.empty()) {
    return MakeDeriveSID(appcontainersid, cas, cmd, ec);
  }
  constexpr wid_t wids[] = {
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
  for (auto id : wids) {
    PSID psid = HeapAlloc(GetProcessHeap(), 0, SECURITY_MAX_SID_SIZE);
    if (psid == nullptr) {
      ec = bela::make_system_error_code(L"MakeSID<HeapAlloc> ");
      return false;
    }
    DWORD sidlistsize = SECURITY_MAX_SID_SIZE;
    if (::CreateWellKnownSid(id, NULL, psid, &sidlistsize) != TRUE) {
      HeapFree(GetProcessHeap(), 0, psid);
      continue;
    }
    if (::IsWellKnownSid(psid, id) != TRUE) {
      HeapFree(GetProcessHeap(), 0, psid);
      continue;
    }
    SID_AND_ATTRIBUTES attr;
    attr.Sid = psid;
    attr.Attributes = SE_GROUP_ENABLED;
    cas.push_back(attr);
  }
  constexpr const wchar_t *appid = L"Privexec.AppContainer.WellKnownSid";
  if (cmd.appid.empty()) {
    cmd.appid = appid;
  }
  DeleteAppContainerProfile(cmd.appid.data()); // ignore error
  if (CreateAppContainerProfile(cmd.appid.data(), cmd.appid.data(), cmd.appid.data(), (cas.empty() ? NULL : cas.data()),
                                (DWORD)cas.size(), &appcontainersid) != S_OK) {
    ec = bela::make_system_error_code(L"MakeSID<CreateAppContainerProfile> ");
    return false;
  }
  return true;
}

bool AllowNameObjectAccess(PSID appContainerSid, LPWSTR name, SE_OBJECT_TYPE type, ACCESS_MASK accessMask) {
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
    status = GetNamedSecurityInfoW(name, type, DACL_SECURITY_INFORMATION, nullptr, nullptr, &oldAcl, nullptr, nullptr);
    if (status != ERROR_SUCCESS) {
      return false;
    }
    if (SetEntriesInAclW(1, &ea, oldAcl, &newAcl) != ERROR_SUCCESS) {
      return false;
    }
    status = SetNamedSecurityInfoW(name, type, DACL_SECURITY_INFORMATION, nullptr, nullptr, newAcl, nullptr);
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
#define PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY_EX                                                       \
  ProcThreadAttributeValue(ProcThreadAttributeAllApplicationPackagesPolicy, FALSE, TRUE, FALSE)

bool appcommand::execute(bela::error_code &ec) {
  PSID appcontainersid{nullptr};
  capabilities_t cas;
  if (!MakeSID(appcontainersid, cas, *this, ec)) {
    for (auto &c : cas) {
      HeapFree(GetProcessHeap(), 0, c.Sid);
    }
    return false;
  }
  if (PWSTR pszstr; ::ConvertSidToStringSidW(appcontainersid, &pszstr) == TRUE) {
    sid = pszstr;
    if (PWSTR pszfolder; SUCCEEDED(::GetAppContainerFolderPath(pszstr, &pszfolder))) {
      folder = pszfolder;
      if (cwd.empty()) {
        cwd = folder;
      }
      ::CoTaskMemFree(pszfolder);
    }
    LocalFree(pszstr);
  }
  PROCESS_INFORMATION pi;
  STARTUPINFOEX siex = {sizeof(STARTUPINFOEX)};
  siex.StartupInfo.cb = sizeof(siex);
  SIZE_T cbAttributeListSize = 0;
  InitializeProcThreadAttributeList(NULL, 3, 0, &cbAttributeListSize);
  siex.lpAttributeList = reinterpret_cast<PAttribute>(HeapAlloc(GetProcessHeap(), 0, cbAttributeListSize));
  auto closer = bela::finally([&] {
    // delete when func exit.
    DeleteProcThreadAttributeList(siex.lpAttributeList);
    for (auto &c : cas) {
      HeapFree(GetProcessHeap(), 0, c.Sid);
    }
    if (appcontainersid != nullptr) {
      FreeSid(appcontainersid);
    }
  });
  if (InitializeProcThreadAttributeList(siex.lpAttributeList, 3, 0, &cbAttributeListSize) != TRUE) {
    ec = bela::make_system_error_code(L"appcommand::execute<InitializeProcThreadAttributeList> ");
    return false;
  }
  SECURITY_CAPABILITIES sc;
  sc.AppContainerSid = appcontainersid;
  sc.Capabilities = (cas.empty() ? NULL : cas.data());
  sc.CapabilityCount = static_cast<DWORD>(cas.size());
  sc.Reserved = 0;
  if (UpdateProcThreadAttribute(siex.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES, &sc, sizeof(sc),
                                nullptr, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"appcommand::execute<UpdateProcThreadAttribute> ");
    return false;
  }
  DWORD dwvalue = 1;
  if (islpac) {
    // ProcThreadAttributeAllApplicationPackagesPolicy
    // UpdateProcThreadAttribute(siex.lpAttributeList,0,)
    if (UpdateProcThreadAttribute(siex.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY_EX,
                                  &dwvalue, sizeof(dwvalue), nullptr, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"appcommand::execute<UpdateProcThreadAttribute> ");
      return false;
    }
  }
  for (auto &f : allowdirs) {
    AllowNameObjectAccess(appcontainersid, f.data(), SE_FILE_OBJECT, FILE_ALL_ACCESS);
  }

  for (auto &r : regdirs) {
    AllowNameObjectAccess(appcontainersid, r.data(), SE_REGISTRY_KEY, KEY_ALL_ACCESS);
  }

  DWORD createflags = EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT;
  if (visible == visible_t::newconsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (visible == visible_t::hide) {
    createflags |= CREATE_NO_WINDOW;
    siex.StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    siex.StartupInfo.wShowWindow = SW_HIDE;
  }
  bela::EscapeArgv ea;
  for (const auto &a : argv) {
    ea.Append(a);
  }
  if (CreateProcessW(string_nullable(path), ea.data(), nullptr, nullptr, FALSE, createflags, string_nullable(env),
                     string_nullable(cwd), reinterpret_cast<STARTUPINFOW *>(&siex), &pi) != TRUE) {
    ec = bela::make_system_error_code(L"appcommand::execute<CreateProcessW> ");
    return false;
  }
  pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}
} // namespace wsudo::exec