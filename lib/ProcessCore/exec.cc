////
#include <bela/base.hpp> // Windows
#include <bela/finaly.hpp>
#include <shellapi.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <userenv.h>
#include <Sddl.h>
#include "process_fwd.hpp"
#include "elevator.hpp"

namespace priv {
//

bool Process::ExecNone() {
  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  DWORD createflags = CREATE_UNICODE_ENVIRONMENT;
  if (visible == VisibleMode::NewConsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (visible == VisibleMode::Hide) {
    createflags |= CREATE_NO_WINDOW;
  }
  if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, createflags,
                     nullptr, EmptyNull(cwd), &si, &pi) != TRUE) {
    return false;
  }
  pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool Process::ExecLow() {
  HANDLE hToken;
  HANDLE hNewToken;
  LPCWSTR szIntegritySid = L"S-1-16-4096";
  PSID pIntegritySid = NULL;
  TOKEN_MANDATORY_LABEL TIL = {0};
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
    kmessage = L"lowlevelexec<OpenProcessToken>";
    return false;
  }
  if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
                        TokenPrimary, &hNewToken)) {
    kmessage = L"lowlevelexec<DuplicateTokenEx>";
    CloseHandle(hToken);
    return false;
  }
  if (!ConvertStringSidToSidW(szIntegritySid, &pIntegritySid)) {
    CloseHandle(hToken);
    CloseHandle(hNewToken);
    kmessage = L"lowlevelexec<ConvertStringSidToSidW>";
    return false;
  }

  TIL.Label.Attributes = SE_GROUP_INTEGRITY;
  TIL.Label.Sid = pIntegritySid;
  auto deleter = bela::finally([&] {
    CloseHandle(hToken);
    CloseHandle(hNewToken);
    LocalFree(pIntegritySid);
  });
  // Set process integrity levels
  if (!SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
                           sizeof(TOKEN_MANDATORY_LABEL) +
                               GetLengthSid(pIntegritySid))) {
    kmessage = L"lowlevelexec<SetTokenInformation>";
    return false;
  }
  return ExecWithToken(hNewToken,false);
}

bool Process::ExecElevated() {
  if (IsUserAdministratorsGroup()) {
    return ExecNone();
  }
  auto pszcmd = cmd.data();
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
  if (visible == VisibleMode::Hide) {
    info.nShow = SW_HIDE;
  } else {
    info.nShow = SW_SHOWNORMAL;
  }
  info.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS;
  info.lpDirectory = EmptyNull(cwd);
  if (ShellExecuteExW(&info) != TRUE) {
    return false;
  }
  pid = GetProcessId(info.hProcess);
  CloseHandle(info.hProcess);
}

bool Process::ExecWithToken(HANDLE hToken, bool desktop) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  wchar_t lpDesktop[] = L"WinSta0\\Default";
  if (desktop) {
    si.lpDesktop = lpDesktop;
  }
  LPVOID lpEnvironment = nullptr;
  HANDLE hProcessToken = nullptr;
  if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken)) {
    ::CreateEnvironmentBlock(&lpEnvironment, hProcessToken, TRUE);
    CloseHandle(hProcessToken);
  }
  auto deleter = bela::finally([&] {
    if (lpEnvironment != nullptr) {
      ::DestroyEnvironmentBlock(lpEnvironment);
    }
  });
  DWORD createflags = 0;
  if (lpEnvironment = nullptr) {
    createflags |= CREATE_UNICODE_ENVIRONMENT;
  }
  if (visible == VisibleMode::NewConsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (visible == VisibleMode::Hide) {
    createflags |= CREATE_NO_WINDOW;
  }
  if (CreateProcessAsUserW(hToken, nullptr, cmd.data(), nullptr, nullptr, FALSE,
                           createflags, lpEnvironment, EmptyNull(cwd), &si,
                           &pi) != TRUE) {
    return false;
  }
  pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool Process::Exec(EexcLevel el) {
  if (el == EexcLevel::AppContainer) {
    return true;
  }
  switch (el) {
  case EexcLevel::MIC:
    return ExecLow();
  case EexcLevel::NoElevated:
    return ExecNoElevated();
  case EexcLevel::Elevated:
    return ExecElevated();
  case EexcLevel::System:
    return ExecSystem();
  case EexcLevel::TrustedInstaller:
    return ExecTI();
  default:
    break;
  }
  return ExecNone();
}

} // namespace priv