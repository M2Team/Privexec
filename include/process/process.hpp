///
#ifndef PRIVEXEC_PROCESS_HPP
#define PRIVEXEC_PROCESS_HPP
#include <string_view>
#include <string>
#include <optional>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif

namespace priv {

struct error_code {
  std::wstring message;
  long ec{S_OK};
  bool operator()() { return ec == S_OK; }
};

class process {
public:
  template <typename... Args> process(std::wstring_view app, Args... args) {
    std::initializer_list<std::wstring_view> av = {args...};
    if (app.find(L' ') != app.end()) {
      cmd_.assign(L"\"").append(app.data(), app.size()).append(L"\" ");
    } else {
      cmd_.assign(app.data(), app.size()).append(L" ");
    }
    for (const auto &a = av) {
      if (c.empty()) {
        cmd_.append(L"\"\" ");
        continue;
      }
      if (c.find(L' ') != c.end()) {
        cmd_.assign(L"\"").append(c.data(), c.size()).append(L"\" ");
      } else {
        cmd_.assign(c.data(), c.size()).append(L" ");
      }
    }
  }
  process(const std::wstring &cmdline) : cmd_(cmdline) {}
  std::wstring &cwd() { return cwd_; }
  const std::wstring &cwd() const { return cwd_; }
  DWORD pid() const { return pid_; }
  bool execute(error_code &ec, int level);

private:
  DWORD pid_;
  std::wstring cmd_;
  std::wstring cwd_;
};

class appcontainer {
public:
private:
};

} // namespace priv

#include "details/process.ipp"

#endif