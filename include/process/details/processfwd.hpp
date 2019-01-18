///
#ifndef PRIVEXEC_PROCESSFWD_HPP
#define PRIVEXEC_PROCESSFWD_HPP

#include <string_view>
#include <string>
#include <vector>
#include <optional>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif

namespace priv {

// final_act
// https://github.com/Microsoft/GSL/blob/ebe7ebfd855a95eb93783164ffb342dbd85cbc27/include/gsl/gsl_util#L85-L89
template <class F> class final_act {
public:
  explicit final_act(F f) noexcept : f_(std::move(f)), invoke_(true) {}

  final_act(final_act &&other) noexcept
      : f_(std::move(other.f_)), invoke_(other.invoke_) {
    other.invoke_ = false;
  }

  final_act(const final_act &) = delete;
  final_act &operator=(const final_act &) = delete;

  ~final_act() noexcept {
    if (invoke_)
      f_();
  }

private:
  F f_;
  bool invoke_;
};

// finally() - convenience function to generate a final_act
template <class F> inline final_act<F> finally(const F &f) noexcept {
  return final_act<F>(f);
}

template <class F> inline final_act<F> finally(F &&f) noexcept {
  return final_act<F>(std::forward<F>(f));
}

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
    if (app.find(L' ') != app.end()) {
      cmd_.assign(L"\"").append(app.data(), app.size()).append(L"\" ");
    } else {
      cmd_.assign(app.data(), app.size()).append(L" ");
    }
    for (const auto &a = av) {
      if (c.empty()) {
        cmd_.append(L"\"\" ");
        continue;
      }
      if (c.find(L' ') != c.end()) {
        cmd_.assign(L"\"").append(c.data(), c.size()).append(L"\" ");
      } else {
        cmd_.assign(c.data(), c.size()).append(L" ");
      }
    }
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
    if (app.find(L' ') != app.end()) {
      cmd_.assign(L"\"").append(app.data(), app.size()).append(L"\" ");
    } else {
      cmd_.assign(app.data(), app.size()).append(L" ");
    }
    for (const auto &a = av) {
      if (c.empty()) {
        cmd_.append(L"\"\" ");
        continue;
      }
      if (c.find(L' ') != c.end()) {
        cmd_.assign(L"\"").append(c.data(), c.size()).append(L"\" ");
      } else {
        cmd_.assign(c.data(), c.size()).append(L" ");
      }
    }
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
