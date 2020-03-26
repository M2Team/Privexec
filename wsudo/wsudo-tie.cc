// wsudotie.cc
/*
wsudo-tie wsudo administrator level permission middleware.
usage: wsudo-tie command args...
 -v|--version        print version and exit
 -h|--help           print help information and exit
 -p|--parent         wsudo prcocess pid.
 -V|--verbose        Make the operation more talkative
 -d|--pwd           wsudo-tie switch to workdir
 -c|--cwd            Use a working directory to launch the process.
 -e|--env            Set Environment Variable.

*/
// other's command line
#include <bela/stdwriter.hpp>
#include <bela/escapeargv.hpp>
#include <bela/picker.hpp>
#include <bela/str_split.hpp>
#include <bela/path.hpp>
#include <bela/parseargv.hpp>
#include <version.h>
#include "env.hpp"

namespace wsudo::tie {
struct AppMode {
  Derivator envctx;
  std::vector<std::wstring_view> args;
  std::wstring_view cwd; // --cwd -c
  std::wstring_view pwd; // --pwd self cwd
  DWORD parentid{0};     // parent process pid
  bool verbose{false};   // --verbose -V
  void Verbose(const wchar_t *fmt) const {
    if (verbose) {
      bela::FileWrite(stderr, fmt);
    }
  }
  template <typename... Args>
  void Verbose(const wchar_t *fmt, Args... args) const {
    if (verbose) {
      const bela::format_internal::FormatArg arg_array[] = {args...};
      auto str = bela::format_internal::StrFormatInternal(fmt, arg_array,
                                                          sizeof...(args));
      bela::FileWrite(stderr, str);
    }
  }

  int ParseArgv(int argc, wchar_t **argv);
  bool InitializeApp();
  void Verbose();
};

void Version() {
  bela::FPrintF(stderr, L"\x1b[36mwsudo %s\x1b[0m\n", PRIVEXEC_BUILD_VERSION);
}

void Usage(bool err = false) {
  constexpr const wchar_t *kUsage =
      LR"(wsudo-tie wsudo administrator level permission middleware.
usage: wsudo-tie command args...
   -v|--version        print version and exit
   -h|--help           print help information and exit
   -p|--parent         wsudo prcocess pid.
   -V|--verbose        Make the operation more talkative
   -d|--pwd            wsudo-tie switch to workdir
   -c|--cwd            Use a working directory to launch the process.
   -e|--env            Set Environment Variable.

)";
  char32_t sh = 0x1F496; //  💖
  bela::FPrintF(stderr, L"\x1b[%dmwsudo-tie %c %s %s\x1b[0m\n", err ? 31 : 36,
                sh, PRIV_VERSION_MAIN, kUsage);
}

int AppMode::ParseArgv(int argc, wchar_t **argv) {
  bela::ParseArgv pa(argc, argv, true);
  pa.Add(L"version", bela::no_argument, L'v')
      .Add(L"help", bela::no_argument, L'h')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"pwd", bela::required_argument, L'd')
      .Add(L"parent", bela::required_argument, L'P')
      .Add(L"cwd", bela::required_argument, L'c')
      .Add(L"env", bela::required_argument, L'e');
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
        case 'P': // parent
          if (!bela::SimpleAtoi(va, &parentid)) {
            bela::BelaMessageBox(nullptr,
                                 L"Parent process PID format is incorrect", va,
                                 nullptr, bela::mbs_t::FATAL);
          }
          break;
        case 'V':
          verbose = true;
          break;
        case 'd':
          pwd = va;
          break;
        case 'c':
          cwd = va;
          break;
        case 'e':
          envctx.Append(va);
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!result) {
    bela::BelaMessageBox(nullptr, L"wsudo-tie ParseArgv:", ec.message.data(),
                         nullptr, bela::mbs_t::FATAL);
    return 1;
  }
  /// Copy TO
  args = pa.UnresolvedArgs();
  return 0;
}

bool AppMode::InitializeApp() {
  if (parentid == 0) {
    bela::BelaMessageBox(nullptr, L"wsudo-tie InitializeApp error:",
                         L"missing process pid", nullptr, bela::mbs_t::FATAL);
    return false;
  }
  FreeConsole();
  if (AttachConsole(parentid) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::StrAppend(&ec.message, L"parent process id: ", parentid);
    bela::BelaMessageBox(nullptr, L"wsudo-tie AttachConsole error:",
                         ec.message.data(), nullptr, bela::mbs_t::FATAL);
    return false;
  }
  if (!pwd.empty() && SetCurrentDirectoryW(pwd.data()) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::BelaMessageBox(nullptr, L"wsudo-tie SetCurrentDirectoryW error:",
                         ec.message.data(), nullptr, bela::mbs_t::FATAL);
    return false;
  }
  return true;
}

void AppMode::Verbose() {
  if (!verbose) {
    return;
  }
  if (!cwd.empty()) {
    bela::FPrintF(stderr, L"\x1b[01;33m* App cwd: %s\x1b[0m\n", cwd);
  }
}

} // namespace wsudo::tie

void dumpPathEnv() {
  auto path = bela::GetEnv(L"PATH");
  std::vector<std::wstring_view> pv =
      bela::StrSplit(path, bela::ByChar(L';'), bela::SkipEmpty());
  for (auto p : pv) {
    bela::FPrintF(stderr, L"\x1b[01;33m%s\x1b[0m\n", p);
  }
}

// FreeConsole
// AttachConsole to parent process
// CreateProcessW cmdline cwd env...

int wmain(int argc, wchar_t **argv) {
  wsudo::tie::AppMode am;
  if (am.ParseArgv(argc, argv) != 0) {
    return -1;
  }
  if (!am.InitializeApp()) {
    return -1;
  }
  am.Verbose();
  if (am.args.empty()) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo-tie unable start new process. "
                          L"cmdline missing\x1b[0m\n");
    return false;
  }
  bela::EscapeArgv ea;
  for (auto s : am.args) {
    ea.Append(s);
  }
  am.envctx.Apply([&](std::wstring_view k, std::wstring_view v) {
    am.Verbose(L"\x1b[01;33m* App apply env '%s' = '%s'\x1b[0m\n", k, v);
  });
  std::wstring exe;
  // CreateProcessW search path maybe failed.
  if (bela::ExecutableExistsInPath(am.args[0], exe)) {
    am.Verbose(L"\x1b[01;33m* App execute path '%s'\x1b[0m\n", exe);
  }
  am.Verbose(L"\x1b[01;33m* App real argv0 '%s'\x1b[0m\n", am.args[0]);
  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  DWORD createflags = CREATE_UNICODE_ENVIRONMENT;
  if (CreateProcessW(exe.empty() ? nullptr : exe.data(), ea.data(), nullptr,
                     nullptr, FALSE, createflags, nullptr,
                     am.cwd.empty() ? nullptr : am.cwd.data(), &si,
                     &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr,
                  L"\x1b[31mwsudo-tie unable CreateProcessW "
                  L"%s\x1b[0m\n\x1b[31mcommand: [%s]\x1b[0m\n",
                  ec.message, ea.sv());
    dumpPathEnv();
    return 1;
  }
  bela::FPrintF(stderr,
                L"\x1b[01;32mnew administrator process is running: %d\x1b[0m\n",
                pi.dwProcessId);
  SetConsoleCtrlHandler(nullptr, TRUE);
  if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable wait process '%s'\x1b[0m\n",
                  ec.message);
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return -1;
  }
  SetConsoleCtrlHandler(nullptr, FALSE);
  DWORD exitcode = 0;
  if (GetExitCodeProcess(pi.hProcess, &exitcode) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"\x1b[31munable get process exit code '%s'\x1b[0m\n",
                  ec.message);
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return static_cast<int>(exitcode);
}
