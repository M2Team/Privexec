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
  bela::FPrintF(stderr, L"\x1b[%dmwsudo %c %d.%d %s\x1b[0m\n", err ? 31 : 36, sh,
                PRIVEXEC_VERSION_MAJOR, PRIVEXEC_VERSION_MINOR, kUsage);
}

namespace wsudo {
bool IsDebugMode = false;
//    -a          AppContainer
//    -M          Mandatory Integrity Control
//    -U          No Elevated(UAC)
//    -A          Administrator
//    -S          System
//    -T          TrustedInstaller
bool IsAppLevel(std::wstring_view k, priv::ExecLevel &level) {
  static struct {
    const std::wstring_view s;
    priv::ExecLevel level;
  } userlevels[] = {
      //
      {L"appcontainer", priv::ExecLevel::AppContainer},
      {L"mic", priv::ExecLevel::MIC},
      {L"noelevated", priv::ExecLevel::NoElevated},
      {L"administrator", priv::ExecLevel::Elevated},
      {L"system", priv::ExecLevel::System},
      {L"trustedinstaller", priv::ExecLevel::TrustedInstaller}
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

// ParseArgv todo
int AppMode::ParseArgv(int argc, wchar_t **argv) {
  bela::ParseArgv pa(argc, argv, true);
  pa.Add(L"version", bela::no_argument, L'v')
      .Add(L"help", bela::no_argument, L'h')
      .Add(L"user", bela::required_argument, L'u')
      .Add(L"new-console", bela::no_argument, L'n')
      .Add(L"hide", bela::no_argument, L'H')
      .Add(L"wait", bela::no_argument, L'w')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"appx", bela::required_argument, L'x')
      .Add(L"cwd", bela::required_argument, L'c')
      .Add(L"env", bela::required_argument, L'e')
      .Add(L"lpac", bela::no_argument, L'L')
      .Add(L"", bela::no_argument, L'a')
      .Add(L"", bela::no_argument, L'M')
      .Add(L"", bela::no_argument, L'U')
      .Add(L"", bela::no_argument, L'A')
      .Add(L"", bela::no_argument, L'S')
      .Add(L"", bela::no_argument, L'T')
      .Add(L"appname", bela::required_argument, 1001)
      .Add(L"disable-alias", bela::no_argument, 1002);
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
          visible = priv::VisibleMode::NewConsole;
          break;
        case 'H':
          visible = priv::VisibleMode::Hide;
          break;
        case 'w':
          wait = true;
          break;
        case 'V':
          wsudo::IsDebugMode = true;
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
          level = priv::ExecLevel::AppContainer;
          break;
        case 'M':
          level = priv::ExecLevel::MIC;
          break;
        case 'U':
          level = priv::ExecLevel::NoElevated;
          break;
        case 'A':
          level = priv::ExecLevel::Elevated;
          break;
        case 'S':
          level = priv::ExecLevel::System;
          break;
        case 'T':
          level = priv::ExecLevel::TrustedInstaller;
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

const std::wstring_view AppSLevel(priv::ExecLevel l) {
  static struct {
    const wchar_t *s;
    priv::ExecLevel level;
  } userlevels[] = {
      //
      {L"AppContainer", priv::ExecLevel::AppContainer},
      {L"Mandatory Integrity Control", priv::ExecLevel::MIC},
      {L"NoElevated", priv::ExecLevel::NoElevated},
      {L"Administrator", priv::ExecLevel::Elevated},
      {L"System", priv::ExecLevel::System},
      {L"TrustedInstaller", priv::ExecLevel::TrustedInstaller}
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
  if (!IsDebugMode) {
    return;
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
}

bool IsConsoleSuffix(std::wstring_view path) {
  constexpr std::wstring_view suffixs[] = {L".bat", L".cmd", L".com"};
  for (auto s : suffixs) {
    if (bela::EndsWithIgnoreCase(path, s)) {
      return true;
    }
  }
  return false;
}

bool AppSubsystemIsConsole(std::wstring &cmd, bool aliasexpand, const AppMode &am) {
  if (!aliasexpand) {
    std::wstring exe;
    if (!bela::env::ExecutableExistsInPath(cmd, exe)) {
      // NOT FOUND
      return false;
    }
    DbgPrint(L"App full path '%s'", exe);
    bela::error_code ec;
    auto realexe = bela::RealPathEx(exe, ec);
    if (!realexe) {
      bela::FPrintF(stderr, L"realpath: %s error: %s\n", exe, ec.message);
      return false;
    }
    DbgPrint(L"App real path '%s'", *realexe);
    auto pe = bela::pe::Expose(*realexe, ec);
    if (!pe) {
      DbgPrint(L"Not PE File '%s'", *realexe);
      if (IsConsoleSuffix(exe)) {
        cmd.assign(exe);
        return true;
      }
      return false;
    }
    DbgPrint(L"App real argv0 '%s'", exe);
    return pe->subsystem == bela::pe::Subsystem::CUI;
  }
  int Argc;
  auto Argv = CommandLineToArgvW(cmd.data(), &Argc);
  if (Argc == 0) {
    return false;
  }
  DbgPrint(L"App real argv0 '%s'", Argv[0]);
  std::wstring exe;
  if (!bela::env::ExecutableExistsInPath(Argv[0], exe)) {
    LocalFree(Argv);
    return false;
  }
  LocalFree(Argv);
  DbgPrint(L"App full path '%s'", exe);
  bela::error_code ec;
  auto realexe = bela::RealPathEx(exe, ec);
  if (!realexe) {
    bela::FPrintF(stderr, L"realpath: %s error: %s\n", exe, ec.message);
    return false;
  }
  DbgPrint(L"App real path '%s'", *realexe);
  auto pe = bela::pe::Expose(*realexe, ec);
  if (!pe) {
    DbgPrint(L"Not PE File '%s'", *realexe);
    return IsConsoleSuffix(*realexe);
  }
  DbgPrint(L"App real argv0 '%s'", *realexe);
  return pe->subsystem == bela::pe::Subsystem::CUI;
}

int AppWait(DWORD pid) {
  if (pid == 0) {
    return 0;
  }
  auto hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
  if (hProcess == INVALID_HANDLE_VALUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable open process '%s'\x1b[0m\n", ec.message);
    return -1;
  }
  SetConsoleCtrlHandler(nullptr, TRUE);
  if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable wait process '%s'\x1b[0m\n", ec.message);
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(hProcess);
    return -1;
  }
  SetConsoleCtrlHandler(nullptr, FALSE);
  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable get process exit code '%s'\x1b[0m\n", ec.message);
  }
  CloseHandle(hProcess);
  return static_cast<int>(exitcode);
}

std::wstring ExpandArgv0(std::wstring_view argv0, bool disablealias, const wsudo::AppMode &am,
                         bool &aliasexpand) {
  aliasexpand = false;
  if (disablealias) {
    return bela::WindowsExpandEnv(argv0);
  }
  wsudo::AliasEngine ae;
  if (!ae.Initialize()) {
    return bela::WindowsExpandEnv(argv0);
  }
  auto cmd = std::wstring(argv0);
  auto al = ae.Target(cmd);
  if (!al) {
    return bela::WindowsExpandEnv(argv0);
  }
  aliasexpand = true;
  DbgPrint(L"App alias '%s' expand to '%s'", cmd, *al);
  return bela::WindowsExpandEnv(*al);
}

int AppExecuteAppContainer(wsudo::AppMode &am) {
  bela::EscapeArgv ea;
  bool aliasexpand = false;
  auto argv0 = ExpandArgv0(am.args[0], am.disablealias, am, aliasexpand);
  auto isconsole = wsudo::AppSubsystemIsConsole(argv0, aliasexpand, am);
  bool waitable = false;
  if (aliasexpand) {
    ea.AssignNoEscape(argv0);
  } else {
    ea.Assign(argv0);
  }
  DbgPrint(L"App real arg0 '%s'", argv0);
  if (am.visible == priv::VisibleMode::None && isconsole || am.wait) {
    waitable = true;
  }

  if (isconsole) {
    DbgPrint(L"App subsystem is console, %s", am.Visible());
  }
  for (size_t i = 1; i < am.args.size(); i++) {
    ea.Append(am.args[i]);
  }
  DbgPrint(L"App real command '%s'", argv0);
  am.envctx.Apply([&](std::wstring_view k, std::wstring_view v) {
    DbgPrint(L"App apply env '%s' = '%s'", k, v);
  });
  priv::AppContainer p(ea.sv());
  if (!am.cwd.empty()) {
    p.Chdir(bela::WindowsExpandEnv(am.cwd));
  }
  if (!am.appname.empty()) {
    p.Name(am.appname);
  }
  auto appx = bela::WindowsExpandEnv(am.appx);
  p.ChangeVisibleMode(am.visible);
  p.EnableLPAC(am.lpac);
  if (am.lpac) {
    DbgPrint(L"AppContainer: Less Privileged AppContainer Is "
             L"Enabled.");
  }
  if (!am.appname.empty()) {
    p.Name(am.appname);
  }
  bela::FPrintF(stderr, L"\x1b[01;32mCommand: %s", ea.sv());
  bool ok = false;
  if (am.appx.empty()) {
    if (am.lpac) {
      bela::FPrintF(stderr, L"\x1b[31mAppContainer: LPAC mode is set but will "
                            L"not take effect. Appmanifest not set.\x1b[0m\n");
    }
    ok = p.InitializeNone();
  } else {
    ok = p.InitializeFile(appx);
  }
  if (!ok) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr,
                  L"\x1b[31minitialize appconatiner process  last error %d  "
                  L"(%s): %s\x1b[0m\n",
                  ec.code, p.Message(), ec.message);
    return 1;
  }
  if (!p.Exec()) {
    auto ec = bela::make_system_error_code();
    if (p.Message().empty()) {
      bela::FPrintF(stderr, L"\x1b[31mcreate appconatiner process  last error %d : %s\x1b[0m\n",
                    ec.code, ec.message);
    } else {
      bela::FPrintF(stderr,
                    L"\x1b[31mcreate appconatiner process  last error %d  "
                    L"(%s): %s\x1b[0m\n",
                    ec.code, p.Message(), ec.message);
    }
    return 1;
  }
  bela::FPrintF(stderr, L"\x1b[01;32mnew appcontainer process is running: %d\nsid: %s\x1b[0m\n",
                p.PID(), p.SSID());
  if (waitable) {
    return AppWait(p.PID());
  }
  return 0;
}

inline std::wstring AppGetcwd() {
  std::wstring stack;
  stack.resize(256);
  auto len = GetCurrentDirectory((DWORD)stack.size(), stack.data());
  if (len == 0) {
    return L"";
  }
  stack.resize(len);
  if (len < 256) {
    return stack;
  }
  auto newlen = GetCurrentDirectory((DWORD)stack.size(), stack.data());
  if (newlen == 0 || newlen >= len) {
    return L"";
  }
  stack.resize(newlen);
  return stack;
}

std::optional<std::wstring> AppTieExecuteExists() {
  bela::error_code ec;
  auto finalexeparent = bela::ExecutableFinalPathParent(ec);
  if (!finalexeparent) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo unable resolve executable %s \x1b[0m\n", ec.message);
    return std::nullopt;
  }
  auto file = bela::StringCat(*finalexeparent, L"\\wsudo-tie.exe");
  if (bela::PathExists(file)) {
    return std::make_optional(std::move(file));
  }
  return std::nullopt;
}

int AppExecuteTie(std::wstring_view tie, std::wstring_view arg0, wsudo::AppMode &am) {
  bela::EscapeArgv ea;

  auto self = GetCurrentProcessId();
  // parent pid
  ea.Append(L"--parent").Append(bela::AlphaNum(self).Piece());
  auto pwd = AppGetcwd();
  if (!pwd.empty()) {
    ea.Append(L"--pwd").Append(pwd);
  }
  // parent work dir (wsudo-tie will chdir to)
  // env values...
  if (wsudo::IsDebugMode) {
    ea.Append(L"--verbose");
  }
  // spec cwd
  if (!am.cwd.empty()) {
    ea.Append(L"--cwd").Append(bela::WindowsExpandEnv(am.cwd));
  }
  for (const auto &kv : am.envctx.Items()) {
    ea.Append(L"--env").Append(bela::StringCat(kv.first, L"=", kv.second));
  }
  ea.AppendNoEscape(arg0);
  for (size_t i = 1; i < am.args.size(); i++) {
    ea.Append(am.args[i]);
  }

  SHELLEXECUTEINFOW info;
  ZeroMemory(&info, sizeof(info));
  info.lpFile = tie.data();
  info.lpParameters = ea.data();
  info.lpVerb = L"runas";
  info.cbSize = sizeof(info);
  info.hwnd = NULL;
  info.nShow = SW_HIDE;
  info.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS;
  if (ShellExecuteExW(&info) != TRUE) {
    return 1;
  }
  auto pid = GetProcessId(info.hProcess);
  CloseHandle(info.hProcess);
  return AppWait(pid);
}

bool IsConhosted(wsudo::AppMode &am) {
  auto h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h == nullptr || h == INVALID_HANDLE_VALUE) {
    DbgPrint(L"unable get std handle");
    return false;
  }
  auto t = GetFileType(h);
  if (t == FILE_TYPE_CHAR) {
    return true;
  }
  if (t == FILE_TYPE_PIPE) {
    constexpr unsigned int pipemaxlen = 512;
    WCHAR buffer[pipemaxlen] = {0};
    if (GetFileInformationByHandleEx(h, FileNameInfo, buffer, pipemaxlen * 2) == TRUE) {
      auto pb = reinterpret_cast<FILE_NAME_INFO *>(buffer);
      std::wstring_view pipename{pb->FileName, pb->FileNameLength / 2};
      auto pn = bela::StringCat(L"\\\\.\\pipe", pipename);
      DbgPrint(L"App stdout: '%s'", pn);
    }
    return false;
  }

  return false;
}

int AppExecute(wsudo::AppMode &am) {
  bela::EscapeArgv ea;
  bool aliasexpand = false;
  auto argv0 = ExpandArgv0(am.args[0], am.disablealias, am, aliasexpand);
  auto isconsole = AppSubsystemIsConsole(argv0, aliasexpand, am);
  if (aliasexpand) {
    ea.AssignNoEscape(argv0);
  } else {
    ea.Assign(argv0);
  }
  DbgPrint(L"App real arg0 '%s'", argv0);
  auto elevated = priv::IsUserAdministratorsGroup();
  // If wsudo-tie exists. we will use wsudo-tie as administrator proxy
  if (!elevated && am.level == priv::ExecLevel::Elevated && isconsole &&
      am.visible == priv::VisibleMode::None && IsConhosted(am)) {
    auto tie = AppTieExecuteExists();
    if (tie) {
      DbgPrint(L"App subsystem is console, use %s as middleware.", *tie);
      return AppExecuteTie(*tie, ea.sv(), am);
    }
  }
  bool waitable = false;
  if (!elevated && am.level == priv::ExecLevel::Elevated && am.visible == priv::VisibleMode::None) {
    am.visible = priv::VisibleMode::NewConsole; /// change visible
  }
  if (am.visible == priv::VisibleMode::None && isconsole || am.wait) {
    waitable = true;
  }

  if (isconsole) {
    DbgPrint(L"App subsystem is console, %s", am.Visible());
  }

  for (size_t i = 1; i < am.args.size(); i++) {
    ea.Append(am.args[i]);
  }
  DbgPrint(L"App real command '%s'", argv0);
  am.envctx.Apply([&](std::wstring_view k, std::wstring_view v) {
    DbgPrint(L"App apply env '%s' = '%s'", k, v);
  });
  priv::Process p(ea.sv());
  p.ChangeVisibleMode(am.visible);
  if (!am.cwd.empty()) {
    p.Chdir(bela::WindowsExpandEnv(am.cwd));
  }
  bela::FPrintF(stderr, L"\x1b[01;32mCommand: %s\x1b[0m\n", ea.sv());
  if (p.Exec(am.level)) {
    bela::FPrintF(stderr, L"\x1b[01;32mnew process is running: %d\x1b[0m\n", p.PID());
    if (waitable) {
      return AppWait(p.PID());
    }
    return 0;
  }
  auto ec = bela::make_system_error_code();
  if (p.Message().empty()) {
    bela::FPrintF(stderr, L"\x1b[31mcreate process  last error %d : %s\x1b[0m\n", ec.code,
                  ec.message);
  } else {
    bela::FPrintF(stderr, L"\x1b[31mcreate process  last error %d  (%s): %s\x1b[0m\n", ec.code,
                  p.Message(), ec.message);
  }
  return 1;
}
} // namespace wsudo

int wmain(int argc, wchar_t **argv) {
  wsudo::AppMode am;
  if (am.ParseArgv(argc, argv) != 0) {
    return 1;
  }
  if (am.args.empty()) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo missing command %s see usage:\x1b[0m\n", am.message);
    Usage(true);
    return 1;
  }
  if (am.args[0] == L"alias") {
    return wsudo::AliasSubcmd(am.args);
  }
  am.Verbose();
  priv::dotcom_global_initializer di;
  if (am.level == priv::ExecLevel::AppContainer) {
    return wsudo::AppExecuteAppContainer(am);
  }
  return wsudo::AppExecute(am);
}
