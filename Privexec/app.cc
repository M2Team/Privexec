///
#include <process/process.hpp>
#include "app.hpp" // included windows.h
#include <Windowsx.h>
// C RunTime Header Files
#include <cstdlib>
#include <malloc.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <string>

#include "resource.h"

namespace priv {

// expand env
inline std::wstring ExpandEnv(const std::wstring &s) {
  auto len = ExpandEnvironmentStringsW(s.data(), nullptr, 0);
  if (len <= 0) {
    return s;
  }
  std::wstring s2(len + 1, L'\0');
  auto N = ExpandEnvironmentStringsW(s.data(), &s2[0], len + 1);
  s2.resize(N - 1);
  return s2;
}

int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  return DialogBoxParamW(hInstance, MAKEINTRESOURCE(IDD_APPLICATION_DIALOG),
                         NULL, App::WindowProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR WINAPI App::WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                               LPARAM lParam) {
  App *app{nullptr};
  if (message == WM_INITDIALOG) {
    auto app = reinterpret_cast<App *>(lParam);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    app->Initialize(hWnd);
  } else if ((app = GetThisFromHandle(hWnd)) != nullptr) {
    return app->MessageHandler(message, wParam, lParam);
  }
  return FALSE;
}

bool App::InitializeCapabilities() {
  auto func = [&](int id, wid_t wsid) {
    auto h = GetDlgItem(hWnd, id);
    checks.insert(std::pair<HWND, wid_t>(h, wsid));
  };
  func(IDP_INTERNETCLIENT, WinCapabilityInternetClientSid);
  func(IDP_INTERNETCLIENTSERVER, WinCapabilityInternetClientServerSid);
  func(IDP_PRIVATENETWORKCLIENTSERVER,
       WinCapabilityPrivateNetworkClientServerSid);
  func(IDP_DOCUMENTSLIBRARY, WinCapabilityDocumentsLibrarySid);
  func(IDP_PICTURESLIBRARY, WinCapabilityPicturesLibrarySid);
  func(IDP_VIDEOSLIBRARY, WinCapabilityVideosLibrarySid);
  func(IDP_MUSICLIBRARY, WinCapabilityMusicLibrarySid);
  func(IDP_ENTERPRISEAUTHENTICATION, WinCapabilityEnterpriseAuthenticationSid);
  func(IDP_SHAREDUSERCERTIFICATES, WinCapabilitySharedUserCertificatesSid);
  func(IDP_REMOVABLESTORAGE, WinCapabilityRemovableStorageSid);
  func(IDP_APPOINTMENTS, WinCapabilityAppointmentsSid);
  func(IDP_CONTACTS, WinCapabilityContactsSid);
  return true;
}

bool App::UpdateCapabilities(const std::wstring &file) {
  std::vector<wid_t> cas;
  if (!priv::WellKnownFromAppmanifest(file, cas)) {
    return false;
  }
  for (auto &c : checks) {
    if (std::find(cas.begin(), cas.end(), c.second) != cas.end()) {
      Button_SetCheck(c.first, TRUE);
    } else {
      Button_SetCheck(c.first, FALSE);
    }
  }
  return true;
}

bool App::Initialize(HWND window) {
  hWnd = window;
  HICON icon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPLICATION_ICON));
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
  if (priv::IsUserAdministratorsGroup()) {
    // Update title when app run as admin
    WCHAR title[256];
    auto N = GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    wcscat_s(title, L" [Administrator]");
    SetWindowTextW(hWnd, title);
  }
  combox.hBox = GetDlgItem(hWnd, IDC_USER_COMBOX);
  cmd.hInput = GetDlgItem(hWnd, IDC_COMMAND_COMBOX);
  cmd.hButton = GetDlgItem(hWnd, IDB_COMMAND_TARGET);
  AppAliasInitialize(cmd.hInput, alias); // Initialize app alias
  ///
  cwd.hInput = GetDlgItem(hWnd, IDE_APPSTARTUP);
  cwd.hButton = GetDlgItem(hWnd, IDB_APPSTARTUP);
  appx.hInput = GetDlgItem(hWnd, IDE_APPCONTAINER_APPMANIFEST);
  appx.hButton = GetDlgItem(hWnd, IDB_APPCONTAINER_BUTTON);
  SelChanged(); /// disable appcontainer.

  return true;
}

bool App::SelChanged() {
  if (IsMatchedApplevel(priv::ProcessAppContainer)) {
    appx.Update(L"");
    appx.Visible(TRUE);
  } else {
    appx.Update(L"AppxManifest.xml or Package.appxmanifest");
    appx.Visible(FALSE);
  }
  return true;
}

std::wstring App::ResolveCMD() {
  auto cmd_ = cmd.Content();
  auto it = alias.find(cmd_);
  if (it != alias.end()) {
    return ExpandEnv(it->second);
  }
  return ExpandEnv(cmd_);
}

std::wstring App::ResolveCWD() {
  auto cwd_ = ExpandEnv(cwd.Content());
  if (!cwd_.empty() &&
      (GetFileAttributesW(cwd_.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    /// resolved cwd  valid
    return cwd_;
  }
  auto N = GetCurrentDirectoryW(0, nullptr);
  if (N <= 0) {
    return L"";
  }
  cwd_.resize(N + 1);
  N = GetCurrentDirectoryW(N + 1, &cwd_[0]);
  cwd_.resize(N);
  return cwd_;
}

bool App::AppExecute() {
  auto appindex = AppIndex();
  auto cmd_ = ResolveCMD();
  auto cwd_ = ResolveCWD(); // app startup directory
  if (appindex == priv::ProcessAppContainer) {
    //// TODO app container.
    std::vector<wid_t> cas;
    for (auto &c : checks) {
      if (Button_GetCheck(c.first) == BST_CHECKED) {
        cas.push_back((wid_t)c.second);
      }
    }
    priv::appcontainer p(cmd_);
    p.cwd().assign(cwd_);
    if (!p.initialize(cas.data(), cas.data() + cas.size()) || !p.execute()) {
      auto ec = priv::error_code::lasterror();
      ec.message.append(L" (").append(p.message()).append(L")");
      MessageBoxW(hWnd, ec.message.c_str(),
                  L"Privexec create appconatiner process failed",
                  MB_OK | MB_ICONERROR);
      return false;
    }
    return true;
  }

  priv::process p(cmd_);
  p.cwd().assign(cwd_);
  if (!p.execute(appindex)) {
    auto ec = priv::error_code::lasterror();
    ec.message.append(L" (").append(p.message()).append(L")");
    MessageBoxW(hWnd, ec.message.c_str(), L"Privexec create process failed",
                MB_OK | MB_ICONERROR);
    return false;
  }
  return true;
}

INT_PTR App::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)CreateSolidBrush(RGB(255, 255, 255));
  case WM_SYSCOMMAND:
    break;
  case WM_COMMAND: {
    // WM_COMMAND
    switch (LOWORD(wParam)) {
    case IDC_USER_COMBOX:
      if (HIWORD(wParam) == CBN_SELCHANGE) {
        SelChanged();
      }
      return TRUE;
    case IDB_COMMAND_TARGET: /// lookup command
      return TRUE;
    case IDB_APPSTARTUP: // select startup dir
      return TRUE;
    case IDB_APPCONTAINER_BUTTON: // select appmanifest file
      return TRUE;
    case IDB_EXECUTE_BUTTON: {
      auto hExecute = reinterpret_cast<HWND>(lParam);
      EnableWindow(hExecute, FALSE);
      AppExecute();
      EnableWindow(hExecute, TRUE);
    }
      return TRUE;
    case IDB_EXIT_BUTTON:
      DestroyWindow(hWnd);
      return TRUE;
    default:
      return FALSE;
    }

  } break;
  case WM_CLOSE:
    DestroyWindow(hWnd);
    return TRUE;
  case WM_DESTROY:
    PostQuitMessage(0);
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

} // namespace priv