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

  HMENU hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_APPEXEC_ABOUT,
              L"About Privexec\tAlt+F1");
  cmd.hInput = GetDlgItem(hWnd, IDC_COMMAND_COMBOX);
  cmd.hButton = GetDlgItem(hWnd, IDB_COMMAND_TARGET);
  AppAliasInitialize(cmd.hInput, alias); // Initialize app alias
  ///
  cwd.hInput = GetDlgItem(hWnd, IDE_APPSTARTUP);
  cwd.hButton = GetDlgItem(hWnd, IDB_APPSTARTUP);
  appx.hAcl= GetDlgItem(hWnd, IDE_APPCONTAINER_ACL);
  appx.hName= GetDlgItem(hWnd, IDE_APPCONTAINER_NAME);
  appx.hlview = GetDlgItem(hWnd, IDL_APPCONTAINER_LISTVIEW);
  appx.hlpacbox = GetDlgItem(hWnd, IDC_LPACMODE);
    hINFO = GetDlgItem(hWnd, IDE_APPEXEC_INFO);
  InitializeCapabilities();

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

bool App::AppLookupAcl(std::vector<std::wstring> &fsdir,
                       std::vector<std::wstring> &registries) {
  //
  return true;
}

bool App::AppExecute() {
  constexpr const wchar_t *ei =
      L"Ask for help with this issue. \nVisit: <a "
      L"href=\"https://github.com/M2Team/Privexec/issues\">Privexec Issues</a>";
  auto cmd_ = ResolveCMD();
  if (cmd_.empty()) {
    utils::PrivMessageBox(hWnd, L"Please input command line", L"command empty",
                          ei, utils::kFatalWindow);
    return false;
  }
  auto cwd_ = ResolveCWD(); // app startup directory
                            //// TODO app container.
  auto cas = appx.Capabilities();
  priv::appcontainer p(cmd_);
  p.enablelpac(appx.IsLowPrivilegeAppContainer());
  p.cwd().assign(cwd_);
  if (!p.initialize(cas) || !p.execute()) {
    auto ec = priv::error_code::lasterror();
    if (!p.message().empty()) {
      ec.message.append(L" (").append(p.message()).append(L")");
    }
    utils::PrivMessageBox(hWnd, L"Privexec create appconatiner process failed",
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
  HDROP hDrop = (HDROP)wParam;
  UINT nfilecounts = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
  WCHAR dfile[4096] = {0};
  for (UINT i = 0; i < nfilecounts; i++) {
    DragQueryFileW(hDrop, i, dfile, 4096);
    cmd.Update(dfile);
  }
  DragFinish(hDrop);
  return true;
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
  case WM_DROPFILES:
    DropFiles(wParam, lParam);
    return TRUE;
  case WM_SYSCOMMAND:
    switch (LOWORD(wParam)) {
    case IDM_APPEXEC_ABOUT:
      utils::PrivMessageBox(
          hWnd, L"About Privexec",
          L"Prerelease:"
          L" " PRIVEXEC_BUILD_VERSION L"\nCopyright \xA9 2019, Force "
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
    case IDB_COMMAND_TARGET: /// lookup command
      AppLookupExecute();
      return TRUE;
    case IDB_APPSTARTUP: // select startup dir
      AppLookupCWD();
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
