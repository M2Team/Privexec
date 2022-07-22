///
#include "execinternal.hpp"
#include <bela/escapeargv.hpp>
#include <shellapi.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <userenv.h>
#include <Sddl.h>
#include <comdef.h>
#include <wtsapi32.h>

namespace wsudo::exec {
bool make_standard_token(PHANDLE hNewToken, bela::error_code &ec) {
  if (IsUserAdministratorsGroup()) {
    privilege_view pv = {{SE_TCB_NAME, SE_ASSIGNPRIMARYTOKEN_NAME, SE_INCREASE_QUOTA_NAME}};
    PermissionAdjuster eo;
    if (!eo.Elevate(&pv, ec)) {
      return false;
    }
    auto hToken = INVALID_HANDLE_VALUE;
    auto deleter = bela::finally([&] { FreeToken(hToken); });
    // get user login token
    if (WTSQueryUserToken(eo.SID(), &hToken) != TRUE) {
      ec = bela::make_system_error_code(L"WTSQueryUserToken: ");
      return false;
    }
    if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, hNewToken) != TRUE) {
      ec = bela::make_system_error_code(L"DuplicateTokenEx: ");
      return false;
    }
    return true;
  }
  // when not administrator
  HANDLE hCurrentToken = INVALID_HANDLE_VALUE;
  if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hCurrentToken)) {
    return false;
  }
  if (!DuplicateTokenEx(hCurrentToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, hNewToken)) {
    CloseHandle(hCurrentToken);
    return false;
  }
  CloseHandle(hCurrentToken);
  return true;
}

// execute start a normal command
bool execute_basic(command &cmd, bela::error_code &ec) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  bela::EscapeArgv ea;
  for (const auto &a : cmd.argv) {
    ea.Append(a);
  }
  DWORD createflags = CREATE_UNICODE_ENVIRONMENT;
  if (cmd.visible == visible_t::newconsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (cmd.visible == visible_t::hide) {
    createflags |= CREATE_NO_WINDOW;
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
  }
  if (CreateProcessW(string_nullable(cmd.path), ea.data(), nullptr, nullptr, FALSE, createflags,
                     string_nullable(cmd.env), string_nullable(cmd.cwd), &si, &pi) != TRUE) {
    ec = bela::make_system_error_code(L"CreateProcessW");
    return false;
  }
  cmd.pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool execute_with_token(HANDLE hToken, bool desktop, command &cmd, bela::error_code &ec) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  wchar_t lpDesktop[] = L"WinSta0\\Default";
  if (desktop) {
    si.lpDesktop = lpDesktop;
  }
  bela::EscapeArgv ea;
  for (const auto &a : cmd.argv) {
    ea.Append(a);
  }
  DWORD createflags = CREATE_UNICODE_ENVIRONMENT;
  if (cmd.visible == visible_t::newconsole) {
    createflags |= CREATE_NEW_CONSOLE;
  } else if (cmd.visible == visible_t::hide) {
    createflags |= CREATE_NO_WINDOW;
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
  }
  if (CreateProcessAsUserW(hToken, string_nullable(cmd.path), ea.data(), nullptr, nullptr, FALSE, createflags,
                           string_nullable(cmd.env), string_nullable(cmd.cwd), &si, &pi) != TRUE) {
    ec = bela::make_system_error_code(L"CreateProcessAsUserW");
    return false;
  }
  cmd.pid = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

bool execute_with_low(command &cmd, bela::error_code &ec) {
  HANDLE hNewToken;
  LPCWSTR szIntegritySid = L"S-1-16-4096";
  PSID pIntegritySid = nullptr;
  TOKEN_MANDATORY_LABEL TIL = {0};

  auto deleter = bela::finally([&] {
    FreeToken(hNewToken);
    if (pIntegritySid != nullptr) {
      LocalFree(pIntegritySid);
    }
  });
  if (!ConvertStringSidToSidW(szIntegritySid, &pIntegritySid)) {
    ec = bela::make_system_error_code(L"ConvertStringSidToSidW: ");
    return false;
  }
  TIL.Label.Attributes = SE_GROUP_INTEGRITY;
  TIL.Label.Sid = pIntegritySid;
  if (!make_standard_token(&hNewToken, ec)) {
    return false;
  }
  // Set process integrity levels
  if (!SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
                           sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(pIntegritySid))) {
    ec = bela::make_system_error_code(L"SetTokenInformation: ");
    return false;
  }
  return execute_with_token(hNewToken, false, cmd, ec);
}

bool execute_with_elevated(command &cmd, bela::error_code &ec) {
  if (IsUserAdministratorsGroup()) {
    return execute_basic(cmd, ec);
  }
  SHELLEXECUTEINFOW info;
  ZeroMemory(&info, sizeof(info));
  bela::EscapeArgv ea;
  for (size_t i = 1; i < cmd.argv.size(); i++) {
    ea.Append(cmd.argv[i]);
  }
  info.lpFile = cmd.path.empty() ? cmd.argv[0].data() : cmd.path.data();
  info.lpParameters = ea.size() == 0 ? nullptr : ea.data();
  info.lpVerb = L"runas";
  info.cbSize = sizeof(info);
  info.hwnd = NULL;
  if (cmd.visible == visible_t::hide) {
    info.nShow = SW_HIDE;
  } else {
    info.nShow = SW_SHOWNORMAL;
  }
  info.fMask = SEE_MASK_DEFAULT | SEE_MASK_NOCLOSEPROCESS;
  info.lpDirectory = string_nullable(cmd.cwd);
  if (ShellExecuteExW(&info) != TRUE) {
    ec = bela::make_system_error_code(L"ShellExecuteExW: ");
    return false;
  }
  cmd.pid = GetProcessId(info.hProcess);
  CloseHandle(info.hProcess);
  return true;
}

bool execute_standard(command &cmd, bela::error_code &ec) {
  HANDLE hNewToken{nullptr};
  auto deleter = bela::finally([&] { FreeToken(hNewToken); });
  if (!make_standard_token(&hNewToken, ec)) {
    return false;
  }
  return execute_with_token(hNewToken, false, cmd, ec);
}

bool command::execute(bela::error_code &ec) {
  switch (priv) {
  case privilege_t::appcontainer:
    ec = bela::make_error_code(1, L"BUG: please use appcommand run appcontainer");
    return false;
  case privilege_t::mic:
    return wsudo::exec::execute_with_low(*this, ec);
  case privilege_t::basic:
    return wsudo::exec::execute_basic(*this, ec);
  case privilege_t::standard:
    return wsudo::exec::execute_standard(*this, ec);
  case privilege_t::elevated:
    return wsudo::exec::execute_with_elevated(*this, ec);
  case privilege_t::system:
    return wsudo::exec::execute_with_system(*this, ec);
  case privilege_t::trustedinstaller:
    return wsudo::exec::execute_with_ti(*this, ec);
  default:
    break;
  }
  return wsudo::exec::execute_basic(*this, ec);
}

} // namespace wsudo::exec