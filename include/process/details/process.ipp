/// process base
#ifndef PRIVEXEC_PRCOESS_IPP
#define PRIVEXEC_PRCOESS_IPP
#include "processfwd.hpp"
#include <shellapi.h>
#include <Shlwapi.h>
#include <string>

namespace priv {
// execute only
bool process::execute() {
  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  if (CreateProcessW(nullptr, &cmd_[0], nullptr, nullptr, FALSE, 0, nullptr,
                     Castwstr(cwd_), &si, &pi) != TRUE) {
    return false;
  }
  pid_ = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool process::elevatedexec() {
  if (IsUserAdministratorsGroup()) {
    return execute();
  }
  auto pszcmd = cmd_.data();
  LPWSTR pszargs = PathGetArgsW(pszcmd);
  std::wstring file;
  if (pszargs != nullptr) {
    file.assign(pszcmd, pszargs);
    auto iter = file.rbegin();
    for (; iter != file.rend(); iter++) {
      if (*iter != L' ' && *iter != L'\t')
        break;
    }
    file.resize(file.size() - (iter - file.rbegin()));
  } else {
    file.assign(pszcmd);
  }
  SHELLEXECUTEINFOW info;
  ZeroMemory(&info, sizeof(info));
  info.lpFile = file.data();
  info.lpParameters = pszargs;
  info.lpVerb = L"runas";
  info.cbSize = sizeof(info);
  info.hwnd = NULL;
  info.nShow = SW_SHOWNORMAL;
  info.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS;
  info.lpDirectory = nullptr;
  if (ShellExecuteExW(&info) != TRUE) {
    return false;
  }
  pid_ = GetProcessId(info.hProcess);
  CloseHandle(info.hProcess);
  return true;
}

// Exe with token
bool process::execwithtoken(HANDLE hToken, bool desktop) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  if (desktop) {
    si.lpDesktop = L"WinSta0\\Default";
  }
  LPVOID lpEnvironment = nullptr;
  HANDLE hProcessToken = nullptr;
  if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken)) {
    CreateEnvironmentBlock(&lpEnvironment, hProcessToken, TRUE);
    CloseHandle(hProcessToken);
  }
  auto deleter = finally([&] {
    if (lpEnvironment != nullptr) {
      DestroyEnvironmentBlock(lpEnvironment);
    }
  });
  if (CreateProcessAsUserW(
          hToken, nullptr, &cmd_[0], nullptr, nullptr, FALSE,
          (lpEnvironment == nullptr) ? 0 : CREATE_UNICODE_ENVIRONMENT,
          lpEnvironment, Castwstr(cwd_), &si, &pi) != TRUE) {
    return false;
  }
  pid_ = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool process::execute(int level) {
  switch (level) {
  case ProcessAppContainer: {
    appcontainer ac(cmd_);
    if (!ac.initialize()) {
      return false;
    }
    ac.cwd() = cwd_;
    if (!ac.execute()) {
      return false;
    }
    pid_ = ac.pid();
    return true;
  }
    return true;
  case ProcessMandatoryIntegrityControl:
    return lowlevelexec();
  case ProcessNoElevated:
    return noelevatedexec();
  case ProcessElevated:
    return elevatedexec();
  case ProcessSystem:
    return systemexec();
  case ProcessTrustedInstaller:
    return tiexec();
  default:
    break;
  }
  return false;
}

} // namespace priv

#endif