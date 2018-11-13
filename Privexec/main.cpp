////
#define NOMINMAX
#include "Precompiled.h"
#include "resource.h"
#include <Shlobj.h>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
/// include json.hpp
#include "../Privexec.Core/Privexec.Core.hpp"
#include "../inc/version.h"
#include "json.hpp"

inline std::wstring utf8towide(std::string_view str) {
  std::wstring wstr;
  auto N =
      MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
  if (N > 0) {
    wstr.resize(N);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], N);
  }
  return wstr;
}

inline std::wstring PWDExpand(const std::wstring &d) {
  std::wstring xd(PATHCCH_MAX_CCH, L'\0');
  if (d.empty()) {
    auto N = GetCurrentDirectoryW(PATHCCH_MAX_CCH, &xd[0]);
    xd.resize(N > 0 ? (size_t)N : 0);
    return xd;
  }
  auto N = ExpandEnvironmentStringsW(d.c_str(), &xd[0], PATHCCH_MAX_CCH);
  if (N <= 0) {
    return L"";
  }
  xd.resize(N);
  if ((GetFileAttributesW(xd.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return xd;
  }
  return L"";
}

class DotComInitialize {
public:
  DotComInitialize() {
    if (FAILED(CoInitialize(NULL))) {
      throw std::runtime_error("CoInitialize failed");
    }
  }
  ~DotComInitialize() { CoUninitialize(); }
};

HINSTANCE g_hInst = nullptr;

std::vector<std::pair<int, const wchar_t *>> users;
std::unordered_map<std::wstring, std::wstring> Aliascmd;

bool InitializeCombobox(HWND hCombox) {
  int usersindex = 0;
  users.push_back(std::make_pair(kAppContainer, L"App Container"));
  users.push_back(std::make_pair(kMandatoryIntegrityControl,
                                 L"Mandatory Integrity Control"));
  users.push_back(std::make_pair(kUACElevated, L"UAC Elevated"));
  users.push_back(std::make_pair(kAdministrator, L"Administrator"));
  usersindex = (int)(users.size() - 1);
  if (priv::IsUserAdministratorsGroup()) {
    users.push_back(std::make_pair(kSystem, L"System"));
    users.push_back(std::make_pair(kTrustedInstaller, L"TrustedInstaller"));
  }
  for (const auto &i : users) {
    ::SendMessageW(hCombox, CB_ADDSTRING, 0, (LPARAM)i.second);
  }
  ::SendMessageW(hCombox, CB_SETCURSEL, (WPARAM)usersindex, 0);
  return true;
}

bool Execute(int cur, const std::wstring &cmdline, const std::wstring &pwd) {
  auto iter = Aliascmd.find(cmdline);

  std::wstring xcmd(PATHCCH_MAX_CCH, L'\0');
  DWORD N = 0;
  if (iter != Aliascmd.end()) {
    N = ExpandEnvironmentStringsW(iter->second.data(), &xcmd[0],
                                  PATHCCH_MAX_CCH);
  } else {
    N = ExpandEnvironmentStringsW(cmdline.data(), &xcmd[0], PATHCCH_MAX_CCH);
  }

  xcmd.resize(N - 1);
  if (cur >= users.size()) {
    return false;
  }
  auto xpwd = PWDExpand(pwd);
  DWORD dwProcessId;
  return priv::PrivCreateProcess(users[cur].first, &xcmd[0], dwProcessId,
                                 xpwd.empty() ? nullptr : &xpwd[0]);
}

bool PathAppImageCombineExists(std::wstring &file, const wchar_t *cmd) {
  if (PathFileExistsW(L"Privexec.json")) {
    file.assign(L"Privexec.json");
    return true;
  }
  file.resize(PATHCCH_MAX_CCH, L'\0');
  auto pszFile = &file[0];
  auto N = GetModuleFileNameW(nullptr, pszFile, PATHCCH_MAX_CCH);
  if (PathCchRemoveExtension(pszFile, N + 8) != S_OK) {
    return false;
  }

  if (PathCchAddExtension(pszFile, N + 8, L".json") != S_OK) {
    return false;
  }
  auto k = wcslen(pszFile);
  file.resize(k);
  if (PathFileExistsW(file.data()))
    return true;
  file.clear();
  return false;
}

bool InitializePrivApp(HWND hWnd) {
  std::wstring file;
  if (!PathAppImageCombineExists(file, L"Privexec.json")) {
    return false;
  }
  auto hCombox = GetDlgItem(hWnd, IDC_COMMAND_COMBOX);
  if (priv::IsUserAdministratorsGroup()) {
    WCHAR title[256];
    auto N = GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    wcscat_s(title, L" [Administrator]");
    SetWindowTextW(hWnd, title);
  }
  try {
    std::ifstream fs;
    fs.open(file);
    auto json = nlohmann::json::parse(fs);
    auto cmds = json["Alias"];
    for (auto &cmd : cmds) {
      auto name = utf8towide(cmd["Name"].get<std::string>());
      auto command = utf8towide(cmd["Command"].get<std::string>());
      Aliascmd.insert(std::make_pair(name, command));
      ::SendMessage(hCombox, CB_ADDSTRING, 0, (LPARAM)name.data());
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}

std::unordered_map<HWND, WELL_KNOWN_SID_TYPE> capchecks;

bool ChangeCapabilitiesVisible(bool enable) {
  for (auto c : capchecks) {
    EnableWindow(c.first, enable ? TRUE : FALSE);
  }
  return true;
}

bool InitializeCapabilities(HWND hParent) {
  //
  auto func = [&](int id, WELL_KNOWN_SID_TYPE wsid) {
    auto h = GetDlgItem(hParent, id);
    capchecks.insert(std::pair<HWND, WELL_KNOWN_SID_TYPE>(h, wsid));
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

bool UpdateCapabilities(HWND hParent, const std::wstring &file) {
  //
  std::vector<WELL_KNOWN_SID_TYPE> cas;
  if (!priv::WellKnownFromAppmanifest(file, cas)) {
  }
  for (auto &c : capchecks) {
    if (std::find(cas.begin(), cas.end(), c.second) != cas.end()) {
      Button_SetCheck(c.first, TRUE);
    } else {
      Button_SetCheck(c.first, FALSE);
    }
  }
  return true;
}

bool ExecuteAppcontainer(const std::wstring &cmdline, const std::wstring &pwd) {
  auto iter = Aliascmd.find(cmdline);
  std::wstring xcmd(PATHCCH_MAX_CCH, L'\0');
  DWORD N = 0;
  if (iter != Aliascmd.end()) {
    N = ExpandEnvironmentStringsW(iter->second.data(), &xcmd[0],
                                  PATHCCH_MAX_CCH);
  } else {
    N = ExpandEnvironmentStringsW(cmdline.data(), &xcmd[0], PATHCCH_MAX_CCH);
  }
  xcmd.resize(N - 1);

  std::vector<WELL_KNOWN_SID_TYPE> cas;
  for (auto &c : capchecks) {
    if (Button_GetCheck(c.first) == BST_CHECKED) {
      cas.push_back((WELL_KNOWN_SID_TYPE)c.second);
    }
  }

  priv::AppContainerContext ctx;
  if (!ctx.InitializeWithCapabilities(cas.data(), (int)cas.size())) {
    return false;
  }
  auto xpwd = PWDExpand(pwd);
  DWORD dwProcessId;
  return ctx.Execute(&xcmd[0], dwProcessId, xpwd.empty() ? nullptr : &xpwd[0]);
}

INT_PTR WINAPI ApplicationProc(HWND hWndDlg, UINT message, WPARAM wParam,
                               LPARAM lParam) {
  switch (message) {
  // DarkGray background
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
    // case WM_CTLCOLORBTN:
    // return (INT_PTR)CreateSolidBrush(RGB(0, 191, 255));
    return (INT_PTR)CreateSolidBrush(RGB(255, 255, 255));
  case WM_INITDIALOG: {
    HICON icon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPLICATION_ICON));
    SendMessage(hWndDlg, WM_SETICON, ICON_BIG, (LPARAM)icon);
    InitializePrivApp(hWndDlg);
    auto hCombox = GetDlgItem(hWndDlg, IDC_USER_COMBOX);
    InitializeCombobox(hCombox);
    HMENU hSystemMenu = ::GetSystemMenu(hWndDlg, FALSE);
    InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_PRIVEXEC_ABOUT,
                L"About Privexec\tAlt+F1");
    Edit_SetText(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST),
                 L"AppxManifest.xml or Package.appxmanifest");
    EnableWindow(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST),
                 FALSE); // Disable AppContainer SID
    InitializeCapabilities(hWndDlg);
    ChangeCapabilitiesVisible(false);
    return 0;
  } break;
  case WM_SYSCOMMAND:
    switch (LOWORD(wParam)) {
    case IDM_PRIVEXEC_ABOUT: {
      MessageWindowEx(
          hWndDlg, L"About Privexec",
          L"Prerelease:"
          L" " PRIVEXEC_BUILD_VERSION L"\nCopyright \xA9 2018, Force "
          L"Charlie. All Rights Reserved.",
          L"For more information about this tool.\nVisit: <a "
          L"href=\"http://forcemz.net/\">forcemz.net</a>",
          kAboutWindow);
    }
      return 0;
    default:
      return DefWindowProcW(hWndDlg, message, wParam, lParam);
    }
    break;
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDC_USER_COMBOX: {
      auto wmEvent = HIWORD(wParam);
      if (wmEvent == CBN_SELCHANGE) {
        auto N = SendMessage(GetDlgItem(hWndDlg, IDC_USER_COMBOX), CB_GETCURSEL,
                             0, 0);
        if (N == kAppContainer) {
          EnableWindow(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST), TRUE);
          SetWindowTextW(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST),
                         L"");
          ChangeCapabilitiesVisible(true);
        } else {
          EnableWindow(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST),
                       FALSE);
          SetWindowTextW(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST),
                         L"AppxManifest.xml or Package.appxmanifest");
          ChangeCapabilitiesVisible(false);
        }
      }
    } break;
    case IDB_COMMAND_TARGET: {
      std::wstring cmd;
      if (PrivexecDiscoverWindow(hWndDlg, cmd, L"Privexec: Open Execute",
                                 kExecute)) {
        SetWindowTextW(GetDlgItem(hWndDlg, IDC_COMMAND_COMBOX), cmd.c_str());
      }
    } break;
    case IDB_APPSTARTUP: {
      std::wstring pwd;
      if (FolderOpenWindow(hWndDlg, pwd, L"Privexec: Startup Folder")) {
        SetWindowTextW(GetDlgItem(hWndDlg, IDE_APPSTARTUP), pwd.c_str());
      }
    } break;
    case IDB_APPCONTAINER_BUTTON: {
      std::wstring manifest;
      if (PrivexecDiscoverWindow(hWndDlg, manifest,
                                 L"Privexec: Open AppManifest", kAppmanifest)) {
        SetWindowTextW(GetDlgItem(hWndDlg, IDE_APPCONTAINER_APPMANIFEST),
                       manifest.c_str());
        UpdateCapabilities(hWndDlg, manifest);
      }
    } break;
    case IDB_EXECUTE_BUTTON: {
      std::wstring cmd;
      std::wstring folder;
      auto hCombox = GetDlgItem(hWndDlg, IDC_COMMAND_COMBOX);
      auto Length = GetWindowTextLengthW(hCombox);
      if (Length == 0) {
        return 0;
      }
      cmd.resize(Length + 1);
      GetWindowTextW(hCombox, &cmd[0], Length + 1); //// Null T
      cmd.resize(Length);
      ///
      auto hPWD = GetDlgItem(hWndDlg, IDE_APPSTARTUP);
      Length = GetWindowTextLengthW(hPWD);
      folder.resize(Length + 1);
      GetWindowTextW(hPWD, &folder[0], Length + 1); //// Null T
      folder.resize(Length);

      auto N =
          SendMessage(GetDlgItem(hWndDlg, IDC_USER_COMBOX), CB_GETCURSEL, 0, 0);
      ::EnableWindow(GetDlgItem(hWndDlg, IDB_EXECUTE_BUTTON), FALSE);
      if (N == kAppContainer) {
        if (!ExecuteAppcontainer(cmd, folder)) {
          priv::ErrorMessage err(GetLastError());
          MessageBoxW(hWndDlg, err.message(),
                      L"Privexec create appconatiner process failed",
                      MB_OK | MB_ICONERROR);
        }
      } else {
        if (!Execute((int)N, cmd, folder)) {
          priv::ErrorMessage err(GetLastError());
          MessageBoxW(hWndDlg, err.message(), L"Privexec create process failed",
                      MB_OK | MB_ICONERROR);
        }
      }
      ::EnableWindow(GetDlgItem(hWndDlg, IDB_EXECUTE_BUTTON), TRUE);
    } break;
    case IDB_EXIT_BUTTON:
      DestroyWindow(hWndDlg);
      break;
    }
    return 0;
  }

  case WM_CLOSE: {
    DestroyWindow(hWndDlg);
    return 0;
  }

  case WM_DESTROY: {
    PostQuitMessage(0);
    return 0;
  }
  default:
    break;
  }
  return FALSE;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  g_hInst = hInstance;
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);
  DialogBoxParamW(hInstance, MAKEINTRESOURCE(IDD_APPLICATION_DIALOG), NULL,
                  ApplicationProc, 0L);
  return 0;
}
