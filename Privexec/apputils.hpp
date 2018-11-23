///
#ifndef PRIVEXEC_APPUTILS_HPP
#define PRIVEXEC_APPUTILS_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <Shlwapi.h>
#include <string>
#include <optional>

namespace utils {
enum windowtype_t {
  // window type
  kInfoWindow,
  kWarnWindow,
  kFatalWindow,
  kAboutWindow
};

HRESULT PrivMessageBox(HWND hWnd, LPCWSTR pszWindowTitle, LPCWSTR pszContent,
                       LPCWSTR pszExpandedInfo, windowtype_t type);
using filter_t = COMDLG_FILTERSPEC;
std::optional<std::wstring> PrivFilePicker(HWND hWnd, const wchar_t *title,
                                           const filter_t *filter, uint32_t flen);
std::optional<std::wstring> PrivFolderPicker(HWND hWnd, const wchar_t *title);
} // namespace utils

#endif