#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/stdwriter.hpp>
#include <bela/finaly.hpp>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <sal.h>
#include <WinNT.h>
#include <processthreadsapi.h>
#include <io.h>
#include <time.h>

class EdgeImpersonate {
public:
  static constexpr std::wstring_view EdgeProcName = L"MicrosoftEdge.exe";
  EdgeImpersonate() noexcept = default;
  EdgeImpersonate(const EdgeImpersonate &) = delete;
  EdgeImpersonate &operator=(EdgeImpersonate &) = delete;
  ~EdgeImpersonate();
  bool PickEdge();
  bool GetEdgeToken();
  bool Launch();

private:
  DWORD edgepid{0};
  HANDLE sandboxToken{INVALID_HANDLE_VALUE};
};

EdgeImpersonate::~EdgeImpersonate() {
  if (sandboxToken != INVALID_HANDLE_VALUE) {
    CloseHandle(sandboxToken);
  }
}

bool EdgeImpersonate::PickEdge() {
  HANDLE hProcessSnap = INVALID_HANDLE_VALUE;
  auto deleter = bela::finally([&] {
    if (hProcessSnap != INVALID_HANDLE_VALUE) {
      CloseHandle(hProcessSnap);
    }
  });
  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"CreateToolhelp32Snapshot: %s\n", ec.message);
    return false;
  }
  PROCESSENTRY32W pe32;
  ZeroMemory(&pe32, sizeof(PROCESSENTRY32W));
  pe32.dwSize = sizeof(PROCESSENTRY32W);
  if (Process32FirstW(hProcessSnap, &pe32) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"Process32FirstW: %s\n", ec.message);
    return false;
  }
  do {
    if (bela::EqualsIgnoreCase(EdgeProcName, pe32.szExeFile)) {
      edgepid = pe32.th32ProcessID;
      return true;
    }
  } while (Process32NextW(hProcessSnap, &pe32) == TRUE);
  bela::FPrintF(stderr, L"Microsoft Edge not running\n");
  return false;
}

bool EdgeImpersonate::GetEdgeToken() {
  auto edgeHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, edgepid);
  if (edgeHandle == nullptr) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"OpenProcess: %s\n", ec.message);
    return false;
  }
  auto deleter = bela::finally([&] {
    if (edgeHandle != nullptr) {
      CloseHandle(edgeHandle);
    }
  });
  if (OpenProcessToken(edgeHandle, TOKEN_DUPLICATE, &sandboxToken) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"OpenProcessToken: %s\n", ec.message);
    return false;
  }
  return true;
}

bool EdgeImpersonate::Launch() {
  HANDLE ImpersonatePrimaryToken;
  if (DuplicateTokenEx(sandboxToken, MAXIMUM_ALLOWED, nullptr,
                       SecurityImpersonation, TokenPrimary,
                       &ImpersonatePrimaryToken) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"DuplicateTokenEx: %s\n", ec.message);
    return false;
  }
  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(STARTUPINFOW);
  PROCESS_INFORMATION pi;
  wchar_t cmd[256] = L"notepad.exe";
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  if (CreateProcessAsUserW(ImpersonatePrimaryToken, nullptr, cmd, nullptr,
                           nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr,
                           &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"CreateProcessAsUserW: %s\n", ec.message);
    CloseHandle(ImpersonatePrimaryToken);
    return false;
  }
  bela::FPrintF(stderr, L"create new process %d success\n", pi.dwProcessId);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(ImpersonatePrimaryToken);
  return true;
}

int wmain() {
  EdgeImpersonate ei;
  if (!ei.PickEdge()) {
    return 1;
  }
  if (!ei.GetEdgeToken()) {
    return 1;
  }
  if (ei.Launch()) {
    return 0;
  }
  return 1;
}