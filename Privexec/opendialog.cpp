#include "Precompiled.h"
#include <ShlObj.h>
#include <Uxtheme.h>
#include <atlbase.h>
#include <atlcoll.h>
#include <atlsimpstr.h>
#include <atlstr.h>
#include <atlwin.h>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#pragma warning(disable : 4091)
#include <Shlwapi.h>

using namespace ATL;

typedef COMDLG_FILTERSPEC FilterSpec;

const FilterSpec filterSpec[] = {
    {L"Windows Execute(*.exe;*.com;*.bat)", L"*.exe;*.com;*.bat"},
    {L"All Files (*.*)", L"*.*"}};

const FilterSpec mfilterSpec[] = {
    {L"Windows Appxmanifest (*.appxmanifest;*.xml)", L"*.appxmanifest;*.xml"},
    {L"All Files (*.*)", L"*.*"}};

void ReportErrorMessage(LPCWSTR pszFunction, HRESULT hr) {
  wchar_t szBuffer[65535] = {0};
  if (SUCCEEDED(StringCchPrintf(
          szBuffer, ARRAYSIZE(szBuffer),
          L"Call: %s Failed w/hr 0x%08lx ,Please Checker Error Code!",
          pszFunction, hr))) {
    int nB = 0;
    TaskDialog(nullptr, GetModuleHandle(nullptr), L"Failed information",
               L"Call Function Failed:", szBuffer, TDCBF_OK_BUTTON,
               TD_ERROR_ICON, &nB);
  }
}
bool PrivexecDiscoverWindow(HWND hParent, std::wstring &filename,
                             const wchar_t *pszWindowTitle, int type) {
  HRESULT hr = S_OK;
  IFileOpenDialog *pWindow = nullptr;
  IShellItem *pItem = nullptr;
  PWSTR pwszFilePath = nullptr;
  if (CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(&pWindow)) != S_OK) {
    ReportErrorMessage(L"FileOpenWindowProvider", hr);
    return false;
  }
  switch (type) {
  case kExecute:
    hr = pWindow->SetFileTypes(sizeof(filterSpec) / sizeof(filterSpec[0]),
                               filterSpec);
    hr = pWindow->SetFileTypeIndex(1);
    break;
  case kAppmanifest:
    hr = pWindow->SetFileTypes(sizeof(mfilterSpec) / sizeof(mfilterSpec[0]),
                               mfilterSpec);
    hr = pWindow->SetFileTypeIndex(1);
    break;
  default:
    ReportErrorMessage(L"unsupported openfile dialog type", hr);
    return false;
  }

  hr = pWindow->SetTitle(pszWindowTitle ? pszWindowTitle
                                        : L"Open File Provider");
  hr = pWindow->Show(hParent);
  if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
    // User cancelled.
    hr = S_OK;
    goto done;
  }
  if (FAILED(hr)) {
    goto done;
  }
  hr = pWindow->GetResult(&pItem);
  if (FAILED(hr))
    goto done;
  hr = pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pwszFilePath);
  if (FAILED(hr)) {
    goto done;
  }
  filename.assign(pwszFilePath);
done:
  if (pwszFilePath) {
    CoTaskMemFree(pwszFilePath);
  }
  if (pItem) {
    pItem->Release();
  }
  if (pWindow) {
    pWindow->Release();
  }
  return hr == S_OK;
}
