#include "stdafx.h"
#include "Privexec.Core.hpp"


bool CreateNoElevatedProcess(LPWSTR pszCmdline, DWORD &dwProcessId) {
	if (!IsUserAdministratorsGroup()) {
		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));
		auto bResult = CreateProcessW(nullptr, pszCmdline, nullptr, nullptr, FALSE,
			0, nullptr, nullptr, &si, &pi);
		if (bResult) {
			dwProcessId = pi.dwProcessId;
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			return true;
		}
		return false;
	}
	return true;
}