#include "Precompiled.h"
#include <Windowsx.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>

HRESULT WINAPI TaskDialogCallbackProc__(__in HWND hwnd, __in UINT msg,
                                        __in WPARAM wParam, __in LPARAM lParam,
                                        __in LONG_PTR lpRefData) {
  UNREFERENCED_PARAMETER(lpRefData);
  UNREFERENCED_PARAMETER(wParam);
  switch (msg) {
  case TDN_CREATED:
    ::SetForegroundWindow(hwnd);
    break;
  case TDN_RADIO_BUTTON_CLICKED:
    break;
  case TDN_BUTTON_CLICKED:
    break;
  case TDN_HYPERLINK_CLICKED:
    ShellExecuteW(hwnd, NULL, (LPCTSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
    break;
  }
  return S_OK;
}

HRESULT WINAPI MessageWindowEx(HWND hWnd, LPCWSTR pszWindowTitle,
                               LPCWSTR pszContent, LPCWSTR pszExpandedInfo,
                               MessageWinodwEnum type) {
  TASKDIALOGCONFIG tdConfig;
  int nButton = 0;
  int nRadioButton = 0;
  memset(&tdConfig, 0, sizeof(tdConfig));
  tdConfig.cbSize = sizeof(tdConfig);
  tdConfig.hwndParent = hWnd;
  tdConfig.hInstance = GetModuleHandleW(nullptr);
  tdConfig.pszWindowTitle = pszWindowTitle;
  tdConfig.pszContent = pszContent;
  tdConfig.pszExpandedInformation = pszExpandedInfo;
  if (pszExpandedInfo) {
    tdConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_EXPAND_FOOTER_AREA |
                       TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT |
                       TDF_ENABLE_HYPERLINKS;
  } else {
    tdConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION |
                       TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
  }

  tdConfig.nDefaultRadioButton = nRadioButton;
  switch (type) {
  case kInfoWindow:
    tdConfig.pszMainIcon = TD_INFORMATION_ICON;
    break;
  case kWarnWindow:
    tdConfig.pszMainIcon = TD_WARNING_ICON;
    break;
  case kFatalWindow:
    tdConfig.pszMainIcon = TD_ERROR_ICON;
    break;
  case kAboutWindow: {
    if (hWnd) {
      HICON hIcon =
          reinterpret_cast<HICON>(SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0));
      if (hIcon) {
        tdConfig.hMainIcon = hIcon;
        tdConfig.dwFlags |= TDF_USE_HICON_MAIN;
      }
    }
  } break;
  default:
    break;
  }
  tdConfig.pszCollapsedControlText = _T("More information");
  tdConfig.pszExpandedControlText = _T("Less information");
  tdConfig.pfCallback = TaskDialogCallbackProc__;
  return TaskDialogIndirect(&tdConfig, &nButton, &nRadioButton, NULL);
}