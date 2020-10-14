//
#include <bela/path.hpp>
#include <Shellapi.h>
#include <Shlobj.h>
#include <bela/escapeargv.hpp>
#include "wsudo.hpp"

namespace wsudo {

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

inline std::wstring AppCwd() {
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
  auto file = bela::StringCat(*finalexeparent, L"\\wsudo-bridge.exe");
  if (bela::PathExists(file)) {
    return std::make_optional(std::move(file));
  }
  return std::nullopt;
}

const wchar_t *AppStringLevel(wsudo::exec::privilege_t level) {
  switch (level) {
  case wsudo::exec::privilege_t::system:
    return L"system";
  case wsudo::exec::privilege_t::trustedinstaller:
    return L"trustedinstaller";
  default:
    break;
  }
  return nullptr;
}

int App::DelegateExecute() {
  auto tie = AppTieExecuteExists();
  if (!tie) {
    return 1;
  }
  DbgPrint(L"App use wsudo-bridge: %s", *tie);
  bela::EscapeArgv ea;
  auto self = GetCurrentProcessId();
  // parent pid
  ea.Append(L"--parent").Append(bela::AlphaNum(self).Piece());
  auto pwd = AppCwd();
  if (!pwd.empty()) {
    ea.Append(L"--pwd").Append(pwd);
  }
  ea.Append(bela::StringCat(L"--visible=", static_cast<int>(visible)));
  // is parent is terminal attach console
  if (bela::terminal::IsTerminal(stderr)) {
    ea.Append(L"--attach");
  }
  if (Waitable()) {
    ea.Append(L"--wait");
  }
  // parent work dir (wsudo-bridge will chdir to)
  // env values...
  if (wsudo::IsDebugMode) {
    ea.Append(L"--verbose");
  }
  if (auto sl = AppStringLevel(level); sl != nullptr) {
    ea.Append(L"-u").Append(sl);
  }
  // spec cwd
  if (!cwd.empty()) {
    ea.Append(L"--cwd").Append(cwd);
  }
  for (const auto e : envlist) {
    ea.Append(L"-e").Append(e);
  }
  ea.Append(path);
  for (size_t i = 1; i < argv.size(); i++) {
    ea.Append(argv[i]);
  }

  SHELLEXECUTEINFOW info;
  ZeroMemory(&info, sizeof(info));
  info.lpFile = tie->data();
  info.lpParameters = ea.data();
  info.lpVerb = L"runas";
  info.cbSize = sizeof(info);
  info.hwnd = NULL;
  info.nShow = SW_HIDE;
  info.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS;
  if (ShellExecuteExW(&info) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"unable create process %s", ec.message);
    return 1;
  }
  auto pid = GetProcessId(info.hProcess);
  CloseHandle(info.hProcess);
  return WaitForExit(pid);
}
} // namespace wsudo