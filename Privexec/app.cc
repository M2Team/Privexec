///
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
#include <bela/simulator.hpp>
#include <bela/picker.hpp>
#include <exec.hpp>
#include "resource.h"

namespace priv {

int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  return (int)DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_APPLICATION_DIALOG), NULL, App::WindowProc,
                              reinterpret_cast<LPARAM>(this));
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

  auto elevated = wsudo::exec::IsUserAdministratorsGroup();
  if (elevated) {
    // Update title when app run as admin
    WCHAR title[256];
    auto N = GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    wcscat_s(title, L" [Administrator]");
    SetWindowTextW(hWnd, title);
  }
  box = GetDlgItem(hWnd, IDC_USER_COMBOX);

  box.Append((int)wsudo::exec::privilege_t::appcontainer, L"AppContainer");
  box.Append((int)wsudo::exec::privilege_t::mic, L"Mandatory Integrity Control");
  if (elevated) {
    box.Append((int)wsudo::exec::privilege_t::standard, L"Not Elevated (UAC)", true);
    box.Append((int)wsudo::exec::privilege_t::elevated, L"Administrator");
    box.Append((int)wsudo::exec::privilege_t::system, L"System");
    box.Append((int)wsudo::exec::privilege_t::trustedinstaller, L"TrustedInstaller");
  } else {
    box.Append((int)wsudo::exec::privilege_t::standard, L"Not Elevated (UAC)");
    box.Append((int)wsudo::exec::privilege_t::elevated, L"Administrator", true);
  }
  HMENU hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_EDIT_ALIASFILE, L"Open Privexec.json");
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

bool App::AppAliasEdit() {
  auto aliasFile = priv::AppAliasFile();
  wsudo::exec::command cmd;
  std::wstring code;
  if (bela::env::LookPath(L"code.cmd", code)) {
    cmd.path = std::move(code);
    cmd.argv.emplace_back(L"code.cmd");
    cmd.visible = wsudo::exec::visible_t::hide; // hide console window
  } else {
    cmd.argv.emplace_back(L"notepad.exe");
  }
  cmd.argv.emplace_back(std::move(aliasFile));
  cmd.priv = wsudo::exec::privilege_t::standard;
  bela::error_code ec;
  if (!cmd.execute(ec)) {
    bela::BelaMessageBox(hWnd, L"open editor edit alias failed", ec.message.data(), nullptr, bela::mbs_t::FATAL);
    return false;
  }
  return true;
}

bool App::SelChanged() {
  if (box.IsMatch((int)wsudo::exec::privilege_t::appcontainer)) {
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
  auto commandline = ResolveCMD();
  if (commandline.empty()) {
    bela::BelaMessageBox(hWnd, L"Please input command line", L"command empty", PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  bela::error_code ec;
  if (appindex == (int)wsudo::exec::privilege_t::appcontainer) {
    wsudo::exec::appcommand cmd;
    if (!wsudo::exec::SplitArgv(commandline, cmd.path, cmd.argv, ec)) {
      bela::BelaMessageBox(hWnd, L"Privexec SplitArgv failed", ec.message.data(), PRIVEXEC_APPLINKE,
                           bela::mbs_t::FATAL);
      return false;
    }
    cmd.caps = appcas.Capabilities();
    cmd.appmanifest = bela::WindowsExpandEnv(appx.Content());
    cmd.islpac = appcas.IsLowPrivilegeAppContainer();
    cmd.cwd = ResolveCWD(true);
    if (!cmd.initialize(ec)) {
      bela::BelaMessageBox(hWnd, L"Privexec AppContainer init", ec.message.data(), PRIVEXEC_APPLINKE,
                           bela::mbs_t::FATAL);
      return false;
    }
    if (!cmd.execute(ec)) {
      bela::BelaMessageBox(hWnd, L"Privexec create AppContainer process failed", ec.message.data(), PRIVEXEC_APPLINKE,
                           bela::mbs_t::FATAL);
      return false;
    }
    return true;
  }

  wsudo::exec::command cmd;
  cmd.priv = static_cast<wsudo::exec::privilege_t>(appindex);
  if (!wsudo::exec::SplitArgv(commandline, cmd.path, cmd.argv, ec)) {
    bela::BelaMessageBox(hWnd, L"Privexec SplitArgv failed", ec.message.data(), PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  cmd.cwd = ResolveCWD();
  if (!cmd.execute(ec)) {
    bela::BelaMessageBox(hWnd, L"Privexec create process failed", ec.message.data(), PRIVEXEC_APPLINKE,
                         bela::mbs_t::FATAL);
    return false;
  }
  return true;
}

inline std::wstring quotesText(std::wstring_view text) {
  if (auto pos = text.find(L' '); pos != std::wstring_view::npos) {
    return bela::StringCat(L"\"", text, L"\"");
  }
  return std::wstring(text);
}

bool App::AppLookupExecute() {
  const bela::filter_t filters[] = {{L"Windows Execute(*.exe;*.com;*.bat;*.cmd)", L"*.exe;*.com;*.bat;*.cmd"},
                                    {L"All Files (*.*)", L"*.*"}};
  auto exe = bela::FilePicker(hWnd, L"Privexec: Select Execute", filters);
  if (exe) {
    cmd.Update(quotesText(*exe));
    return true;
  }
  return false;
}

bool App::AppLookupManifest() {
  const bela::filter_t filters[] = {{L"Windows Appxmanifest (*.appxmanifest;*.xml)", L"*.appxmanifest;*.xml"},
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
    cmd.Update(quotesText(dfile));
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
      bela::BelaMessageBox(hWnd, L"About Privexec", PRIVEXEC_APPVERSION, PRIVEXEC_APPLINK, bela::mbs_t::ABOUT);
      break;
    case IDM_EDIT_ALIASFILE:
      AppAliasEdit();
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
