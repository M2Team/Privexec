///
#ifndef PRIVEXEC_PROCESS_FWD_HPP
#define PRIVEXEC_PROCESS_FWD_HPP
#include <string_view>
#include <vector>
#include <bela/base.hpp>
#include <bela/span.hpp>
#include <bela/escapeargv.hpp>

namespace priv {
// process execute level
enum class ExecLevel : int {
  None = -1,
  AppContainer,
  MIC,
  NoElevated,
  Elevated,
  System,
  TrustedInstaller
};
enum class VisibleMode : int {
  None = 0, // not set
  NewConsole,
  Hide
};

constexpr inline const wchar_t *EmptyNull(std::wstring_view s) {
  return s.empty() ? nullptr : s.data();
}

inline void CloseHandleEx(HANDLE &h) {
  if (h != INVALID_HANDLE_VALUE) {
    ::CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }
}

class Process {
public:
  Process() = default;
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  Process(std::wstring_view cmd_) : cmd(cmd_) {}
  template <typename... Args> Process(std::wstring_view app, Args... args) {
    std::wstring_view svvv = {app, args...};
    bela::EscapeArgv ea;
    ea.AssignFull(svv);
    cmd = ea.sv();
  }
  std::wstring &Commandline() { return cmd; }
  std::wstring_view Chdir(std::wstring_view dir) {
    cwd = dir;
    return cwd;
  }
  VisibleMode ChangeVisibleMode(VisibleMode visible_) {
    visible = visible_;
    return visible;
  }
  bool Exec(ExecLevel el = ExecLevel::None);
  const std::wstring_view Message() const { return kmessage; }
  DWORD PID() const { return pid; }

private:
  bool ExecNone();
  bool ExecLow();
  bool ExecNoElevated();
  bool ExecElevated();
  bool ExecSystem();
  bool ExecTI();
  bool ExecWithToken(HANDLE hToken, bool desktop);
  bool ExecWithAppContainer();
  std::wstring cmd;
  std::wstring cwd;
  std::wstring kmessage;
  DWORD pid{0};
  VisibleMode visible{VisibleMode::None}; ///
};

using wid_t = WELL_KNOWN_SID_TYPE;
using widcontainer_t = std::vector<wid_t>;
using alloweddir_t = std::vector<std::wstring>;
using capabilities_t = std::vector<SID_AND_ATTRIBUTES>;
bool MergeFromAppManifest(std::wstring_view file,
                          std::vector<std::wstring> &caps);

class AppContainer {
public:
  AppContainer() = default;
  AppContainer(std::wstring_view cmd_) : cmd(cmd_) {}
  template <typename... Args>
  AppContainer(std::wstring_view app, Args... args) {
    std::wstring_view svvv = {app, args...};
    bela::EscapeArgv ea;
    ea.AssignFull(svv);
    cmd = ea.sv();
  }
  std::wstring &Commandline() { return cmd; }
  std::wstring_view Chdir(std::wstring_view dir) {
    cwd = dir;
    return cwd;
  }
  std::wstring_view Name() const { return name; }
  std::wstring_view Name(std::wstring_view n) {
    name = n;
    return name;
  }
  VisibleMode ChangeVisibleMode(VisibleMode visible_) {
    visible = visible_;
    return visible;
  }

  alloweddir_t &AllowedDirs() { return alloweddirs; }
  const alloweddir_t &AllowedDirs() const { return alloweddirs; }
  alloweddir_t &Registries() { return registries; }
  const alloweddir_t &Registries() const { return registries; }
  const std::wstring &SSID() const { return strid; }
  const std::wstring &ConatinerFolder() const { return folder; }
  bool EnableLPAC(bool enable) {
    lpac = enable;
    return lpac;
  }
  DWORD PID() const { return pid; }
  const std::wstring_view Message() const { return kmessage; }
  bool InitializeNone(); // default;
  bool Initialize(const bela::Span<std::wstring> caps);
  bool Initialize(const bela::Span<wid_t> wids);
  bool InitializeFile(std::wstring_view file);
  bool Exec();

private:
  std::wstring cmd;
  std::wstring cwd;
  std::wstring name;
  alloweddir_t alloweddirs;
  alloweddir_t registries;
  std::wstring strid;
  std::wstring folder;
  std::wstring kmessage;
  PSID appcontainersid{nullptr};
  capabilities_t ca;
  VisibleMode visible{VisibleMode::None}; ///
  DWORD pid;
  bool lpac{false};
};

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
bool GetNoElevatedToken(PHANDLE hNewToken);
} // namespace priv

#endif