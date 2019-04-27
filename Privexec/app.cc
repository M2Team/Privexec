///
#include <process/process.hpp>
#include "app.hpp" // included windows.h
#include <Windowsx.h>
// C RunTime Header Files
#include <cstdlib>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
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
                              MAKEINTRESOURCEW(IDD_APPLICATION_DIALOG), NULL,
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

bool App::Initialize(HWND window) {
  hWnd = window;
  HICON icon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPLICATION_ICON));
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
  // Drag support
  ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
  ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
  ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
  ::DragAcceptFiles(hWnd, TRUE);

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
    box.Append(priv::ProcessNoElevated, L"Not Elevated (UAC)", true);
    box.Append(priv::ProcessElevated, L"Administrator");
    box.Append(priv::ProcessSystem, L"System");
    box.Append(priv::ProcessTrustedInstaller, L"TrustedInstaller");
  } else {
    box.Append(priv::ProcessNoElevated, L"Not Elevated (UAC)");
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
  if (cmd_.empty()) {
    utils::PrivMessageBox(hWnd, L"Please input command line", L"command empty",
                          PRIVEXEC_APPLINKE, utils::kFatalWindow);
    return false;
  }
  auto cwd_ = ResolveCWD(); // app startup directory

  if (appindex == priv::ProcessAppContainer) {
    //// TODO app container.
    auto cas = appcas.Capabilities();
    auto xml = ExpandEnv(appx.Content());
    if (!xml.empty() && !priv::MergeFromAppmanifest(xml, cas)) {
      auto ec = base::make_system_error_code();
      utils::PrivMessageBox(hWnd, L"Privexec appmanifest error",
                            ec.message.c_str(), PRIVEXEC_APPLINKE,
                            utils::kFatalWindow);
      return false;
    }
    priv::appcontainer p(cmd_);
    p.enablelpac(appcas.IsLowPrivilegeAppContainer());
    p.cwd().assign(cwd_);
    if (!p.initialize(cas) || !p.execute()) {
      auto ec = base::make_system_error_code();
      if (!p.message().empty()) {
        ec.message.append(L" (").append(p.message()).append(L")");
      }
      utils::PrivMessageBox(
          hWnd, L"Privexec create appconatiner process failed",
          ec.message.c_str(), PRIVEXEC_APPLINKE, utils::kFatalWindow);
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
    auto ec = base::make_system_error_code();
    if (!p.message().empty()) {
      ec.message.append(L" (").append(p.message()).append(L")");
    }
    utils::PrivMessageBox(hWnd, L"Privexec create process failed",
                          ec.message.c_str(), PRIVEXEC_APPLINKE,
                          utils::kFatalWindow);
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

bool App::DropFiles(WPARAM wParam, LPARAM lParam) {
  const LPCWSTR appmanifest[] = {L".xml", L".appxmanifest"};
  HDROP hDrop = (HDROP)wParam;
  UINT nfilecounts = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
  WCHAR dfile[4096] = {0};
  for (UINT i = 0; i < nfilecounts; i++) {
    DragQueryFileW(hDrop, i, dfile, 4096);
    if (PathFindSuffixArrayW(dfile, appmanifest, ARRAYSIZE(appmanifest))) {
      if (appx.IsVisible()) {
        appx.Update(dfile);
      }
      continue;
    }
    cmd.Update(dfile);
  }
  DragFinish(hDrop);
  return true;
}

INT_PTR App::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC: {
    if (hbrBkgnd == nullptr) {
      hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
    }
    return (INT_PTR)hbrBkgnd;
  }

  case WM_DROPFILES:
    DropFiles(wParam, lParam);
    return TRUE;
  case WM_SYSCOMMAND:
    switch (LOWORD(wParam)) {
    case IDM_PRIVEXEC_ABOUT:
      utils::PrivMessageBox(hWnd, L"About Privexec", PRIVEXEC_APPVERSION,
                            PRIVEXEC_APPLINK, utils::kAboutWindow);
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
