////
#include "apputils.hpp"
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>

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

namespace utils {
template <class T> class comptr {
public:
  comptr() { ptr = NULL; }
  comptr(T *p) {
    ptr = p;
    if (ptr != NULL)
      ptr->AddRef();
  }
  comptr(const comptr<T> &sptr) {
    ptr = sptr.ptr;
    if (ptr != NULL)
      ptr->AddRef();
  }
  T **operator&() { return &ptr; }
  T *operator->() { return ptr; }
  T *operator=(T *p) {
    if (*this != p) {
      ptr = p;
      if (ptr != NULL)
        ptr->AddRef();
    }
    return *this;
  }
  operator T *() const { return ptr; }
  template <class I> HRESULT QueryInterface(REFCLSID rclsid, I **pp) {
    if (pp != NULL) {
      return ptr->QueryInterface(rclsid, (void **)pp);
    } else {
      return E_FAIL;
    }
  }
  HRESULT CoCreateInstance(REFCLSID clsid, IUnknown *pUnknown,
                           REFIID interfaceId,
                           DWORD dwClsContext = CLSCTX_ALL) {
    HRESULT hr = ::CoCreateInstance(clsid, pUnknown, dwClsContext, interfaceId,
                                    (void **)&ptr);
    return hr;
  }
  ~comptr() {
    if (ptr != NULL)
      ptr->Release();
  }

private:
  T *ptr;
};

HRESULT PrivMessageBox(HWND hWnd, LPCWSTR pszWindowTitle, LPCWSTR pszContent,
                       LPCWSTR pszExpandedInfo, windowtype_t type) {
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
  tdConfig.pszCollapsedControlText = L"More information";
  tdConfig.pszExpandedControlText = L"Less information";
  tdConfig.pfCallback = TaskDialogCallbackProc__;
  return TaskDialogIndirect(&tdConfig, &nButton, &nRadioButton, NULL);
}
std::optional<std::wstring> PrivFilePicker(HWND hWnd, const wchar_t *title,
                                           const filter_t *filter,
                                           uint32_t flen) {
  if (filter == nullptr) {
    return std::nullopt;
  }
  comptr<IFileOpenDialog> window;
  comptr<IShellItem> item;
  if (window.CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                              IID_IFileOpenDialog,
                              CLSCTX_INPROC_SERVER) != S_OK) {
    return std::nullopt;
  }
  if (window->SetFileTypes(flen, filter) != S_OK) {
    return std::nullopt;
  }
  if (window->SetTitle(title == nullptr ? L"Open File Picker" : title) !=
      S_OK) {
    return std::nullopt;
  }
  if (window->Show(hWnd) != S_OK) {
    //
    return std::nullopt;
  }
  if (window->GetResult(&item) != S_OK) {
    return std::nullopt;
  }
  LPWSTR pszFilePath = nullptr;
  if (item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath) !=
      S_OK) {
    return std::nullopt;
  }
  auto opt = std::make_optional<std::wstring>(pszFilePath);
  CoTaskMemFree(pszFilePath);
  return opt;
}
std::optional<std::wstring> PrivFolderPicker(HWND hWnd, const wchar_t *title) {
  comptr<IFileDialog> w;
  if (w.CoCreateInstance(CLSID_FileOpenDialog, nullptr, IID_IFileDialog,
                         CLSCTX_INPROC_SERVER) != S_OK) {
    return std::nullopt;
  }
  DWORD dwo = 0;
  if (w->GetOptions(&dwo) != S_OK) {
    return std::nullopt;
  }
  w->SetOptions(dwo | FOS_PICKFOLDERS);
  w->SetTitle(title == nullptr ? L"Open Folder Picker" : title);
  comptr<IShellItem> item;
  if (w->Show(hWnd) != S_OK) {
    return std::nullopt;
  }
  if (w->GetResult(&item) != S_OK) {
    return std::nullopt;
  }
  LPWSTR pszFolderPath = nullptr;
  if (item->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath) != S_OK) {
    return std::nullopt;
  }
  auto opt = std::make_optional<std::wstring>(pszFolderPath);
  CoTaskMemFree(pszFolderPath);
  return opt;
}
} // namespace utils