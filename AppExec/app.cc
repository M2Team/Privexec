///
#include <process.hpp>
#include <bela/env.hpp>
#include <bela/path.hpp>
#include <bela/match.hpp>
#include "app.hpp" // included windows.h
#include <Windowsx.h>
// C RunTime Header Files
#include <cstdlib>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <bela/picker.hpp>
#include "resource.h"

namespace priv {

// expand env

int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  AppInitializeSettings(as);
  return (int)DialogBoxParamW(hInstance,
                              MAKEINTRESOURCEW(IDD_APPLICATION_DIALOG), NULL,
                              App::WindowProc, reinterpret_cast<LPARAM>(this));
}
App::~App() {
  if (hbrBkgnd != nullptr) {
    DeleteObject(hbrBkgnd);
    hbrBkgnd = nullptr;
  }
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
  title.Initialize(hWnd);
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
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_APPTHEME,
              L"Change Panel Color");
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_APPEXEC_ABOUT,
              L"About AppContainer Exec\tAlt+F1");
  cmd.hInput = GetDlgItem(hWnd, IDC_COMMAND_EDIT);
  cmd.hButton = GetDlgItem(hWnd, IDB_COMMAND_TARGET);
  ///
  cwd.hInput = GetDlgItem(hWnd, IDE_APPSTARTUP);
  cwd.hButton = GetDlgItem(hWnd, IDB_APPSTARTUP);
  appx.hAcl = GetDlgItem(hWnd, IDE_APPCONTAINER_ACL);
  appx.hName = GetDlgItem(hWnd, IDE_APPCONTAINER_NAME);
  appx.hlview = GetDlgItem(hWnd, IDL_APPCONTAINER_LISTVIEW);
  appx.hlpacbox = GetDlgItem(hWnd, IDC_ENABLE_LPAC);
  appx.UpdateName(L"Privexec.AppContainer.Launcher");
  ::SetFocus(cmd.hInput);
  trace.hWindow = GetDlgItem(hWnd, IDE_APPEXEC_INFO);

  // SetWindowLongPtr(trace.hInfo, GWL_EXSTYLE, 0);

  InitializeCapabilities();

  return true;
}

bool App::AppUpdateWindow() {
  if (hbrBkgnd != nullptr) {
    DeleteObject(hbrBkgnd);
    hbrBkgnd = nullptr;
  }
  ::InvalidateRect(hWnd, nullptr, TRUE);
  ::InvalidateRect(appx.hlpacbox, nullptr, TRUE);
  return true;
}

bool App::AppTheme() {
  static COLORREF CustColors[] = {
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255),
  };
  CHOOSECOLORW co;
  ZeroMemory(&co, sizeof(co));
  co.lStructSize = sizeof(CHOOSECOLOR);
  co.hwndOwner = hWnd;
  co.lpCustColors = (LPDWORD)CustColors;
  co.rgbResult = as.bk;
  co.lCustData = 0;
  co.lpTemplateName = nullptr;
  co.lpfnHook = nullptr;
  co.Flags = CC_FULLOPEN | CC_RGBINIT;
  if (ChooseColorW(&co)) {
    auto r = GetRValue(co.rgbResult);
    auto g = GetGValue(co.rgbResult);
    auto b = GetBValue(co.rgbResult);
    as.bk = RGB(r, g, b);
    as.textcolor = calcLuminance(r, g, b);
    AppUpdateWindow();
    AppApplySettings(as);
  }
  return true;
}

std::wstring App::ResolveCMD() {
  auto cmd_ = cmd.Content();
  return bela::ExpandEnv(cmd_);
}

std::wstring App::ResolveCWD() {
  auto cwd_ = bela::ExpandEnv(cwd.Content());
  if (!cwd_.empty() &&
      (GetFileAttributesW(cwd_.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    /// resolved cwd  valid
    return cwd_;
  }
  return L"";
}

std::vector<std::wstring_view> NewlineTokenize(std::wstring_view sv) {
  std::vector<std::wstring_view> output;
  size_t first = 0;
  while (first < sv.size()) {
    const auto second = sv.find_first_of(L'\n', first);
    if (first != second) {
      auto subsv = sv.substr(first, second - first);
      if (subsv.back() == L'\r') {
        subsv.remove_suffix(1);
      }
      if (!subsv.empty()) {
        output.emplace_back(subsv);
      }
    }
    if (second == std::wstring_view::npos) {
      break;
    }
    first = second + 1;
  }
  return output;
}

bool IsRegistryKey(std::wstring_view path) {
  auto pv = bela::SplitPath(path);
  if (pv.empty()) {
    return false;
  }
  constexpr std::wstring_view keys[] = {
      L"HKEY_CLASSES_ROOT",  L"HKCR", L"HKEY_CURRENT_USER", L"HKCU",
      L"HKEY_LOCAL_MACHINE", L"HKLM", L"HKEY_USERS",        L"HKU"};
  for (auto k : keys) {
    if (bela::EqualsIgnoreCase(k, pv[0])) {
      return true;
    }
  }
  return false;
}

bool App::AppLookupAcl(std::vector<std::wstring> &fsdir,
                       std::vector<std::wstring> &registries) {
  auto acl = appx.AclText();
  for (auto it = acl.begin(); it != acl.end(); it++) {
    if (*it == L'/') {
      *it = L'\\'; // Replace all '/' to ' \\'
    }
  }
  auto dirs = NewlineTokenize(acl);
  for (auto d : dirs) {
    if (IsRegistryKey(d)) {
      trace.Append(L"Registry Access", d);
      registries.emplace_back(d);
    } else {
      trace.Append(L"Fs Access", d);
      fsdir.emplace_back(d);
    }
  }
  return true;
}

bool App::AppExecute() {
  trace.Clear();
  auto cmd_ = ResolveCMD();
  if (cmd_.empty()) {
    bela::BelaMessageBox(hWnd, L"Please input command line", L"command empty",
                         PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  trace.Append(L"Command", cmd_);
  auto cwd_ = ResolveCWD(); // app startup directory
  //// TODO app container.
  if (!cwd_.empty()) {
    trace.Append(L"CurrentDir", cwd_);
  }
  auto cas = appx.Capabilities();
  priv::AppContainer p(cmd_);
  p.EnableLPAC(appx.IsLowPrivilegeAppContainer());
  p.Chdir(cwd_);
  p.Name(appx.AppContainerName());
  AppLookupAcl(p.AllowedDirs(), p.Registries());
  if (!p.Initialize({cas.data(), cas.size()}) || !p.Exec()) {
    auto ec = bela::make_system_error_code();
    if (!p.Message().empty()) {
      bela::StrAppend(&ec.message, L"(", p.Message(), L")");
    }
    bela::BelaMessageBox(hWnd, L"Appexec create appconatiner process failed",
                         ec.message.data(), PRIVEXEC_APPLINKE,
                         bela::mbs_t::FATAL);
    return false;
  }
  trace.Append(L"AppContainer Name", p.Name());
  trace.Append(L"AppContainer SID", p.SSID());
  trace.Append(L"AppContainer Folder", p.ConatinerFolder());
  return true;
}

bool App::AppLookupExecute() {
  const bela::filter_t filters[] = {
      {L"Windows Execute(*.exe;*.com;*.bat)", L"*.exe;*.com;*.bat"},
      {L"All Files (*.*)", L"*.*"}};
  auto exe = bela::FilePicker(hWnd, L"Privexec: Select Execute", filters);
  if (exe) {
    cmd.Update(*exe);
    return true;
  }
  return false;
}

bool App::AppLookupCWD() {
  auto folder =
      bela::FolderPicker(hWnd, L"Appexec: Select App Launcher Folder");
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
  switch (message) {
  case WM_CTLCOLORDLG: {
    if (hbrBkgnd == nullptr) {
      hbrBkgnd = CreateSolidBrush(as.bk);
    }
    return (INT_PTR)hbrBkgnd;
  }
  case WM_CTLCOLORSTATIC: {
    HDC hdc = (HDC)wParam;
    HWND hChild = (HWND)lParam;
    if (hChild == trace.hWindow) {
      return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }
    SetTextColor(hdc, as.textcolor);
    SetBkColor(hdc, as.bk);
    SetBkMode(hdc, TRANSPARENT);
    if (hbrBkgnd == nullptr) {
      hbrBkgnd = CreateSolidBrush(as.bk);
    }
    return (INT_PTR)hbrBkgnd;
  }
  case WM_DROPFILES:
    DropFiles(wParam, lParam);
    return TRUE;
  case WM_SYSCOMMAND:
    switch (LOWORD(wParam)) {
    case IDM_APPEXEC_ABOUT:
      bela::BelaMessageBox(hWnd, L"About AppContainer Exec",
                           PRIVEXEC_APPVERSION, PRIVEXEC_APPLINK,
                           bela::mbs_t::ABOUT);
      break;
    case IDM_APPTHEME:
      AppTheme();
      break;
    default:
      break;
    }
    break;
  case WM_COMMAND: {
    // WM_COMMAND
    switch (LOWORD(wParam)) {
    case IDB_APPX_IMPORT: {

      const bela::filter_t filters[] = {
          {L"Windows Appxmanifest (*.appxmanifest;*.xml)",
           L"*.appxmanifest;*.xml"},
          {L"All Files (*.*)", L"*.*"}};
      auto xml =
          bela::FilePicker(hWnd, L"AppExec: Select AppManifest", filters);
      if (xml) {
        ParseAppx(*xml);
      }
    }
      return TRUE;
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
