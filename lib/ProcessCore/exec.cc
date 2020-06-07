////
#include <bela/base.hpp> // Windows
#include <shellapi.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <userenv.h>
#include <Sddl.h>
#include <comdef.h>
#include <wtsapi32.h>
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
  if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, createflags, nullptr,
                     EmptyNull(cwd), &si, &pi) != TRUE) {
    return false;
  }
  pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool GetNoElevatedToken(PHANDLE hNewToken) {
  if (IsUserAdministratorsGroup()) {
    PrivilegeView pv = {{SE_TCB_NAME, SE_ASSIGNPRIMARYTOKEN_NAME, SE_INCREASE_QUOTA_NAME}};
    Elevator eo;
    std::wstring msg;
    if (!eo.Elevate(&pv, msg)) {
      return false;
    }
    auto hToken = INVALID_HANDLE_VALUE;
    auto deleter = bela::finally([&] { CloseHandleEx(hToken); });
    // get user login token
    if (WTSQueryUserToken(eo.SID(), &hToken) != TRUE) {
      return false;
    }
    if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary,
                         hNewToken) != TRUE) {
      return false;
    }
    return true;
  }
  // when not administrator
  HANDLE hCurrentToken = INVALID_HANDLE_VALUE;
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hCurrentToken)) {
    return false;
  }
  if (!DuplicateTokenEx(hCurrentToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary,
                        hNewToken)) {
    CloseHandle(hCurrentToken);
    return false;
  }
  CloseHandle(hCurrentToken);
  return true;
}

bool Process::ExecLow() {
  HANDLE hNewToken;
  LPCWSTR szIntegritySid = L"S-1-16-4096";
  PSID pIntegritySid = nullptr;
  TOKEN_MANDATORY_LABEL TIL = {0};

  auto deleter = bela::finally([&] {
    CloseHandleEx(hNewToken);
    if (pIntegritySid != nullptr) {
      LocalFree(pIntegritySid);
    }
  });
  if (!ConvertStringSidToSidW(szIntegritySid, &pIntegritySid)) {
    kmessage = L"lowlevelexec<ConvertStringSidToSidW>";
    return false;
  }
  TIL.Label.Attributes = SE_GROUP_INTEGRITY;
  TIL.Label.Sid = pIntegritySid;
  if (!GetNoElevatedToken(&hNewToken)) {
    kmessage = L"lowlevelexec<GetNoElevatedToken>";
    return false;
  }
  // Set process integrity levels
  if (!SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
                           sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(pIntegritySid))) {
    kmessage = L"lowlevelexec<SetTokenInformation>";
    return false;
  }
  return ExecWithToken(hNewToken, false);
}

bool Process::ExecNoElevated() {
  HANDLE hNewToken;
  LPCWSTR szIntegritySid = L"S-1-16-8192";
  PSID pIntegritySid = nullptr;
  TOKEN_MANDATORY_LABEL TIL = {0};

  auto deleter = bela::finally([&] {
    CloseHandleEx(hNewToken);
    if (pIntegritySid != nullptr) {
      LocalFree(pIntegritySid);
    }
  });
  if (!ConvertStringSidToSidW(szIntegritySid, &pIntegritySid)) {
    kmessage = L"NoElevated <ConvertStringSidToSidW>";
    return false;
  }
  TIL.Label.Attributes = SE_GROUP_INTEGRITY;
  TIL.Label.Sid = pIntegritySid;
  if (!GetNoElevatedToken(&hNewToken)) {
    kmessage = L"NoElevated <GetNoElevatedToken>";
    return false;
  }
  // Set process integrity levels
  if (!SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
                           sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(pIntegritySid))) {
    kmessage = L"NoElevated <SetTokenInformation>";
    return false;
  }
  return ExecWithToken(hNewToken, false);
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
  return true;
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
  if (CreateProcessAsUserW(hToken, nullptr, cmd.data(), nullptr, nullptr, FALSE, createflags,
                           lpEnvironment, EmptyNull(cwd), &si, &pi) != TRUE) {
    return false;
  }
  pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool Process::Exec(ExecLevel el) {
  if (el == ExecLevel::AppContainer) {
    return true;
  }
  switch (el) {
  case ExecLevel::MIC:
    return ExecLow();
  case ExecLevel::NoElevated:
    return ExecNoElevated();
  case ExecLevel::Elevated:
    return ExecElevated();
  case ExecLevel::System:
    return ExecSystem();
  case ExecLevel::TrustedInstaller:
    return ExecTI();
  default:
    break;
  }
  return ExecNone();
}

} // namespace priv
