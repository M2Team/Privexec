///
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
#include <exec.hpp>
#include "resource.h"

namespace priv {

// expand env

int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  AppInitializeSettings(as);
  return (int)DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_APPLICATION_DIALOG), NULL, App::WindowProc,
                              reinterpret_cast<LPARAM>(this));
}
App::~App() {
  if (hbrBkgnd != nullptr) {
    DeleteObject(hbrBkgnd);
    hbrBkgnd = nullptr;
  }
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

WCHAR placeholderText[] = L"Registry ACLs only support starts With: \"Registry::CLASSES_ROOT\", "
                          L"\"Registry::CURRENT_USER\", \"Registry::MACHINE\", and \"Registry::USERS\".";

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

  auto elevated = wsudo::exec::IsUserAdministratorsGroup();
  if (elevated) {
    // Update title when app run as admin
    WCHAR title[256];
    auto N = GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    wcscat_s(title, L" [Administrator]");
    SetWindowTextW(hWnd, title);
  }

  HMENU hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_APPTHEME, L"Change Panel Color");
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_APPEXEC_ABOUT, L"About AppContainer Exec\tAlt+F1");
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
  InitializeCapabilities();
  hTip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, hWnd, NULL, hInst, nullptr);
  TOOLINFO toolInfo = {0};
  toolInfo.cbSize = sizeof(toolInfo);
  toolInfo.hwnd = hWnd;
  toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  toolInfo.uId = (UINT_PTR)appx.hAcl;
  toolInfo.lpszText = placeholderText;
  ::SendMessageW(hTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
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
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
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
  return bela::WindowsExpandEnv(cmd_);
}

std::wstring App::ResolveCWD() {
  auto cwd_ = bela::WindowsExpandEnv(cwd.Content());
  if (!cwd_.empty() && (GetFileAttributesW(cwd_.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0) {
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

std::wstring_view makeSubRegistryKey(std::wstring_view path) {
  constexpr std::wstring_view prefix = {L"Registry::"};
  if (bela::StartsWithIgnoreCase(path, prefix)) {
    if (path.size() > prefix.size() && bela::IsPathSeparator(path[prefix.size()])) {
      return path.substr(prefix.size() + 1);
    }
    return path.substr(prefix.size());
  }
  return L"";
}

std::optional<std::wstring> MakeRegistryKey(std::wstring_view path) {
  auto s = makeSubRegistryKey(path);
  if (s.empty()) {
    return std::nullopt;
  }
  auto sv = bela::SplitPath(s);
  if (sv.empty()) {
    return std::nullopt;
  }
  constexpr std::wstring_view localPrefix = L"LOCAL_";
  constexpr std::wstring_view localHKeyPrefix = L"HKEY_LOCAL_";
  if (bela::StartsWithIgnoreCase(sv[0], localPrefix)) {
    sv[0].remove_prefix(localPrefix.size());
  } else if (bela::StartsWithIgnoreCase(sv[0], localPrefix)) {
    sv[0].remove_prefix(localHKeyPrefix.size());
  }
  return std::make_optional<std::wstring>(bela::StrJoin(sv, L"\\"));
}

bool App::AppLookupAcl(std::vector<std::wstring> &fsdir, std::vector<std::wstring> &registries) {
  auto acl = appx.AclText();
  for (auto it = acl.begin(); it != acl.end(); it++) {
    if (*it == L'/') {
      *it = L'\\'; // Replace all '/' to ' \\'
    }
  }
  auto dirs = NewlineTokenize(acl);
  for (auto d : dirs) {
    auto dir = bela::WindowsExpandEnv(d);
    if (auto reg = MakeRegistryKey(dir); reg && !reg->empty()) {
      trace.Append(L"Registry Access", *reg);
      registries.emplace_back(*reg);
      continue;
    }
    trace.Append(L"Fs Access", dir);
    fsdir.emplace_back(dir);
  }
  return true;
}

bool App::AppExecute() {
  trace.Clear();
  auto commandline = ResolveCMD();
  if (commandline.empty()) {
    bela::BelaMessageBox(hWnd, L"Please input command line", L"command empty", PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  bela::error_code ec;
  wsudo::exec::appcommand cmd;
  if (!wsudo::exec::SplitArgv(commandline, cmd.path, cmd.argv, ec)) {
    bela::BelaMessageBox(hWnd, L"Privexec SplitArgv failed", ec.message.data(), PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  trace.Append(L"Command", commandline);
  cmd.cwd = ResolveCWD(); // app startup directory
  //// TODO app container.
  if (!cmd.cwd.empty()) {
    trace.Append(L"CurrentDir", cmd.cwd);
  }
  cmd.caps = appx.Capabilities();
  cmd.islpac = appx.IsLowPrivilegeAppContainer();
  cmd.appid = appx.AppContainerName();
  AppLookupAcl(cmd.allowdirs, cmd.regdirs);

  if (!cmd.initialize(ec)) {
    bela::BelaMessageBox(hWnd, L"Privexec AppContainer init", ec.message.data(), PRIVEXEC_APPLINKE, bela::mbs_t::FATAL);
    return false;
  }
  if (!cmd.execute(ec)) {
    bela::BelaMessageBox(hWnd, L"Privexec create AppContainer process failed", ec.message.data(), PRIVEXEC_APPLINKE,
                         bela::mbs_t::FATAL);
    return false;
  }
  trace.Append(L"AppContainer Name", cmd.appid);
  trace.Append(L"AppContainer SID", cmd.sid);
  trace.Append(L"AppContainer Folder", cmd.folder);
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

bool App::AppLookupCWD() {
  auto folder = bela::FolderPicker(hWnd, L"Appexec: Select App Launcher Folder");
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
    cmd.Update(quotesText(dfile));
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
      bela::BelaMessageBox(hWnd, L"About AppContainer Exec", PRIVEXEC_APPVERSION, PRIVEXEC_APPLINK, bela::mbs_t::ABOUT);
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

      const bela::filter_t filters[] = {{L"Windows Appxmanifest (*.appxmanifest;*.xml)", L"*.appxmanifest;*.xml"},
                                        {L"All Files (*.*)", L"*.*"}};
      auto xml = bela::FilePicker(hWnd, L"AppExec: Select AppManifest", filters);
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
