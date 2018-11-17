////
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <process/process.hpp>
#include "../Privexec.Console/Privexec.Console.hpp"
#include <version.h>

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
    for (auto c : realcmd) {
      switch (c) {
      case L'"':
        buf.push_back(L'\\');
        buf.push_back(L'"');
        break;
      case L'\t':
        needwarp = true;
        break;
      case L' ':
        needwarp = true;
      default:
        buf.push_back(c);
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
    auto xlen = wcslen(cmd);
    if (xlen == 0) {
      argv_.append(L" \"\"");
      return *this;
    }
    auto end = cmd + xlen;
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
  priv::process p(argb.str());
  console::Print(console::fc::Yellow, L"Command: %s\n", argb.str());
  if (p.execute(level)) {
    console::Print(console::fc::Green, L"new process is running: %d\n",
                   p.pid());
    return true;
  }
  auto ec = priv::error_code::lasterror();
  console::Print(console::fc::Red, L"create process  last error %d  (%s): %s\n",
                 ec.code, p.message().c_str(), ec.message);
  return false;
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *longname) {
  return (wcscmp(candidate, longname) == 0);
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *shortname,
                  const wchar_t *longname) {
  return (wcscmp(candidate, shortname) == 0 ||
          (longname != nullptr && wcscmp(candidate, longname) == 0));
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
      {L"a", priv::ProcessAppContainer},
      {L"m", priv::ProcessMandatoryIntegrityControl},
      {L"u", priv::ProcessNoElevated},
      {L"A", priv::ProcessElevated},
      {L"s", priv::ProcessSystem},
      {L"t", priv::ProcessTrustedInstaller}};
  auto iter = users.find(user);
  if (iter != users.end()) {
    return iter->second;
  }
  return -1;
}

const wchar_t *kUsage = LR"(execute some app
usage: wsudo command args....
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
    console::Print(console::fc::Cyan, L"wsudo %s\n", PRIVEXEC_BUILD_VERSION);
    return 0;
  }
  if (IsArg(Arg, L"-h", L"--help")) {
    console::Print(console::fc::Cyan, L"wsudo \x2665 %s", kUsage);
    return 0;
  }
  int index = 1;
  auto Argv = argv + 1;
  int level = priv::ProcessNoElevated;
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