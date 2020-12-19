// wsudo-bridge.cc
#include <bela/terminal.hpp>
#include <bela/escapeargv.hpp>
#include <bela/picker.hpp>
#include <bela/str_split.hpp>
#include <bela/path.hpp>
#include <bela/parseargv.hpp>
#include <bela/simulator.hpp>
#include <baseversion.h>
#include <exec.hpp>

namespace wsudo::bridge {
bool IsDebugMode = false;
int WriteTrace(std::wstring_view msg);

template <typename... Args> bela::ssize_t DbgPrintP(const wchar_t *fmt, const Args&... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  return WriteTrace(str);
}
inline bela::ssize_t DbgPrintP(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return WriteTrace(msg);
}

int WriteError(std::wstring_view msg);
template <typename... Args> bela::ssize_t PrintError(const wchar_t *fmt, const Args&... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  return WriteError(str);
}
inline bela::ssize_t PrintError(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return WriteError(msg);
}

// DbgPrint added newline
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
}

struct App {
  wsudo::exec::command cmd;
  bela::env::Simulator simulator;
  std::wstring pwd;
  DWORD parentid{0}; // parent process pid
  bool attachConsole{false};
  bool waitForExit{false};
  bool ParseArgv(int argc, wchar_t **argv);
  bool InitializeApp();
  int Execute();
  void SetEnv(std::wstring_view es) {
    auto envstr = bela::WindowsExpandEnv(es);
    std::wstring_view nv(envstr);
    if (auto pos = nv.find(L'='); pos != std::wstring_view::npos) {
      auto name = nv.substr(0, pos);
      if (bela::EqualsIgnoreCase(name, L"Path")) {
        std::vector<std::wstring_view> paths =
            bela::StrSplit(nv.substr(pos + 1), bela::ByChar(L';'), bela::SkipEmpty());
        for (const auto p : paths) {
          simulator.PathPushFront(p);
        }
      } else {
        simulator.SetEnv(name, nv.substr(pos + 1), true);
      }
    }
  }
};

void Version() {
  //
  bela::FPrintF(stderr, L"\x1b[36mwsudo %s\x1b[0m\n", PRIVEXEC_VERSION);
}

void Usage(bool err = false) {
  constexpr const wchar_t *kUsage = LR"(wsudo-bridge wsudo administrator level permission middleware.
usage: wsudo-bridge command args...
   -v|--version        print version and exit
   -h|--help           print help information and exit
   -p|--parent         wsudo prcocess pid.
   -V|--verbose        Make the operation more talkative
   -d|--pwd            wsudo-bridge switch to workdir
   -c|--cwd            Use a working directory to launch the process.
   -e|--env            Use specific environment variables to start child processes
   -w|--wait           Start application and wait for it to terminate.
   -A|--attach         Attached to the console of the parent process
   -u|--user           run as user (optional), support '-uX', '-u X', '--user=X', '--user X'
                       Supported user categories (Ignore case):
                       Administrator   System    TrustedInstaller
   --visible           Visibility level passed by wsudo

)";
  char32_t sh = 0x1F496; //  💖
  bela::FPrintF(stderr, L"\x1b[%dmwsudo-bridge %c %d.%d %s\x1b[0m\n", err ? 31 : 36, sh, PRIVEXEC_VERSION_MAJOR,
                PRIVEXEC_VERSION_MINOR, kUsage);
}

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

bool App::ParseArgv(int argc, wchar_t **argv) {
  simulator.InitializeEnv();
  bela::ParseArgv pa(argc, argv, true);
  pa.Add(L"version", bela::no_argument, L'v')
      .Add(L"help", bela::no_argument, L'h')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"pwd", bela::required_argument, L'd')
      .Add(L"parent", bela::required_argument, L'P')
      .Add(L"cwd", bela::required_argument, L'c')
      .Add(L"env", bela::required_argument, L'e')
      .Add(L"user", bela::required_argument, L'u')
      .Add(L"wait", bela::no_argument, L'w')
      .Add(L"attach", bela::no_argument, L'A')
      .Add(L"visible", bela::required_argument, 1001);
  bela::error_code ec;
  cmd.priv = wsudo::exec::privilege_t::elevated;
  auto result = pa.Execute(
      [&](int val, const wchar_t *va, const wchar_t *) {
        switch (val) {
        case 'v':
          Version();
          exit(0);
        case 'h':
          Usage();
          exit(0);
        case 'P': // parent
          if (!bela::SimpleAtoi(va, &parentid)) {
            PrintError(L"Incorrect parent process id %s", va);
          }
          break;
        case 'V':
          IsDebugMode = true;
          break;
        case 'u':
          DbgPrintP(L"Privilege %s", va);
          IsAppLevel(va, cmd.priv);
          break;
        case 'd':
          pwd = va;
          break;
        case 'c':
          cmd.cwd = va;
          break;
        case 'e':
          SetEnv(va);
          break;
        case 'A':
          attachConsole = true;
          break;
        case 'w':
          waitForExit = true;
          break;
        case 1001:
          int v;
          if (bela::SimpleAtoi(va, &v) && v >= 0 && v <= static_cast<int>(wsudo::exec::visible_t::hide)) {
            cmd.visible = static_cast<wsudo::exec::visible_t>(v);
          }
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!result) {
    PrintError(L"ParseArgv %s", ec.message);
    return false;
  }
  if (cmd.cwd.empty() && !pwd.empty()) {
    cmd.cwd = pwd;
  }

  auto &Argv = pa.UnresolvedArgs();
  for (size_t i = 0; i < Argv.size(); i++) {
    const auto arg = Argv[i];
    if (auto pos = arg.find(L'='); pos == std::wstring::npos) {
      for (size_t j = i; j < Argv.size(); j++) {
        cmd.argv.emplace_back(Argv[j]);
      }
      break;
    }
    SetEnv(arg);
  }
  if (cmd.argv.empty()) {
    PrintError(L"wsudo-bridge: no command input");
    return false;
  }
  cmd.path = cmd.argv[0];
  cmd.env.assign(simulator.MakeEnv());
  DbgPrintP(L"resolve path: %s", cmd.path);
  return true;
}

bool App::InitializeApp() {
  if (!attachConsole) {
    // no need call gui
    return true;
  }
  if (parentid == 0) {
    PrintError(L"wsudo-bridge: parent process id=0");
    return false;
  }
  DbgPrintP(L"wsudo-bridge FreeConsole. parentid: %d", parentid);
  FreeConsole();
  if (AttachConsole(parentid) != TRUE) {
    auto ec = bela::make_system_error_code();
    PrintError(L"wsudo-bridge: AttachConsole parent %d error: %s", parentid, ec.message);
    return false;
  }
  if (!pwd.empty() && SetCurrentDirectoryW(pwd.data()) != TRUE) {
    auto ec = bela::make_system_error_code();
    PrintError(L"wsudo-bridge: SetCurrentDirectoryW parent %d error: %s", parentid, ec.message);
    return false;
  }
  DbgPrintP(L"wsudo-bridge AttachConsole done. parentid: %d", parentid);
  return true;
}

inline void dumpPathEnv() {
  auto path = bela::GetEnv(L"PATH");
  std::vector<std::wstring_view> pv = bela::StrSplit(path, bela::ByChar(L';'), bela::SkipEmpty());
  for (auto p : pv) {
    bela::FPrintF(stderr, L"\x1b[01;33m%s\x1b[0m\n", p);
  }
}

int WaitForExit(DWORD pid) {
  if (pid == 0) {
    return -1;
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

const std::wstring_view AppSLevel(const wsudo::exec::privilege_t level) {
  constexpr static struct {
    const wchar_t *s;
    wsudo::exec::privilege_t level;
  } userlevels[] = {
      //
      {L"appcontainer", wsudo::exec::privilege_t::appcontainer},
      {L"mandatory integrity control", wsudo::exec::privilege_t::mic},
      {L"standard", wsudo::exec::privilege_t::standard},
      {L"administrator", wsudo::exec::privilege_t::elevated},
      {L"system", wsudo::exec::privilege_t::system},
      {L"trustedinstaller", wsudo::exec::privilege_t::trustedinstaller}
      //
  };
  for (const auto &u : userlevels) {
    if (u.level == level) {
      return u.s;
    }
  }
  return L"UNKNOWN Level";
}

int App::Execute() {
  if (IsDebugMode) {
    dumpPathEnv();
  }
  DbgPrint(L"App[bridge]: %s", cmd.path);
  bela::error_code ec;
  if (!cmd.execute(ec)) {
    bela::FPrintF(stderr, L"start command %s error: %s\n", cmd.argv[0], ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"\x1b[32mnew \x1b[36m%s\x1b[32m process is running: %d\x1b[0m\n", AppSLevel(cmd.priv),
                cmd.pid);
  if (!waitForExit) {
    return 0;
  }
  return WaitForExit(cmd.pid);
}

} // namespace wsudo::bridge

// FreeConsole
// AttachConsole to parent process
// CreateProcessW cmdline cwd env...

int wmain(int argc, wchar_t **argv) {
  wsudo::bridge::App app;
  if (!app.ParseArgv(argc, argv)) {
    return 1;
  }
  if (!app.InitializeApp()) {
    return 1;
  }
  return app.Execute();
}
