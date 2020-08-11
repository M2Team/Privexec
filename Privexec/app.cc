///
#include <process.hpp>
#include "app.hpp" // included windows.h
#include <Windowsx.h>
// C RunTime Header Files
#include <cstdlib>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <string>
#include <bela/env.hpp>
#include <bela/picker.hpp>
#include "resource.h"

namespace priv {

int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  return (int)DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_APPLICATION_DIALOG), NULL,
                              App::WindowProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR WINAPI App::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
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

  box.Append((int)priv::ExecLevel::AppContainer, L"AppContainer");
  box.Append((int)priv::ExecLevel::MIC, L"Mandatory Integrity Control");
  if (elevated) {
    box.Append((int)priv::ExecLevel::NoElevated, L"Not Elevated (UAC)", true);
    box.Append((int)priv::ExecLevel::Elevated, L"Administrator");
    box.Append((int)priv::ExecLevel::System, L"System");
    box.Append((int)priv::ExecLevel::TrustedInstaller, L"TrustedInstaller");
  } else {
    box.Append((int)priv::ExecLevel::NoElevated, L"Not Elevated (UAC)");
    box.Append((int)priv::ExecLevel::Elevated, L"Administrator", true);
  }
  HMENU hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_PRIVEXEC_ABOUT, L"About Privexec\tAlt+F1");
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
  if (box.IsMatch((int)priv::ExecLevel::AppContainer)) {
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
    return bela::WindowsExpandEnv(it->second);
  }
  return bela::WindowsExpandEnv(cmd_);
}

std::wstring App::ResolveCWD(bool allowempty) {
  auto cwd_ = bela::WindowsExpandEnv(cwd.Content());
  if (!cwd_.empty() && (GetFileAttributesW(cwd_.data()) & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    /// resolved cwd  valid
    return cwd_;
  }
  if (allowempty) {
    return L"";
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
    bela::BelaMessageBox(hWnd, L"Please input command line", L"command empty", PRIVEXEC_APPLINKE,
                         bela::mbs_t::FATAL);
    return false;
  }

  if (appindex == (int)priv::ExecLevel::AppContainer) {
    //// TODO app container.
    auto cas = appcas.Capabilities();
    auto xml = bela::WindowsExpandEnv(appx.Content());
    if (!xml.empty() && !priv::MergeFromAppManifest(xml, cas)) {
      auto ec = bela::make_system_error_code();
      bela::BelaMessageBox(hWnd, L"Privexec appmanifest error", ec.message.c_str(),
                           PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
      return false;
    }
    priv::AppContainer p(cmd_);
    p.EnableLPAC(appcas.IsLowPrivilegeAppContainer());
    auto cwd_ = ResolveCWD(true); // app startup directory
    p.Chdir(cwd_);
    if (!p.Initialize({cas.data(), cas.size()}) || !p.Exec()) {
      auto ec = bela::make_system_error_code();
      if (!p.Message().empty()) {
        bela::StrAppend(&ec.message, L"(", p.Message(), L")");
      }
      bela::BelaMessageBox(hWnd, L"Privexec create appconatiner process failed", ec.message.c_str(),
                           PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
      return false;
    }
    return true;
  }

  priv::Process p(cmd_);
  auto cwd_ = ResolveCWD(); // app startup directory
  p.Chdir(cwd_);
  if (!p.Exec((priv::ExecLevel)appindex)) {
    if (appindex == (int)priv::ExecLevel::Elevated && !priv::IsUserAdministratorsGroup()) {
      return false;
    }
    auto ec = bela::make_system_error_code();
    if (!p.Message().empty()) {
      bela::StrAppend(&ec.message, L"(", p.Message(), L")");
    }
    bela::BelaMessageBox(hWnd, L"Privexec create process failed", ec.message.data(),
                         PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  return true;
}

bool App::AppLookupExecute() {
  const bela::filter_t filters[] = {{L"Windows Execute(*.exe;*.com;*.bat)", L"*.exe;*.com;*.bat"},
                                    {L"All Files (*.*)", L"*.*"}};
  auto exe = bela::FilePicker(hWnd, L"Privexec: Select Execute", filters);
  if (exe) {
    cmd.Update(*exe);
    return true;
  }
  return false;
}

bool App::AppLookupManifest() {
  const bela::filter_t filters[] = {
      {L"Windows Appxmanifest (*.appxmanifest;*.xml)", L"*.appxmanifest;*.xml"},
      {L"All Files (*.*)", L"*.*"}};
  auto xml = bela::FilePicker(hWnd, L"Privexec: Select AppManifest", filters);
  if (xml) {
    appx.Update(*xml);
    return true;
  }
  return false;
}
bool App::AppLookupCWD() {
  auto folder = bela::FolderPicker(hWnd, L"Privexec: Select App Launcher Folder");
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
      bela::BelaMessageBox(hWnd, L"About Privexec", PRIVEXEC_APPVERSION, PRIVEXEC_APPLINK,
                           bela::mbs_t::ABOUT);
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
