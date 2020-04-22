/////////////
#include <json.hpp>
#include <Shlwapi.h>
#include <PathCch.h>
#include <bela/path.hpp>
#include <bela/codecvt.hpp>
#include <bela/env.hpp>
#include <file.hpp>
#include "app.hpp"

namespace priv {
bool AppAliasInitialize(HWND hbox, priv::alias_t &alias) {
  bela::error_code ec;
  auto p = bela::ExecutableFinalPathParent(ec);
  if (!p) {
    OutputDebugStringW(ec.data());
    return false;
  }
  auto file = bela::StringCat(*p, L"\\Privexec.json");
  try {
    priv::FD fd;
    if (_wfopen_s(&fd.fd, file.data(), L"rb") != 0) {
      return false;
    }
    auto json = nlohmann::json::parse(fd.fd);
    auto cmds = json["Alias"];
    for (auto &cmd : cmds) {
      auto desc = bela::ToWide(cmd["Desc"].get<std::string_view>());
      alias.insert_or_assign(
          desc, bela::ToWide(cmd["Target"].get<std::string_view>()));
      ::SendMessageW(hbox, CB_ADDSTRING, 0, (LPARAM)desc.data());
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}
} // namespace priv
