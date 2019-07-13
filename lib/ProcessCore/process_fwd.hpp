///
#ifndef PRIVEXEC_PROCESS_FWD_HPP
#define PRIVEXEC_PROCESS_FWD_HPP
#include <string_view>
#include <bela/base.hpp>
#include <bela/span.hpp>
#include <bela/escapeargv.hpp>

namespace priv {
// process execute level
enum class El {
  None = -1,
  AppContainer,
  MIC,
  NoElevated,
  Elevated,
  System,
  TrustedInstaller
};
enum class VisibleMode {
  None = 0, // not set
  NewConsole,
  Hide
};

inline void CloseHandleEx(HANDLE &h) {
  if (h != INVALID_HANDLE_VALUE) {
    ::CloseHandle(h);
    h = INVALID_HANDLE_VALUE;
  }
}

class Process {
public:
  Process() = default;
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  Process(std::wstring_view cmd_) : cmd(cmd_) {}
  template <typename... Args> Process(std::wstring_view s, Args... args) {
    std::wstring_view svvv = {s, args...};
    bela::EscapeArgv ea;
    ea.AssignFull(svv);
    cmd = ea.sv();
  }
  std::wstring_view Chdir(std::wstring_view dir) {
    cwd = dir;
    return cwd;
  }
  VisibleMode ChangeVisibleMode(VisibleMode visible_) {
    visible = visible_;
    return visible;
  }
  bool Exec(El el = El::None);
  const std::wstring_view Message() const { return kmessage; }

private:
  bool ExecNone();
  bool ExecNoElevated();
  bool ExecElevated();
  bool ExecSystem();
  bool ExecTI();
  bool ExecWithToken(HANDLE hToken);
  bool ExecWithAppContainer();
  std::wstring cmd;
  std::wstring cwd;
  std::wstring kmessage;
  DWORD pid{0};
  VisibleMode visible{VisibleMode::None}; ///
};

} // namespace priv

#endif