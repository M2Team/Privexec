////
#include <string>
#include <console/console.hpp>
#include <version.h>
#include <pe.hpp>
#include "wsudo.hpp"
#include "wsudoalias.hpp"

namespace wsudo {

//     -a          AppContainer
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
      [&](const wchar_t *arg, const wchar_t *s,
          const wchar_t *l) -> std::optional<std::wstring_view> {
    std::wstring_view v;
    if (!MatchEx(arg, s, l, v)) {
      // ok.
      return std::nullopt;
    }
    if (!v.empty()) {
      return std::make_optional<std::wstring_view>(v);
    }
    if (i + 1 < argc) {
      return std::make_optional<std::wstring_view>(argv[++i]);
    }
    message.assign(L"Flag '").append(arg).append(L"' missing  argument");
    return std::nullopt;
  };

  for (; i < argc; i++) {
    auto arg = argv[i];
    if (arg[0] != '-') {
      break;
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
    if (Match(arg, L"-n", L"--new-console")) {
      newconsole = true;
      continue;
    }
    if (Match(arg, L"--disable-alias")) {
      disablealias = true;
      continue;
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
    if (!message.empty()) {
      return AppFatal;
    }
    // AppLevel
    if (IsAppLevel(arg)) {
      continue;
    }
    message.assign(L"invalid argument '").append(arg).append(L"'");
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
   -V|--verbose        Make the operation more talkative
   -x|--appx           AppContainer AppManifest file path
   -c|--cwd            Use a working directory to launch the process.
   --disable-alias     Disable Privexec alias, By default, if Privexec exists alias, use it.

Select user can use the following flags:
   -a          AppContainer
   -M          Mandatory Integrity Control
   -U          No Elevated(UAC)
   -A          Administrator
   -S          System
   -T          TrustedInstaller
Example:
   wsudo -A "%SYSTEMROOT%/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile
   wsudo -T cmd

Buitin 'alias' command:
   wsudo alias add ehs "notepad %SYSTEMROOT%/System32/drivers/etc/hosts" "Edit Hosts"
   wsudo alias delete ehs
)";
  priv::Print(err ? priv::fc::Red : priv::fc::Cyan, L"wsudo \x2665 %s\n",
              kUsage);
}

// expand env
inline std::wstring ExpandEnv(const std::wstring &s) {
  auto len = ExpandEnvironmentStringsW(s.data(), nullptr, 0);
  if (len <= 0) {
    return s;
  }
  std::wstring s2(len + 1, L'\0');
  auto N = ExpandEnvironmentStringsW(s.data(), &s2[0], len + 1);
  s2.resize(N - 1);
  return s2;
}

bool AppExecuteSubsystemIsConsole(const std::wstring &cmd, bool verbose) {
  std::wstring xcmd;
  if (cmd.front() == L'"') {
    for (size_t i = 1; i < cmd.size(); i++) {
      if (cmd[i] == L'"' && cmd[i - 1] != L'\\') {
        if (i < 2) {
          return false;
        }
        xcmd.assign(cmd.data() + 1, i - 2);
      }
    }
  } else {
    auto pos = cmd.find(' ');
    if (pos == std::wstring::npos) {
      xcmd = cmd;
    } else {
      xcmd = cmd.substr(0, pos);
    }
  }
  if (verbose) {
    priv::Print(priv::fc::Yellow, L"* App real argv0 '%s'\n", xcmd);
  }
  std::wstring exe;
  if (!priv::FindExecutableImageEx(xcmd, exe)) {
    return false;
  }
  if (verbose) {
    priv::Print(priv::fc::Yellow, L"* App real path '%s'\n", exe);
  }
  return priv::PESubsystemIsConsole(exe);
}

int AppExecute(wsudo::AppMode &am) {
  std::wstring cmd(am.args[0]);
  if (!am.disablealias) {
    wsudo::AliasEngine ae;
    if (ae.Initialize()) {
      auto al = ae.Target(cmd);
      if (al) {
        if (am.verbose) {
          priv::Print(priv::fc::Yellow, L"* App alias '%s' expand to '%s'\n",
                      cmd, *al);
        }
        cmd.assign(*al);
      }
    }
  }
  auto cmdline = ExpandEnv(cmd);
  auto isconsole = AppExecuteSubsystemIsConsole(cmdline, am.verbose);
  if (am.verbose && isconsole) {
    priv::Print(priv::fc::Yellow, L"* App subsystem is console\n");
  }
  for (auto it = am.args.begin() + 1; it != am.args.end(); it++) {
    if (it->empty()) {
      cmdline.append(L" \"\"");
      continue;
    }
    if (it->find(L' ') != std::wstring_view::npos && it->front() != L'"') {
      cmdline.append(L" \"").append(it->data(), it->size()).append(L"\"");
    } else {
      cmdline.append(L" ").append(it->data(), it->size());
    }
  }
  if (am.verbose) {
    priv::Print(priv::fc::Yellow, L"* App real command '%s'\n", cmdline);
  }
  if (am.level == priv::ProcessAppContainer && !am.appx.empty()) {
    priv::appcontainer p(cmdline);
    if (!am.cwd.empty()) {
      p.cwd() = ExpandEnv(am.cwd.data());
    }
    auto appx = ExpandEnv(am.appx.data());
    p.newconsole(am.newconsole);
    if (!p.initialize(appx) || p.execute()) {
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
    return 0;
  }
  priv::process p(cmdline);
  p.newconsole(am.newconsole);
  if (!am.cwd.empty()) {
    p.cwd() = ExpandEnv(am.cwd.data());
  }
  priv::Print(priv::fc::Yellow, L"Command: %s\n", cmdline);
  if (p.execute(am.level)) {
    priv::Print(priv::fc::Green, L"new process is running: %d\n", p.pid());
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
  return AppExecute(am);
}