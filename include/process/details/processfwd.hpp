///
#ifndef PRIVEXEC_PROCESSFWD_HPP
#define PRIVEXEC_PROCESSFWD_HPP
#include <vector>
#include <optional>
#include <base.hpp>
#include <argvbuilder.hpp>

namespace priv {

template <typename T> const wchar_t *Castwstr(const T &t) {
  if (t.empty()) {
    return nullptr;
  }
  return t.data();
}

inline void CloseHandleEx(HANDLE &h) {
  if (h != INVALID_HANDLE_VALUE) {
    CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }
}

enum ProcessExecuteLevel {
  ProcessNone = -1,
  ProcessAppContainer = 0,
  ProcessMandatoryIntegrityControl,
  ProcessNoElevated,
  ProcessElevated,
  ProcessSystem,
  ProcessTrustedInstaller
};

enum visiblemode_t {
  VisibleNone, //
  VisibleNewConsole,
  VisibleHide
};

class process {
public:
  template <typename... Args> process(std::wstring_view app, Args... args) {
    std::initializer_list<std::wstring_view> av = {args...};
    argvbuilder ab;
    ab.assign(app);
    for (auto c : av) {
      ab.append(c);
    }
    cmd_ = ab.args();
  }
  process(const std::wstring &cmdline) : cmd_(cmdline) {}
  process(const process &) = delete;
  process &operator=(const process &) = delete;

  std::wstring &cwd() { return cwd_; }
  const std::wstring &cwd() const { return cwd_; }
  const std::wstring &message() const { return kmessage; } // error reason
  DWORD pid() const { return pid_; }
  //
  bool execute(int level);
  // known level
  bool execute();
  bool noelevatedexec();
  bool tiexec();
  bool elevatedexec();
  bool lowlevelexec();
  bool systemexec();
  //
  visiblemode_t visiblemode(visiblemode_t vm) {
    visible = vm;
    return visible;
  }

private:
  bool execwithtoken(HANDLE hToken, bool desktop = false);
  DWORD pid_{0};
  std::wstring cmd_;
  std::wstring cwd_;
  std::wstring kmessage;
  visiblemode_t visible{VisibleNone}; ///
};
// SECURITY_CAPABILITY_INTERNET_EXPLORER
using wid_t = WELL_KNOWN_SID_TYPE;
using widcontainer = std::vector<wid_t>;
using alloweddir_t = std::vector<std::wstring>;
class appcontainer {
public:
  using capabilities_t = std::vector<SID_AND_ATTRIBUTES>;
  template <typename... Args>
  appcontainer(std::wstring_view app, Args... args) {
    std::initializer_list<std::wstring_view> av = {args...};
    argvbuilder ab;
    ab.assign(app);
    for (auto c : av) {
      ab.append(c);
    }
    cmd_ = ab.args();
  }
  appcontainer(const std::wstring &cmdline) : cmd_(cmdline) {}
  appcontainer(const appcontainer &) = delete;
  appcontainer &operator=(const appcontainer &) = delete;
  ~appcontainer() {
    for (auto &c : ca) {
      if (c.Sid) {
        HeapFree(GetProcessHeap(), 0, c.Sid);
      }
    }
    if (appcontainersid != nullptr) {
      FreeSid(appcontainersid);
    }
  }

  std::wstring &cwd() { return cwd_; }
  const std::wstring &cwd() const { return cwd_; }
  DWORD pid() const { return pid_; }
  const capabilities_t &capabilities() const { return ca; }
  const std::wstring &message() const { return kmessage; } // error reason
  //
  bool initialize();
  bool initialize(const wid_t *begin, const wid_t *end);
  bool initialize(const std::wstring &appxml);
  bool initialize(const std::vector<std::wstring> &names);
  bool initializessid(const std::wstring &ssid);
  bool execute();

  //
  visiblemode_t visiblemode(visiblemode_t vm) {
    visible = vm;
    return visible;
  }
  bool enablelpac(bool enable) {
    lpac = enable;
    return lpac;
  }
  std::wstring &name() { return name_; };
  const std::wstring &name() const { return name_; }
  alloweddir_t &alloweddirs() { return alloweddirs_; }
  const alloweddir_t &alloweddirs() const { return alloweddirs_; }
  alloweddir_t &registries() { return registries_; }
  const alloweddir_t &registries() const { return registries_; }
  const std::wstring &strid() const { return strid_; }
  const std::wstring &conatinerfolder() const { return folder_; }

private:
  DWORD pid_{0};
  std::wstring name_;
  std::wstring cmd_;
  std::wstring cwd_;
  alloweddir_t alloweddirs_;
  alloweddir_t registries_;
  std::wstring strid_;
  std::wstring folder_;
  std::wstring kmessage;
  PSID appcontainersid{nullptr};
  capabilities_t ca;
  visiblemode_t visible{VisibleNone}; ///
  bool lpac{false};
};

/*++
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token.
Arguments: None.
Return Value:
TRUE - Caller has Administrators local group.
FALSE - Caller does not have Administrators local group. --
*/
inline bool IsUserAdministratorsGroup() {
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                &AdministratorsGroup)) {
    return false;
  }
  BOOL b = FALSE;
  if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
    b = FALSE;
  }
  FreeSid(AdministratorsGroup);
  return b == TRUE;
}

///
bool WellKnownFromAppmanifest(const std::wstring &file, widcontainer &sids);
} // namespace priv

#endif
