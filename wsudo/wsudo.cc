////
#include <string>
#include <console/console.hpp>
#include <version.h>
#include <pe.hpp>
#include <argvbuilder.hpp>
#include <Shellapi.h>
#include <Shlobj.h>
#include "wsudo.hpp"
#include "wsudoalias.hpp"

namespace wsudo {

//    -a          AppContainer
//    -M          Mandatory Integrity Control
//    -U          No Elevated(UAC)
//    -A          Administrator
//    -S          System
//    -T          TrustedInstaller
inline bool AppMode::IsAppLevel(const wchar_t *arg) {
  auto l = wcslen(arg);
  if (l != 2) {
    return false;
  }
  switch (arg[1]) {
  case L'a':
    level = priv::ProcessAppContainer;
    break;
  case L'M':
    level = priv::ProcessMandatoryIntegrityControl;
    break;
  case L'U':
    level = priv::ProcessNoElevated;
    break;
  case L'A':
    level = priv::ProcessElevated;
    break;
  case L'S':
    level = priv::ProcessSystem;
    break;
  case L'T':
    level = priv::ProcessTrustedInstaller;
    break;
  default:
    return false;
  }
  return true;
}

bool AppMode::IsAppLevelKey(std::wstring_view k) {
  static struct {
    const wchar_t *s;
    int level;
  } userlevels[] = {
      //
      {L"appcontainer", priv::ProcessAppContainer},
      {L"mic", priv::ProcessMandatoryIntegrityControl},
      {L"noelevated", priv::ProcessNoElevated},
      {L"administrator", priv::ProcessElevated},
      {L"system", priv::ProcessSystem},
      {L"trustedinstaller", priv::ProcessTrustedInstaller}
      //
  };
  for (const auto &u : userlevels) {
    auto l = wcslen(u.s);
    if (l == k.size() && _wcsnicmp(u.s, k.data(), l) == 0) {
      level = u.level;
      return true;
    }
  }
  message.assign(L"unsupport user level '")
      .append(k.data(), k.size())
      .append(L"'");
  return false;
}
// ParseArgv
int AppMode::ParseArgv(int argc, wchar_t **argv) {
  int i = 1;
  // std::wstring_view ua, cwd, appx;
  auto MatchInternal =
      [&](const wchar_t *a, const wchar_t *s,
          const wchar_t *l) -> std::optional<std::wstring_view> {
    std::wstring_view v;
    if (!MatchEx(a, s, l, v)) {
      // ok.
      return std::nullopt;
    }
    if (!v.empty()) {
      return std::make_optional<std::wstring_view>(v);
    }
    if (i + 1 < argc) {
      return std::make_optional<std::wstring_view>(argv[++i]);
    }
    message.assign(L"Flag '").append(a).append(L"' missing  argument");
    return std::nullopt;
  };

  for (; i < argc; i++) {
    auto arg = argv[i];
    if (arg[0] != '-') {
      if (!envctx.Appendable(arg)) {
        break;
      }
      continue;
    }
    if (Match(arg, L"-?", L"-h", L"--help")) {
      return AppHelp;
    }
    if (Match(arg, L"-v", L"--version")) {
      return AppVersion;
    }
    if (Match(arg, L"-V", L"--verbose")) {
      verbose = true;
      continue;
    }
    if (Match(arg, L"-w", L"--wait")) {
      wait = true;
      continue;
    }
    if (Match(arg, L"-n", L"--new-console")) {
      visible = priv::VisibleNewConsole;
      continue;
    }
    if (Match(arg, L"-H", L"--hide")) {
      visible = priv::VisibleHide;
      continue;
    }
    if (Match(arg, L"-L", L"--lpac")) {
      lpac = true;
      continue;
    }
    if (Match(arg, L"--disable-alias")) {
      disablealias = true;
      continue;
    }

    auto es = MatchInternal(arg, L"-e", L"--env");
    if (es) {
      envctx.Append(*es);
      continue;
    }
    if (!message.empty()) {
      return AppFatal;
    }
    auto ul = MatchInternal(arg, L"-u", L"--user");
    if (ul) {
      if (!IsAppLevelKey(*ul)) {
        return AppFatal;
      }
      continue;
    }
    if (!message.empty()) {
      return AppFatal;
    }
    auto cwd_ = MatchInternal(arg, L"-c", L"--cwd");
    if (cwd_) {
      cwd = *cwd_;
      continue;
    }
    if (!message.empty()) {
      return AppFatal;
    }
    auto ax = MatchInternal(arg, L"-x", L"--appx");
    if (ax) {
      appx = *ax;
      continue;
    }
    auto an = MatchInternal(arg, L"--appname", L"--appname");
    if (an) {
      appname = *an;
      continue;
    }
    auto aid = MatchInternal(arg, L"--appsid", L"--appsid");
    if (aid) {
      appsid = *aid;
      continue;
    }
    if (!message.empty()) {
      return AppFatal;
    }
    // AppLevel
    if (IsAppLevel(arg)) {
      continue;
    }
    message.assign(L"unsupport flag '").append(arg).append(L"'");
    return AppFatal;
  }
  for (; i < argc; i++) {
    args.push_back(argv[i]);
  }
  return AppNone;
}

const wchar_t *AppSLevel(int l) {
  static struct {
    const wchar_t *s;
    int level;
  } userlevels[] = {
      //
      {L"AppContainer", priv::ProcessAppContainer},
      {L"Mandatory Integrity Control", priv::ProcessMandatoryIntegrityControl},
      {L"NoElevated", priv::ProcessNoElevated},
      {L"Administrator", priv::ProcessElevated},
      {L"System", priv::ProcessSystem},
      {L"TrustedInstaller", priv::ProcessTrustedInstaller}
      //
  };
  for (const auto &u : userlevels) {
    if (u.level == l) {
      return u.s;
    }
  }
  return L"Unkown";
}

void AppMode::Verbose() {
  if (!verbose) {
    return;
  }
  if (!cwd.empty()) {
    priv::Print(priv::fc::Yellow, L"* App cwd: %s\n", cwd.data());
  }
  if (!appx.empty()) {
    priv::Print(priv::fc::Yellow, L"* App AppContainer Manifest: %s\n",
                appx.data());
  }
  priv::Print(priv::fc::Yellow, L"* App Launcher level: %s\n",
              AppSLevel(level));
  if (disablealias) {
    priv::Print(priv::fc::Yellow, L"* App Alias is disabled\n");
  }
}
} // namespace wsudo

void Version() {
  priv::Print(priv::fc::Cyan, L"wsudo %s\n", PRIVEXEC_BUILD_VERSION);
}

void Usage(bool err = false) {
  constexpr const wchar_t *kUsage =
      LR"(run the program with the specified permissions
usage: wsudo command args....
   -v|--version        print version and exit
   -h|--help           print help information and exit
   -u|--user           run as user (optional), support '-uX', '-u X', '--user=X', '--user X'
                       Supported user categories (Ignore case):
                       AppContainer  MIC
                       NoElevated    Administrator
                       System        TrustedInstaller

   -n|--new-console    Starts a separate window to run a specified program or command.
   -H|--hide           Hide child process window. not wait. (CREATE_NO_WINDOW)
   -w|--wait           Start application and wait for it to terminate.
   -V|--verbose        Make the operation more talkative
   -x|--appx           AppContainer AppManifest file path
   -c|--cwd            Use a working directory to launch the process.
   -e|--env            Set Environment Variable.
   -L|--lpac           Less Privileged AppContainer mode.
   --disable-alias     Disable Privexec alias, By default, if Privexec exists alias, use it.
   --appsid               Set AppContainer SID
   --appname          Set AppContainer Name

Select user can use the following flags:
   -a                  AppContainer
   -M                  Mandatory Integrity Control
   -U                  No Elevated(UAC)
   -A                  Administrator
   -S                  System
   -T                  TrustedInstaller
Example:
   wsudo -A "%SYSTEMROOT%/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile
   wsudo -T cmd
   wsudo -U -V CURL_SSL_BACKEND=schannel curl --verbose  -I https://nghttp2.org

Builtin 'alias' command:
   wsudo alias add ehs "notepad %SYSTEMROOT%/System32/drivers/etc/hosts" "Edit Hosts"
   wsudo alias delete ehs
)";
  priv::Print(err ? priv::fc::Red : priv::fc::Cyan, L"wsudo \x2665 %s\n",
              kUsage);
}

// expand env
inline std::wstring ExpandEnv(std::wstring_view s) {
  auto len = ExpandEnvironmentStringsW(s.data(), nullptr, 0);
  if (len <= 0) {
    return std::wstring(s);
  }
  std::wstring s2(len + 1, L'\0');
  auto N = ExpandEnvironmentStringsW(s.data(), &s2[0], len + 1);
  s2.resize(N - 1);
  return s2;
}

bool AppExecuteSubsystemIsConsole(const std::wstring &cmd, bool aliasexpand,
                                  bool verbose) {
  if (!aliasexpand) {
    std::wstring exe;
    if (!priv::FindExecutableImageEx(cmd, exe)) {
      return false;
    }
    priv::Verbose(verbose, L"* App real argv0 '%s'\n", exe);
    return priv::PESubsystemIsConsole(exe);
  }
  int Argc;
  auto Argv = CommandLineToArgvW(cmd.data(), &Argc);
  if (Argc == 0) {
    return false;
  }
  priv::Verbose(verbose, L"* App real argv0 '%s'\n", Argv[0]);
  std::wstring exe;
  if (!priv::FindExecutableImageEx(Argv[0], exe)) {
    LocalFree(Argv);
    return false;
  }
  LocalFree(Argv);
  priv::Verbose(verbose, L"* App real path '%s'\n", exe);
  return priv::PESubsystemIsConsole(exe);
}

int AppWait(DWORD pid) {
  if (pid == 0) {
    return 0;
  }
  auto hProcess =
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
  if (hProcess == INVALID_HANDLE_VALUE) {
    auto ec = priv::error_code::lasterror();
    priv::Print(priv::fc::Red, L"unable open process '%s'\n", ec.message);
    return -1;
  }
  if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) {
    auto ec = priv::error_code::lasterror();
    priv::Print(priv::fc::Red, L"unable wait process '%s'\n", ec.message);
    CloseHandle(hProcess);
    return -1;
  }
  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) != TRUE) {
    auto ec = priv::error_code::lasterror();
    priv::Print(priv::fc::Red, L"unable get process exit code '%s'\n",
                ec.message);
  }
  CloseHandle(hProcess);
  return static_cast<int>(exitcode);
}

std::wstring ExpandArgv0(std::wstring_view argv0, bool disablealias,
                         bool verbose, bool &aliasexpand) {
  aliasexpand = false;
  if (disablealias) {
    return ExpandEnv(argv0);
  }
  wsudo::AliasEngine ae;
  if (!ae.Initialize()) {
    return ExpandEnv(argv0);
  }
  auto cmd = std::wstring(argv0);
  auto al = ae.Target(cmd);
  if (!al) {
    return ExpandEnv(argv0);
  }
  aliasexpand = true;
  priv::Verbose(verbose, L"* App alias '%s' expand to '%s'\n", cmd, *al);
  return ExpandEnv(*al);
}

int AppExecute(wsudo::AppMode &am) {
  priv::argvbuilder ab;
  bool aliasexpand = false;
  auto argv0 =
      ExpandArgv0(am.args[0], am.disablealias, am.verbose, aliasexpand);
  if (aliasexpand) {
    ab.assign_no_escape(argv0);
  } else {
    ab.assign(argv0);
  }
  auto isconsole = AppExecuteSubsystemIsConsole(argv0, aliasexpand, am.verbose);
  auto elevated = priv::IsUserAdministratorsGroup();
  bool waitable = false;
  if (!elevated && am.level == priv::ProcessElevated &&
      am.visible == priv::VisibleNone) {
    am.visible = priv::VisibleNewConsole; /// change visible
  }
  if (am.visible == priv::VisibleNone && isconsole || am.wait) {
    waitable = true;
  }
  // UAC Elevated

  if (isconsole) {
    priv::Verbose(am.verbose, L"* App subsystem is console, %s\n",
                  am.Visible());
  }

  for (size_t i = 1; i < am.args.size(); i++) {
    ab.append(am.args[i]);
  }
  priv::Verbose(am.verbose, L"* App real command '%s'\n", argv0);
  am.envctx.Apply([&](const std::wstring &k, const std::wstring &v) {
    priv::Verbose(am.verbose, L"* App apply env '%s' = '%s'\n", k, v);
  });
  if (am.level == priv::ProcessAppContainer && !am.appx.empty()) {
    priv::appcontainer p(ab.args());
    if (!am.cwd.empty()) {
      p.cwd() = ExpandEnv(am.cwd.data());
    }
    auto appx = ExpandEnv(am.appx.data());
    p.visiblemode(am.visible);
    p.enablelpac(am.lpac);
    if (am.lpac) {
      priv::Verbose(
          am.verbose,
          L"* AppContainer: Less Privileged AppContainer Is Enabled.\n");
    }
    priv::Print(priv::fc::Yellow, L"Command: %s\n", ab.args());
    if (!p.initialize(appx) || !p.execute()) {
      auto ec = priv::error_code::lasterror();
      if (p.message().empty()) {
        priv::Print(priv::fc::Red,
                    L"create appconatiner process  last error %d : %s\n",
                    ec.code, ec.message);
      } else {
        priv::Print(priv::fc::Red,
                    L"create appconatiner process  last error %d  (%s): %s\n",
                    ec.code, p.message(), ec.message);
      }
      return 1;
    }
    priv::Print(priv::fc::Green, L"new appcontainer process is running: %d\n",
                p.pid());
    if (waitable) {
      return AppWait(p.pid());
    }
    return 0;
  }
  if (am.level == priv::ProcessAppContainer && am.lpac) {
    priv::Print(priv::fc::Yellow, L"AppContainer: LPAC mode is set but will "
                                  L"not take effect. Appmanifest not set.\n");
  }
  priv::process p(ab.args());
  p.visiblemode(am.visible);
  if (!am.cwd.empty()) {
    p.cwd() = ExpandEnv(am.cwd.data());
  }
  priv::Print(priv::fc::Yellow, L"Command: %s\n", ab.args());
  if (p.execute(am.level)) {
    priv::Print(priv::fc::Green, L"new process is running: %d\n", p.pid());
    if (waitable) {
      return AppWait(p.pid());
    }
    return 0;
  }
  auto ec = priv::error_code::lasterror();
  if (p.message().empty()) {
    priv::Print(priv::fc::Red, L"create process  last error %d : %s\n", ec.code,
                ec.message);
  } else {
    priv::Print(priv::fc::Red, L"create process  last error %d  (%s): %s\n",
                ec.code, p.message(), ec.message);
  }
  return 1;
}

class DotComInitialize {
public:
  DotComInitialize() {
    if (FAILED(CoInitialize(NULL))) {
      throw std::runtime_error("CoInitialize failed");
    }
  }
  ~DotComInitialize() { CoUninitialize(); }
};

int wmain(int argc, wchar_t **argv) {
  wsudo::AppMode am;
  switch (am.ParseArgv(argc, argv)) {
  case wsudo::AppFatal:
    priv::Print(priv::fc::Red, L"wsudo parse argv failed: %s\n", am.message);
    return 1;
  case wsudo::AppHelp:
    Usage();
    return 0;
  case wsudo::AppVersion:
    Version();
    return 0;
  default:
    break;
  }
  if (am.args.empty()) {
    priv::Print(priv::fc::Red, L"wsudo missing command %s see usage:\n",
                am.message);
    Usage(true);
    return 1;
  }
  if (am.args[0] == L"alias") {
    return wsudo::AliasSubcmd(am.args, am.verbose);
  }
  am.Verbose();
  DotComInitialize dotcom;
  return AppExecute(am);
}
