////
#include <bela/stdwriter.hpp>

#include <console/console.hpp>
#include <version.h>
#include <pe.hpp>
#include <argvbuilder.hpp>
#include <parseargv.hpp>
#include <Shellapi.h>
#include <Shlobj.h>
#include <apphelp.hpp>

//
#include "wsudo.hpp"
#include "wsudoalias.hpp"

void Version() {
  bela::FPrintF(stderr, L"\x1b[36mwsudo %s\x1b[0m\n", PRIVEXEC_BUILD_VERSION);
}

void Usage(bool err = false) {
  constexpr const wchar_t *kUsage =
      LR"(run the program with the specified permissions
usage: wsudo command args...
   -v|--version        print version and exit
   -h|--help           print help information and exit
   -u|--user           run as user (optional), support '-uX', '-u X', '--user=X', '--user X'
                       Supported user categories (Ignore case):
                       AppContainer    MIC       NoElevated
                       Administrator   System    TrustedInstaller

   -n|--new-console    Starts a separate window to run a specified program or command.
   -H|--hide           Hide child process window. not wait. (CREATE_NO_WINDOW)
   -w|--wait           Start application and wait for it to terminate.
   -V|--verbose        Make the operation more talkative
   -x|--appx           AppContainer AppManifest file path
   -c|--cwd            Use a working directory to launch the process.
   -e|--env            Set Environment Variable.
   -L|--lpac           Less Privileged AppContainer mode.
   --disable-alias     Disable Privexec alias, By default, if Privexec exists alias, use it.
   --appname           Set AppContainer Name

Select user can use the following flags:
   |-a  AppContainer    |-M  Mandatory Integrity Control|-U  No Elevated(UAC)|
   |-A  Administrator   |-S  System                     |-T  TrustedInstaller|
Example:
   wsudo -A "%SYSTEMROOT%/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile
   wsudo -T cmd
   wsudo -U -V --env CURL_SSL_BACKEND=schannel curl --verbose  -I https://nghttp2.org

Builtin 'alias' command:
   wsudo alias add ehs "notepad %SYSTEMROOT%/System32/drivers/etc/hosts" "Edit Hosts"
   wsudo alias delete ehs
)";
  char32_t sh = 0x1F496; //  💖
  bela::FPrintF(stderr, L"\x1b[%dmwsudo %c %s %s\x1b[0m\n", err ? 31 : 36, sh,
                PRIV_VERSION_MAIN, kUsage);
}

namespace wsudo {

//    -a          AppContainer
//    -M          Mandatory Integrity Control
//    -U          No Elevated(UAC)
//    -A          Administrator
//    -S          System
//    -T          TrustedInstaller
bool IsAppLevel(std::wstring_view k, int &level) {
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
  return false;
}

// ParseArgv todo
int AppMode::ParseArgv(int argc, wchar_t **argv) {
  av::ParseArgv pa(argc, argv, true);
  pa.Add(L"version", av::no_argument, L'v')
      .Add(L"help", av::no_argument, L'h')
      .Add(L"user", av::required_argument, L'u')
      .Add(L"new-console", av::no_argument, L'n')
      .Add(L"hide", av::no_argument, L'H')
      .Add(L"wait", av::no_argument, L'w')
      .Add(L"verbose", av::no_argument, L'V')
      .Add(L"appx", av::required_argument, L'x')
      .Add(L"cwd", av::required_argument, L'c')
      .Add(L"env", av::required_argument, L'e')
      .Add(L"lpac", av::no_argument, L'L')
      .Add(L"", av::no_argument, L'a')
      .Add(L"", av::no_argument, L'M')
      .Add(L"", av::no_argument, L'U')
      .Add(L"", av::no_argument, L'A')
      .Add(L"", av::no_argument, L'S')
      .Add(L"", av::no_argument, L'T')
      .Add(L"appname", av::required_argument, 1001)
      .Add(L"disable-alias", av::no_argument, 1002);
  av::error_code ec;
  auto result = pa.Execute(
      [&](int val, const wchar_t *va, const wchar_t *) {
        switch (val) {
        case 'v':
          Version();
          exit(0);
        case 'h':
          Usage();
          exit(0);
        case 'u':
          if (!IsAppLevel(va, level)) {
            bela::FPrintF(stderr, L"wsudo unsupport user level: %s\n", va);
            return false;
          }
          break;
        case 'n':
          visible = priv::VisibleNewConsole;
          break;
        case 'H':
          visible = priv::VisibleHide;
          break;
        case 'w':
          wait = true;
          break;
        case 'V':
          verbose = true;
          break;
        case 'x':
          appx = va;
          break;
        case 'c':
          cwd = va;
          break;
        case 'e':
          envctx.Append(va);
          break;
        case 'L':
          lpac = true;
          break;
        case 'a':
          level = priv::ProcessAppContainer;
          break;
        case 'M':
          level = priv::ProcessMandatoryIntegrityControl;
          break;
        case 'U':
          level = priv::ProcessNoElevated;
          break;
        case 'A':
          level = priv::ProcessElevated;
          break;
        case 'S':
          level = priv::ProcessSystem;
          break;
        case 'T':
          level = priv::ProcessTrustedInstaller;
          break;
        case 1001:
          appname = va;
          break;
        case 1002:
          disablealias = true;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!result) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo ParseArgv: %s\x1b[0m\n", ec.message);
    return 1;
  }
  /// Copy TO
  args = pa.UnresolvedArgs();
  return 0;
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
    bela::FPrintF(stderr, L"\x1b[01;33m* App cwd: %s\x1b[0m\n", cwd);
  }
  if (!appx.empty()) {
    bela::FPrintF(stderr, L"\x1b[01;33m* App AppContainer Manifest: %s\x1b[0m\n",
                  appx);
  }
  bela::FPrintF(stderr, L"\x1b[01;33m* App Launcher level: %s\x1b[0m\n",
                AppSLevel(level));
  if (disablealias) {
    bela::FPrintF(stderr, L"\x1b[01;33m* App Alias is disabled\x1b[0m\n");
  }
}
} // namespace wsudo

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
  priv::Verbose(verbose, L"\x1b[01;33m* App real path '%s'\x1b[0m\n", exe);
  return priv::PESubsystemIsConsole(exe);
}

int AppWait(DWORD pid) {
  if (pid == 0) {
    return 0;
  }
  auto hProcess =
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
  if (hProcess == INVALID_HANDLE_VALUE) {
    auto ec = base::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable open process '%s'\x1b[0m\n", ec.message);
    return -1;
  }
  SetConsoleCtrlHandler(nullptr, TRUE);
  if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) {
    auto ec = base::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable wait process '%s'\x1b[0m\n", ec.message);
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(hProcess);
    return -1;
  }
  SetConsoleCtrlHandler(nullptr, FALSE);
  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) != TRUE) {
    auto ec = base::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable get process exit code '%s'\x1b[0m\n", ec.message);
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
  priv::Verbose(verbose, L"\x1b[01;33m* App alias '%s' expand to '%s'\x1b[0m\n", cmd, *al);
  return ExpandEnv(*al);
}

int AppExecuteAppContainer(wsudo::AppMode &am) {
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
    am.Verbose(L"\x1b[01;33m* App subsystem is console, %s\x1b[0m\n",
               am.Visible());
  }
  for (size_t i = 1; i < am.args.size(); i++) {
    ab.append(am.args[i]);
  }
  am.Verbose(L"\x1b[01;33m* App real command '%s'\x1b[0m\n", argv0);
  am.envctx.Apply([&](const std::wstring &k, const std::wstring &v) {
    am.Verbose(L"\x1b[01;33m* App apply env '%s' = '%s'\x1b[0m\n", k, v);
  });
  priv::appcontainer p(ab.args());
  if (!am.cwd.empty()) {
    p.cwd() = ExpandEnv(am.cwd.data());
  }
  if (!am.appname.empty()) {
    p.name().assign(am.appname);
  }
  auto appx = ExpandEnv(am.appx.data());
  p.visiblemode(am.visible);
  p.enablelpac(am.lpac);
  if (am.lpac) {
    am.Verbose(L"\x1b[01;33m* AppContainer: Less Privileged AppContainer Is "
               L"Enabled.\x1b[0m\n");
  }
  if (!am.appname.empty()) {
    p.name().assign(am.appname);
  }
  bela::FPrintF(stderr, L"\x1b[01;32mCommand: %s\x1b[0m\n", ab.args());
  bool ok = false;
  if (am.appx.empty()) {
    if (am.lpac) {
      bela::FPrintF(stderr, L"\x1b[31mAppContainer: LPAC mode is set but will "
                            L"not take effect. Appmanifest not set.\x1b[0m\n");
    }
    ok = p.initialize();
  } else {
    auto appx = ExpandEnv(am.appx.data());
    ok = p.initialize(appx);
  }
  if (!ok) {
    auto ec = base::make_system_error_code();
    bela::FPrintF(stderr,
                  L"\x1b[31minitialize appconatiner process  last error %d  "
                  L"(%s): %s\x1b[0m\n",
                  ec.code, p.message(), ec.message);
    return 1;
  }
  if (!p.execute()) {
    auto ec = base::make_system_error_code();
    if (p.message().empty()) {
      bela::FPrintF(
          stderr,
          L"\x1b[31mcreate appconatiner process  last error %d : %s\x1b[0m\n",
          ec.code, ec.message);
    } else {
      bela::FPrintF(stderr,
                    L"\x1b[31mcreate appconatiner process  last error %d  "
                    L"(%s): %s\x1b[0m\n",
                    ec.code, p.message(), ec.message);
    }
    return 1;
  }
  bela::FPrintF(
      stderr,
      L"\x1b[01;32mnew appcontainer process is running: %d\nsid: %s\x1b[0m\n",
      p.pid(), p.strid());
  if (waitable) {
    return AppWait(p.pid());
  }
  return 0;
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
    am.Verbose(L"\x1b[01;33m* App subsystem is console, %s\x1b[0m\n",
               am.Visible());
  }

  for (size_t i = 1; i < am.args.size(); i++) {
    ab.append(am.args[i]);
  }
  am.Verbose(L"\x1b[01;33m* App real command '%s'\x1b[0m\n", argv0);
  am.envctx.Apply([&](const std::wstring &k, const std::wstring &v) {
    am.Verbose(L"\x1b[01;33m* App apply env '%s' = '%s'\x1b[0m\n", k, v);
  });
  priv::process p(ab.args());
  p.visiblemode(am.visible);
  if (!am.cwd.empty()) {
    p.cwd() = ExpandEnv(am.cwd.data());
  }
  bela::FPrintF(stderr, L"\x1b[01;32mCommand: %s\x1b[0m\n", ab.args());
  if (p.execute(am.level)) {
    bela::FPrintF(stderr, L"\x1b[01;32mnew process is running: %d\x1b[0m\n",
                  p.pid());
    if (waitable) {
      return AppWait(p.pid());
    }
    return 0;
  }
  auto ec = base::make_system_error_code();
  if (p.message().empty()) {
    bela::FPrintF(stderr,
                  L"\x1b[31mcreate process  last error %d : %s\x1b[0m\n",
                  ec.code, ec.message);
  } else {
    bela::FPrintF(stderr,
                  L"\x1b[31mcreate process  last error %d  (%s): %s\x1b[0m\n",
                  ec.code, p.message(), ec.message);
  }
  return 1;
}

int wmain(int argc, wchar_t **argv) {
  wsudo::AppMode am;
  if (am.ParseArgv(argc, argv) != 0) {
    return 1;
  }
  if (am.args.empty()) {
    bela::FPrintF(stderr,
                  L"\x1b[31mwsudo missing command %s see usage:\x1b[0m\n",
                  am.message);
    Usage(true);
    return 1;
  }
  if (am.args[0] == L"alias") {
    return wsudo::AliasSubcmd(am.args, am.verbose);
  }
  am.Verbose();
  priv::dotcom_global_initializer di;
  if (am.level == priv::ProcessAppContainer) {
    return AppExecuteAppContainer(am);
  }
  return AppExecute(am);
}
