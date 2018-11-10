#include "stdafx.h"
#include "Privexec.Core.hpp"
#include <shellapi.h>
#include <Shlwapi.h>
#include <string>

bool CreateAdministratorsProcess(LPWSTR pszCmdline, DWORD &dwProcessId) {
  if (IsUserAdministratorsGroup()) {
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    auto bResult = CreateProcessW(nullptr, pszCmdline, nullptr, nullptr, FALSE,
                                  0, nullptr, nullptr, &si, &pi);
    if (bResult) {
      dwProcessId = pi.dwProcessId;
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      return true;
    }
    return false;
  }
  LPWSTR pszArgs = PathGetArgsW(pszCmdline);
  std::wstring file;
  if (pszArgs != nullptr) {
    file.assign(pszCmdline, pszArgs);
    auto iter = file.rbegin();
    for (; iter != file.rend(); iter++) {
      if (*iter != L' ' && *iter != L'\t')
        break;
    }
    file.resize(file.size() - (iter - file.rbegin()));
  } else {
    file.assign(pszCmdline);
  }
  SHELLEXECUTEINFOW info;
  ZeroMemory(&info, sizeof(info));
  info.lpFile = file.data();
  info.lpParameters = pszArgs;
  info.lpVerb = L"runas";
  info.cbSize = sizeof(info);
  info.hwnd = NULL;
  info.nShow = SW_SHOWNORMAL;
  info.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS;
  info.lpDirectory = nullptr;
  auto bResult = ShellExecuteExW(&info);
  if (bResult) {
    dwProcessId = GetProcessId(info.hProcess);
    CloseHandle(info.hProcess);
    return true;
  }
  return false;
}

bool PrivCreateProcess(int level, LPWSTR pszCmdline, DWORD &dwProcessId,const std::wstring &sid) {
  switch (level) {
  case kAppContainer:
    return CreateAppContainerProcess(pszCmdline, dwProcessId,sid);
  case kUACElevated:
    return CreateNoElevatedProcess(pszCmdline, dwProcessId);
  case kMandatoryIntegrityControl:
    return CreateLowlevelProcess(pszCmdline, dwProcessId);
  case kAdministrator:
    return CreateAdministratorsProcess(pszCmdline, dwProcessId);
  case kSystem:
    return CreateSystemProcess(pszCmdline, dwProcessId);
  case kTrustedInstaller:
    return CreateTiProcess(pszCmdline, dwProcessId);
  default:
    break;
  }
  SetLastError(ERROR_NOT_SUPPORTED);
  return false;
}