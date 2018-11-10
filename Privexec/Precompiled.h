#ifndef PRECOMPILED_H
#define PRECOMPILED_H
#pragma once
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <Windowsx.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
#include <PathCch.h>
#include <string>

enum MessageWinodwEnum { kInfoWindow, kWarnWindow, kFatalWindow, kAboutWindow };

HRESULT WINAPI MessageWindowEx(HWND hWnd, LPCWSTR pszWindowTitle,
                               LPCWSTR pszContent, LPCWSTR pszExpandedInfo,
                               MessageWinodwEnum type);

enum OpenDialogType : int { kExecute, kAppmanifest };
bool PriveexecDiscoverWindow(HWND hParent, std::wstring &filename,
                             const wchar_t *pszWindowTitle,int type);

#endif // !PRECOMPILED_H
