///
#ifndef PRIVEXEC_PROCESSFWD_HPP
#define PRIVEXEC_PROCESSFWD_HPP

#include <string_view>
#include <string>
#include <vector>
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
  process(const process &) = delete;
  process &operator=(const process &) = delete;

  std::wstring &cwd() { return cwd_; }
  const std::wstring &cwd() const { return cwd_; }
  const std::wstring &message() const { return kmessage; } // error reason
  DWORD pid() const { return pid_; }
  //
  bool execute(int level);

private:
  DWORD pid_;
  std::wstring cmd_;
  std::wstring cwd_;
  std::wstring kmessage;
};

using wid_t = WELL_KNOWN_SID_TYPE;
using widcontainer = std::vector<wid_t>;
class appcontainer {
public:
  using capabilities_t = std::vector<SID_AND_ATTRIBUTES>;
  template <typename... Args>
  appcontainer(std::wstring_view app, Args... args) {
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
  appcontainer(const std::wstring &cmdline) : cmd_(cmdline) {}
  appcontainer(const appcontainer &) = delete;
  appcontainer &operator=(const appcontainer &) = delete;
  ~appcontainer() {
    for (auto &c : ca) {
      if (c.Sid) {
        HeapFree(GetProcessHeap(), 0, c.Sid);
      }
    }
    if (appcontainersid != nullptr) {
      FreeSid(appcontainersid);
    }
  }

  std::wstring &cwd() { return cwd_; }
  const std::wstring &cwd() const { return cwd_; }
  DWORD pid() const { return pid_; }
  const capabilities_t &capabilities() const { return ca; }
  const std::wstring &message() const { return kmessage; } // error reason
  //
  bool initialize();
  bool initialize(const wid_t *begin, const wid_t *end);
  bool initialize(const std::wstring &appxml);
  bool initializessid(const std::wstring &ssid);
  bool execute();

private:
  DWORD pid_{0};
  std::wstring cmd_;
  std::wstring cwd_;
  std::wstring kmessage;
  PSID appcontainersid{nullptr};
  capabilities_t ca;
};
bool WellKnownFromAppmanifest(const std::wstring &file, widcontainer &sids);
} // namespace priv

#endif