#include "stdafx.h"
#include "Privexec.Core.hpp"
#include <Sddl.h>

bool CreateLowlevelProcess(LPWSTR pszCmdline, DWORD &dwProcessId) {
	HANDLE hToken;
	HANDLE hNewToken;
	PWSTR szIntegritySid = L"S-1-16-4096";
	PSID pIntegritySid = NULL;
	TOKEN_MANDATORY_LABEL TIL = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	ULONG ExitCode = 0;

	if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken)) {
		return false;
	}
	if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
		TokenPrimary, &hNewToken)) {
		CloseHandle(hToken);
		return false;
	}
	if (!ConvertStringSidToSidW(szIntegritySid, &pIntegritySid)) {
		CloseHandle(hToken);
		CloseHandle(hNewToken);
		return false;
	}
	TIL.Label.Attributes = SE_GROUP_INTEGRITY;
	TIL.Label.Sid = pIntegritySid;

	// Set process integrity levels
	if (!SetTokenInformation(hNewToken, TokenIntegrityLevel, &TIL,
		sizeof(TOKEN_MANDATORY_LABEL) +
		GetLengthSid(pIntegritySid))) {
		CloseHandle(hToken);
		CloseHandle(hNewToken);
		LocalFree(pIntegritySid);
		return false;
	}

	if (CreateProcessAsUserW(hNewToken, NULL, pszCmdline, NULL, NULL, FALSE, 0,
		NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		dwProcessId = pi.dwProcessId;
		CloseHandle(hToken);
		CloseHandle(hNewToken);
		LocalFree(pIntegritySid);
		return true;
	}
	CloseHandle(hToken);
	CloseHandle(hNewToken);
	LocalFree(pIntegritySid);
	return false;
}