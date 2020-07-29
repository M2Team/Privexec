/////////////
#include <json.hpp>
#include <Shlwapi.h>
#include <PathCch.h>
#include <bela/path.hpp>
#include <bela/codecvt.hpp>
#include <bela/env.hpp>
#include <bela/io.hpp>
#include <file.hpp>
#include <vfsenv.hpp>
#include "app.hpp"

namespace priv {
struct alias_item_t {
  std::string_view name;
  std::string_view desc;
  std::string_view target;
};
bool AppAliasInitializeBuilt(std::wstring_view file) {
  constexpr alias_item_t items[] = {
      {"windbg", "Windows Debugger",
       "\"%ProgramFiles(x86)%\\Windows Kits\\10\\Debuggers\\x64\\windbg.exe\""},       //
      {"edit-hosts", "Edit Hosts", "Notepad %windir%\\System32\\Drivers\\etc\\hosts"}, //
  };
  try {
    nlohmann::json j;
    nlohmann::json alias;
    for (const auto &i : items) {
      nlohmann::json a;
      a["name"] = i.name;
      a["description"] = i.desc;
      a["target"] = i.target;
      alias.push_back(std::move(a));
    }
    j["alias"] = alias;
    auto buf = j.dump(4);
    bela::error_code ec;
    if (!bela::io::WriteTextAtomic(buf, file, ec)) {
      return false;
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}

bool AppAliasInitialize(HWND hbox, priv::alias_t &alias) {
  auto file = PathSearcher::Instance().JoinEtc(L"Privexec.json");
  if (!bela::PathExists(file)) {
    if (!AppAliasInitializeBuilt(file)) {
      return false;
    }
  }
  priv::FD fd;
  if (_wfopen_s(&fd.fd, file.data(), L"rb") != 0) {
    return false;
  }
  try {
    auto json = nlohmann::json::parse(fd.fd, nullptr, true, true);
    auto cmds = json["alias"];
    for (auto &cmd : cmds) {
      auto desc = bela::ToWide(cmd["description"].get<std::string_view>());
      alias.insert_or_assign(desc, bela::ToWide(cmd["target"].get<std::string_view>()));
      ::SendMessageW(hbox, CB_ADDSTRING, 0, (LPARAM)desc.data());
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}
} // namespace priv
