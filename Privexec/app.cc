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
#include "apputils.hpp"
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
  return (int)DialogBoxParamW(hInstance,
                              MAKEINTRESOURCE(IDD_APPLICATION_DIALOG), NULL,
                              App::WindowProc, reinterpret_cast<LPARAM>(this));
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
  appcas.Add(hWnd, IDP_INTERNETCLIENT, WinCapabilityInternetClientSid);
  appcas.Add(hWnd, IDP_INTERNETCLIENTSERVER,
             WinCapabilityInternetClientServerSid);
  appcas.Add(hWnd, IDP_PRIVATENETWORKCLIENTSERVER,
             WinCapabilityPrivateNetworkClientServerSid);
  appcas.Add(hWnd, IDP_DOCUMENTSLIBRARY, WinCapabilityDocumentsLibrarySid);
  appcas.Add(hWnd, IDP_PICTURESLIBRARY, WinCapabilityPicturesLibrarySid);
  appcas.Add(hWnd, IDP_VIDEOSLIBRARY, WinCapabilityVideosLibrarySid);
  appcas.Add(hWnd, IDP_MUSICLIBRARY, WinCapabilityMusicLibrarySid);
  appcas.Add(hWnd, IDP_ENTERPRISEAUTHENTICATION,
             WinCapabilityEnterpriseAuthenticationSid);
  appcas.Add(hWnd, IDP_SHAREDUSERCERTIFICATES,
             WinCapabilitySharedUserCertificatesSid);
  appcas.Add(hWnd, IDP_REMOVABLESTORAGE, WinCapabilityRemovableStorageSid);
  appcas.Add(hWnd, IDP_APPOINTMENTS, WinCapabilityAppointmentsSid);
  appcas.Add(hWnd, IDP_CONTACTS, WinCapabilityContactsSid);
  return true;
}

bool App::UpdateCapabilities(const std::wstring &file) {
  std::vector<wid_t> cas;
  if (!priv::WellKnownFromAppmanifest(file, cas)) {
    return false;
  }
  appcas.Update(cas);
  return true;
}

bool App::Initialize(HWND window) {
  hWnd = window;
  HICON icon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPLICATION_ICON));
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
  auto elevated = priv::IsUserAdministratorsGroup();
  if (elevated) {
    // Update title when app run as admin
    WCHAR title[256];
    auto N = GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    wcscat_s(title, L" [Administrator]");
    SetWindowTextW(hWnd, title);
  }
  box = GetDlgItem(hWnd, IDC_USER_COMBOX);

  box.Append(priv::ProcessAppContainer, L"AppContainer");
  box.Append(priv::ProcessMandatoryIntegrityControl,
             L"Mandatory Integrity Control");
  if (elevated) {
    box.Append(priv::ProcessNoElevated, L"No Elevated (UAC)", true);
    box.Append(priv::ProcessElevated, L"Administrator");
    box.Append(priv::ProcessSystem, L"System");
    box.Append(priv::ProcessTrustedInstaller, L"TrustedInstaller");
  } else {
    box.Append(priv::ProcessNoElevated, L"No Elevated (UAC)");
    box.Append(priv::ProcessElevated, L"Administrator", true);
  }
  HMENU hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_PRIVEXEC_ABOUT,
              L"About Privexec\tAlt+F1");
  cmd.hInput = GetDlgItem(hWnd, IDC_COMMAND_COMBOX);
  cmd.hButton = GetDlgItem(hWnd, IDB_COMMAND_TARGET);
  AppAliasInitialize(cmd.hInput, alias); // Initialize app alias
  ///
  cwd.hInput = GetDlgItem(hWnd, IDE_APPSTARTUP);
  cwd.hButton = GetDlgItem(hWnd, IDB_APPSTARTUP);
  appx.hInput = GetDlgItem(hWnd, IDE_APPCONTAINER_APPMANIFEST);
  appx.hButton = GetDlgItem(hWnd, IDB_APPCONTAINER_BUTTON);
  InitializeCapabilities();
  SelChanged(); /// disable appcontainer.

  return true;
}

bool App::SelChanged() {
  if (box.IsMatch(priv::ProcessAppContainer)) {
    appx.Update(L"");
    appx.Visible(TRUE);
    appcas.Visible(TRUE);
  } else {
    appx.Update(L"AppxManifest.xml or Package.appxmanifest");
    appx.Visible(FALSE);
    appcas.Visible(FALSE);
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
  auto appindex = box.AppIndex();
  auto cmd_ = ResolveCMD();
  auto cwd_ = ResolveCWD(); // app startup directory
  constexpr const wchar_t *ei =
      L"Ask for help with this issue. \nVisit: <a "
      L"href=\"https://github.com/M2Team/Privexec/issues\">Privexec Issues</a>";
  if (appindex == priv::ProcessAppContainer) {
    //// TODO app container.
    auto cas = appcas.Capabilities();
    priv::appcontainer p(cmd_);
    p.cwd().assign(cwd_);
    if (!p.initialize(cas.data(), cas.data() + cas.size()) || !p.execute()) {
      auto ec = priv::error_code::lasterror();
      if (!p.message().empty()) {
        ec.message.append(L" (").append(p.message()).append(L")");
      }
      utils::PrivMessageBox(hWnd,
                            L"Privexec create appconatiner process failed",
                            ec.message.c_str(), ei, utils::kFatalWindow);
      return false;
    }
    return true;
  }

  priv::process p(cmd_);
  p.cwd().assign(cwd_);
  if (!p.execute(appindex)) {
    if (appindex == priv::ProcessElevated &&
        !priv::IsUserAdministratorsGroup()) {
      return false;
    }
    auto ec = priv::error_code::lasterror();
    if (!p.message().empty()) {
      ec.message.append(L" (").append(p.message()).append(L")");
    }
    utils::PrivMessageBox(hWnd, L"Privexec create process failed",
                          ec.message.c_str(), ei, utils::kFatalWindow);
    return false;
  }
  return true;
}

bool App::AppLookupExecute() {
  const utils::filter_t filters[] = {
      {L"Windows Execute(*.exe;*.com;*.bat)", L"*.exe;*.com;*.bat"},
      {L"All Files (*.*)", L"*.*"}};
  auto exe =
      utils::PrivFilePicker(hWnd, L"Privexec: Select Execute", filters, 2);
  if (exe) {
    cmd.Update(*exe);
    return true;
  }
  return false;
}

bool App::AppLookupManifest() {
  const utils::filter_t filters[] = {
      {L"Windows Appxmanifest (*.appxmanifest;*.xml)", L"*.appxmanifest;*.xml"},
      {L"All Files (*.*)", L"*.*"}};
  auto xml =
      utils::PrivFilePicker(hWnd, L"Privexec: Select AppManifest", filters, 2);
  if (xml) {
    appx.Update(*xml);
    UpdateCapabilities(*xml);
    return true;
  }
  return false;
}
bool App::AppLookupCWD() {
  auto folder =
      utils::PrivFolderPicker(hWnd, L"Privexec: Select App Launcher Folder");
  if (folder) {
    cwd.Update(*folder);
    return true;
  }
  return false;
}

INT_PTR App::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) {
  constexpr const wchar_t *appurl =
      L"For more information about this tool. \nVisit: <a "
      L"href=\"https://github.com/M2Team/Privexec\">Privexec</a>\nVisit: <a "
      L"href=\"https://forcemz.net/\">forcemz.net</a>";
  switch (message) {
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)CreateSolidBrush(RGB(255, 255, 255));
  case WM_SYSCOMMAND:
    switch (LOWORD(wParam)) {
    case IDM_PRIVEXEC_ABOUT:
      utils::PrivMessageBox(
          hWnd, L"About Privexec",
          L"Prerelease:"
          L" " PRIVEXEC_BUILD_VERSION L"\nCopyright \xA9 2018, Force "
          L"Charlie. All Rights Reserved.",
          appurl, utils::kAboutWindow);
      break;
    default:
      break;
    }
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
      AppLookupExecute();
      return TRUE;
    case IDB_APPSTARTUP: // select startup dir
      AppLookupCWD();
      return TRUE;
    case IDB_APPCONTAINER_BUTTON: // select appmanifest file
      AppLookupManifest();
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