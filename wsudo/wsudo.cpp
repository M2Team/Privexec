#include <string>
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "../Privexec.Core/Privexec.Core.hpp"
#include "../Privexec.Console/Privexec.Console.hpp"

class ArgvBuffer {
public:
  ArgvBuffer() : argv_(4096, L'\0') {}
  ArgvBuffer &assign(const wchar_t *app) {
    std::wstring realcmd(0x8000, L'\0');
    auto N = ExpandEnvironmentStringsW(app, &realcmd[0], 0x8000);
    realcmd.resize(N);
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
  ArgvBuffer &append(const wchar_t *cmd) {
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
        break;
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
  std::wstring &str() { return argv_; }
  LPWSTR cmdline() { return &argv_[0]; }

private:
  std::wstring argv_;
};

int wmain(int argc, wchar_t **argv) {
  /// wsudo -U=user
  auto Args = GetCommandLineW();
  console::Print(console::fc::Cyan, L"%s Cyan\n", argv[0]);
  console::Print(console::fc::Red, L"%s Read\n", argv[0]);
  return 0;
}