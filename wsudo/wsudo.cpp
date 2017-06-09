#include <string>
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <unordered_map>
#include <iostream>
#include "../Privexec.Core/Privexec.Core.hpp"
#include "../Privexec.Console/Privexec.Console.hpp"

class Arguments {
public:
  Arguments() : argv_(4096, L'\0') {}
  Arguments &assign(const wchar_t *app) {
    std::wstring realcmd(0x8000, L'\0');
    //// N include terminating null character
    auto N = ExpandEnvironmentStringsW(app, &realcmd[0], 0x8000);
    realcmd.resize(N - 1);
    std::wstring buf;
    bool needwarp = false;
    for (auto iter = realcmd.begin(); iter != realcmd.end(); iter++) {
      switch (*iter) {
      case L'"':
        buf.push_back(L'\\');
        buf.push_back(L'"');
        break;
      case L' ':
      case L'\t':
        needwarp = true;
        break;
      default:
        buf.push_back(*iter);
        break;
      }
    }
    if (needwarp) {
      argv_.assign(L"\"").append(buf).push_back(L'"');
    } else {
      argv_.assign(std::move(buf));
    }
    return *this;
  }
  Arguments &append(const wchar_t *cmd) {
    std::wstring buf;
    bool needwarp = false;
    auto end = cmd + wcslen(cmd);
    for (auto iter = cmd; iter != end; iter++) {
      switch (*iter) {
      case L'"':
        buf.push_back(L'\\');
        buf.push_back(L'"');
        break;
      case L' ':
      case L'\t':
        needwarp = true;
      default:
        buf.push_back(*iter);
        break;
      }
    }
    if (needwarp) {
      argv_.append(L" \"").append(buf).push_back(L'"');
    } else {
      argv_.append(L" ").append(buf);
    }
    return *this;
  }
  const std::wstring &str() { return argv_; }

private:
  std::wstring argv_;
};

bool CreateProcessInternal(int level, int Argc, wchar_t **Argv) {
  Arguments argb;
  argb.assign(Argv[0]);
  for (int i = 1; i < Argc; i++) {
    argb.append(Argv[i]);
  }
  DWORD dwProcessId;
  console::Print(console::fc::Yellow, L"Command: %s\n", argb.str());
  if (PrivCreateProcess(level, const_cast<LPWSTR>(argb.str().data()),
                        dwProcessId)) {
    console::Print(console::fc::Green, L"new process is running: %d\n",
                   dwProcessId);
    return true;
  }
  ErrorMessage err(GetLastError());
  console::Print(console::fc::Red, L"create process failed: %s\n",
                 err.message());
  return false;
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *longname) {
  if (wcscmp(candidate, longname) == 0)
    return true;
  return false;
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *shortname,
                  const wchar_t *longname) {
  if (wcscmp(candidate, shortname) == 0 ||
      (longname != nullptr && wcscmp(candidate, longname) == 0))
    return true;
  return false;
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *longname, size_t n,
                  const wchar_t **off) {
  auto l = wcslen(candidate);
  if (l < n)
    return false;
  if (wcsncmp(candidate, longname, n) == 0) {
    if (l > n && candidate[n] == '=') {
      *off = candidate + n + 1;
    } else {
      *off = nullptr;
    }
    return true;
  }
  return false;
}

int SearchUser(const wchar_t *user) {
  std::unordered_map<std::wstring, int> users = {
      {L"a", kAppContainer}, {L"m", kMandatoryIntegrityControl},
      {L"u", kUACElevated},  {L"A", kAdministrator},
      {L"s", kSystem},       {L"t", kTrustedInstaller}};
  auto iter = users.find(user);
  if (iter != users.end()) {
    return iter->second;
  }
  return -1;
}

const wchar_t *kUsage = LR"(execute some app
usage: wsudo args command ....
  -v|--version   print version and exit
  -h|--help      print help information and exit
  -u|--user      run as user (optional), support '-u=X' or '-u X'
users:
   a   AppContainer
   m   Mandatory Integrity Control
   u   UAC No Elevated (default)
   A   Administrator
   s   System
   t   TrustedInstaller
Example:
   wsudo --user=A "%SYSTEMROOT%/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile
   wsudo -u=t cmd
)";

int wmain(int argc, const wchar_t *argv[]) {
  console::EnableVTMode();
  if (argc <= 1) {
    console::Print(console::fc::Red, L"usage: wsudo args\n");
    return 1;
  }
  auto Arg = argv[1];
  if (IsArg(Arg, L"-v", L"--version")) {
    console::Print(console::fc::Cyan, L"wsudo 1.0\n");
    return 0;
  }
  if (IsArg(Arg, L"-h", L"--help")) {
    console::Print(console::fc::Cyan, L"wsudo \x2665 %s", kUsage);
    return 0;
  }
  int index = 1;
  auto Argv = argv + 1;
  int level = kUACElevated;
  const wchar_t *Argoff = nullptr;
  if (IsArg(Arg, L"--user", 6, &Argoff)) {
    if (!Argoff) {
      if (argc < 3) {
        console::Print(console::fc::Red, L"Invalid Argument: %s\n", Arg);
        return 1;
      }
      Argoff = argv[2];
      index = 3;
    } else {
      index = 2;
    }
  } else if (IsArg(Arg, L"-u", 2, &Argoff)) {
    if (!Argoff) {
      if (argc < 3) {
        console::Print(console::fc::Red, L"Invalid Argument: %s\n", Arg);
        return 1;
      }
      Argoff = argv[2];
      index = 3;
    } else {
      index = 2;
    }
  } else if (Arg[0] == L'-') {
    console::Print(console::fc::Red, L"Invalid Argument: %s\n", Arg);
    return 1;
  }
  if (Argoff) {
    level = SearchUser(Argoff);
    if (level == -1) {
      console::Print(console::fc::Red, L"Invalid Argument: %s\n", Argoff);
      return 1;
    }
  }
  if (argc == index) {
    console::Print(console::fc::Red, L"Invalid Argument: %s\n",
                   GetCommandLineW());
    return 1;
  }
  if (!CreateProcessInternal(level, argc - index,
                             const_cast<wchar_t **>(argv + index))) {
    return 1;
  }
  return 0;
}