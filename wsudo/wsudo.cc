////
#include <bela/terminal.hpp>
#include <bela/match.hpp>
#include <bela/path.hpp>
#include <bela/pe.hpp>
#include <bela/parseargv.hpp>
#include <bela/simulator.hpp>
#include <baseversion.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <apphelp.hpp>
//
#include "wsudo.hpp"
#include "wsudoalias.hpp"

void Version() {
  //
  bela::FPrintF(stderr, L"\x1b[36mwsudo %s\x1b[0m\n", PRIVEXEC_VERSION);
}

void Usage(bool err = false) {
  constexpr const wchar_t *kUsage = LR"(run the program with the specified permissions
usage: wsudo command args...
   -v|--version        print version and exit
   -h|--help           print help information and exit
   -V|--verbose        Make the operation more talkative
   -c|--cwd            Use a working directory to launch the process.
   -e|--env            Use specific environment variables to start child processes.
   -n|--nui            Starts a separate window to run a specified program or command.
   -H|--hide           Hide child process window. not wait. (CREATE_NO_WINDOW)
   -w|--wait           Start application and wait for it to terminate.
   -u|--user           run as user (optional), support '-uX', '-u X', '--user=X', '--user X'
                       Supported user categories (Ignore case):
                       AppContainer    MIC       NoElevated
                       Administrator   System    TrustedInstaller

   -x|--appx           AppContainer AppManifest file path
   -L|--lpac           Less Privileged AppContainer mode.
   --disable-alias     Disable Privexec alias, By default, if Privexec exists alias, use it.
   --appid             Set AppContainer ID name (compatible --appname)

Select user can use the following flags:
   -a|--appcontainer   AppContainer
   -M|--mic            Mandatory Integrity Control
   -U|--standard       Standard user no elevated (UAC)
   -A|--administrator  Administrator
   -S|--system         System
   -T|--ti             TrustedInstaller

Example:
   wsudo -A "%SYSTEMROOT%/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile
   wsudo -T cmd
   wsudo -U -V -eCURL_SSL_BACKEND=schannel curl --verbose  -I https://nghttp2.org
   wsudo -U -V CURL_SSL_BACKEND=schannel curl --verbose  -I https://nghttp2.org

Builtin 'alias' command:
   wsudo alias add ehs "notepad %SYSTEMROOT%/System32/drivers/etc/hosts" "Edit Hosts"
   wsudo alias delete ehs
)";
  char32_t sh = 0x1F496; //  💖
  bela::FPrintF(stderr, L"\x1b[%dmwsudo %c %d.%d %s\x1b[0m\n", err ? 31 : 36, sh, PRIVEXEC_VERSION_MAJOR,
                PRIVEXEC_VERSION_MINOR, kUsage);
}

namespace wsudo {
bool IsDebugMode = false;
bool IsAppLevel(std::wstring_view k, wsudo::exec::privilege_t &level) {
  static struct {
    const std::wstring_view s;
    wsudo::exec::privilege_t level;
  } userlevels[] = {
      //
      {L"appcontainer", wsudo::exec::privilege_t::appcontainer},
      {L"mic", wsudo::exec::privilege_t::mic},
      {L"noelevated", wsudo::exec::privilege_t::standard},
      {L"administrator", wsudo::exec::privilege_t::elevated},
      {L"system", wsudo::exec::privilege_t::system},
      {L"trustedinstaller", wsudo::exec::privilege_t::trustedinstaller}
      //
  };
  for (const auto &u : userlevels) {
    if (bela::EqualsIgnoreCase(u.s, k)) {
      level = u.level;
      return true;
    }
  }
  return false;
}

const std::wstring_view AppSLevel(const wsudo::exec::privilege_t level) {
  constexpr static struct {
    const wchar_t *s;
    wsudo::exec::privilege_t level;
  } userlevels[] = {
      //
      {L"AppContainer", wsudo::exec::privilege_t::appcontainer},
      {L"Mandatory Integrity Control", wsudo::exec::privilege_t::mic},
      {L"Standard", wsudo::exec::privilege_t::standard},
      {L"Administrator", wsudo::exec::privilege_t::elevated},
      {L"System", wsudo::exec::privilege_t::system},
      {L"TrustedInstaller", wsudo::exec::privilege_t::trustedinstaller}
      //
  };
  for (const auto &u : userlevels) {
    if (u.level == level) {
      return u.s;
    }
  }
  return L"UNKNOWN Level";
}

const wchar_t *VisibleName(wsudo::exec::visible_t vi) {
  switch (vi) {
  case wsudo::exec::visible_t::none:
    return L"shared console window";
  case wsudo::exec::visible_t::newconsole:
    return L"new console window";
  case wsudo::exec::visible_t::hide:
    return L"no window";
  default:
    break;
  }
  return L"none";
}

// ParseArgv todo
int App::ParseArgv(int argc, wchar_t **argv) {
  simulator.InitializeEnv();
  bela::ParseArgv pa(argc, argv, true);
  pa.Add(L"version", bela::no_argument, L'v')
      .Add(L"help", bela::no_argument, L'h')
      .Add(L"user", bela::required_argument, L'u')
      .Add(L"nui", bela::no_argument, L'n')
      .Add(L"hide", bela::no_argument, L'H')
      .Add(L"wait", bela::no_argument, L'w')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"appx", bela::required_argument, L'x')
      .Add(L"cwd", bela::required_argument, L'c')
      .Add(L"env", bela::required_argument, L'e')
      .Add(L"lpac", bela::no_argument, L'L')
      .Add(L"appcontainer", bela::no_argument, L'a')
      .Add(L"mic", bela::no_argument, L'M')
      .Add(L"standard", bela::no_argument, L'U')
      .Add(L"administrator", bela::no_argument, L'A')
      .Add(L"system", bela::no_argument, L'S')
      .Add(L"ti", bela::no_argument, L'T')
      .Add(L"appid", bela::required_argument, 1001)
      .Add(L"disable-alias", bela::no_argument, 1002)
      .Add(L"appname", bela::required_argument, 1003)
      .Add(L"new-console", bela::no_argument, 1004);
  bela::error_code ec;
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
          visible = wsudo::exec::visible_t::newconsole;
          break;
        case 'H':
          visible = wsudo::exec::visible_t::hide;
          break;
        case 'w':
          wait = true;
          break;
        case 'V':
          wsudo::IsDebugMode = true;
          break;
        case 'x':
          appx = bela::WindowsExpandEnv(va);
          break;
        case 'c':
          cwd = bela::WindowsExpandEnv(va);
          break;
        case 'e':
          SetEnv(va);
          break;
        case 'L':
          lpac = true;
          break;
        case 'a':
          level = wsudo::exec::privilege_t::appcontainer;
          break;
        case 'M':
          level = wsudo::exec::privilege_t::mic;
          break;
        case 'U':
          level = wsudo::exec::privilege_t::standard;
          break;
        case 'A':
          level = wsudo::exec::privilege_t::elevated;
          break;
        case 'S':
          level = wsudo::exec::privilege_t::system;
          break;
        case 'T':
          level = wsudo::exec::privilege_t::trustedinstaller;
          break;
        case 1001:
          appid = va;
          break;
        case 1002:
          disablealias = true;
          break;
        case 1003:
          appid = va;
          break;
        case 1004:
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
  auto &Argv = pa.UnresolvedArgs();
  for (size_t i = 0; i < Argv.size(); i++) {
    const auto arg = Argv[i];
    auto pos = arg.find(L'=');
    if (pos == std::wstring::npos) {
      for (size_t j = i; j < Argv.size(); j++) {
        this->argv.emplace_back(Argv[j]);
      }
      break;
    }
    SetEnv(arg);
  }
  if (!cwd.empty()) {
    DbgPrint(L"App cwd: %s", cwd);
  }
  if (!appx.empty()) {
    DbgPrint(L"App AppContainer Manifest: %s", appx);
  }
  DbgPrint(L"App Launcher level: %s", AppSLevel(level));
  if (disablealias) {
    DbgPrint(L"App Alias is disabled");
  }
  DbgPrint(L"App visible mode: %s", VisibleName(visible));
  return 0;
}

bool SplitArgvInternal(std::wstring &path, std::vector<std::wstring> &argv) {
  std::wstring_view arg0(argv[0]);
  auto a0 = bela::WindowsExpandEnv(arg0);
  argv[0].assign(std::move(a0));
  if (!bela::env::LookPath(argv[0], a0, true)) {
    bela::FPrintF(stderr, L"%s not found in path\n", argv[0]);
    return false;
  }
  path.assign(std::move(a0));
  return true;
}

bool App::SplitArgv() {
  auto splitArgv = [&]() {
    std::wstring_view arg0(argv[0]);
    if (disablealias) {
      DbgPrint(L"disable alias: %s", arg0);
      return SplitArgvInternal(path, argv);
    }
    wsudo::AliasEngine ae;
    if (!ae.Initialize()) {
      DbgPrint(L"unable initialize alias engine");
      return SplitArgvInternal(path, argv);
    }
    auto al = ae.Target(arg0);
    if (!al) {
      return SplitArgvInternal(path, argv);
    }
    DbgPrint(L"App found alias %s", *al);
    std::vector<std::wstring> Argv;
    auto eal = bela::WindowsExpandEnv(*al);
    DbgPrint(L"App expand alias %s", eal);
    bela::error_code ec;
    if (!wsudo::exec::SplitArgv(eal, path, Argv, ec)) {
      bela::FPrintF(stderr, L"SplitArgv: %s\n", ec.message);
      return false;
    }
    for (size_t i = 1; i < argv.size(); i++) {
      Argv.emplace_back(std::move(argv[i]));
    }
    argv = std::move(Argv);
    return true;
  };
  if (!splitArgv()) {
    return false;
  }
  DbgPrint(L"App found arg0 path: %s", path);
  bela::error_code ec;
  auto re = bela::RealPathEx(path, ec);
  if (!re) {
    bela::FPrintF(stderr, L"realpath: %s error: %s\n", *re, ec.message);
    return true;
  }
  DbgPrint(L"App real path '%s'", *re);
  console = bela::pe::IsSubsystemConsole(*re);
  DbgPrint(L"App (script) subsystem is console %b", console);
  return true;
}

int App::AppExecute() {
  wsudo::exec::appcommand cmd;
  cmd.path.assign(std::move(path));
  cmd.argv = std::move(argv);
  cmd.env.assign(simulator.MakeEnv());
  cmd.appid.assign(std::move(appid));
  cmd.appmanifest.assign(std::move(appx));
  cmd.cwd.assign(std::move(cwd));
  cmd.islpac = lpac;
  cmd.visible = visible;
  bela::error_code ec;
  if (!cmd.initialize(ec)) {
    bela::FPrintF(stderr, L"wsudo: initialize AppContainer: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  if (!cmd.execute(ec)) {
    bela::FPrintF(stderr, L"wsudo: \x1b[31m%s\x1b[0m", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"\x1b[32mnew app conatiner process %d running\nSID: %s\nFolder: %s\x1b[0m\n", cmd.pid, cmd.sid,
                cmd.folder);
  if (!Waitable()) {
    DbgPrint(L"App Container process not waitable");
    return 0;
  }
  DbgPrint(L"command %s waitable", cmd.argv[0]);
  return WaitForExit(cmd.pid);
}

int App::Execute() {
  auto elevated = wsudo::exec::IsUserAdministratorsGroup();
  if (!elevated && level >= wsudo::exec::privilege_t::elevated) {
    return DelegateExecute();
  }
  wsudo::exec::command cmd;
  cmd.path.assign(std::move(path));
  cmd.argv = std::move(argv);
  cmd.env.assign(simulator.MakeEnv());
  cmd.cwd.assign(std::move(cwd));
  cmd.visible = visible;
  cmd.priv = level;
  bela::error_code ec;
  if (!cmd.execute(ec)) {
    bela::FPrintF(stderr, L"wsudo: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"\x1b[32mnew process %d running\x1b[0m\n", cmd.pid);
  if (!Waitable()) {
    DbgPrint(L"Process not waitable");
    return 0;
  }
  DbgPrint(L"command %s waitable", cmd.argv[0]);
  return WaitForExit(cmd.pid);
}

} // namespace wsudo

int wmain(int argc, wchar_t **argv) {
  wsudo::App app;
  if (app.ParseArgv(argc, argv) != 0) {
    return 1;
  }
  if (app.argv.empty()) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo: no command input\x1b[0m\n");
    Usage(true);
    return 1;
  }
  if (bela::EqualsIgnoreCase(app.argv[0], L"alias")) {
    return wsudo::AliasSubcmd(app.argv);
  }
  if (!app.SplitArgv()) {
    return 1;
  }
  priv::dotcom_global_initializer di;
  if (app.level == wsudo::exec::privilege_t::appcontainer) {
    return app.AppExecute();
  }
  return app.Execute();
}
