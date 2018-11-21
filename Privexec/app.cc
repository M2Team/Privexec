///
#include <process/process.hpp>
#include <windows.h>
#include <Windowsx.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <string>
#include "app.hpp"
#include "resource.h"

namespace priv {

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

bool App::AliasInitialize() {
  //
  return true;
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
  cmd.hInput = GetDlgItem(hWnd, IDC_COMMAND_COMBOX);
  cmd.hButton = GetDlgItem(hWnd, IDB_COMMAND_TARGET);
  cwd.hInput = GetDlgItem(hWnd, IDE_APPSTARTUP);
  cwd.hButton = GetDlgItem(hWnd, IDB_APPSTARTUP);
  appx.hInput = GetDlgItem(hWnd, IDE_APPCONTAINER_APPMANIFEST);
  appx.hButton = GetDlgItem(hWnd, IDB_APPCONTAINER_BUTTON);
  
  return true;
}

INT_PTR App::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)CreateSolidBrush(RGB(255, 255, 255));
  case WM_SYSCOMMAND:
    break;
  case WM_COMMAND:
    break;
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