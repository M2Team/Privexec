/////////////
#include <json.hpp>
#include <Shlwapi.h>
#include <PathCch.h>
#include <bela/path.hpp>
#include <bela/codecvt.hpp>
#include <bela/env.hpp>
#include <file.hpp>
#include "app.hpp"

/// PathAppImageCombineExists
/// PathAppImageCombineExists
bool PathAppImageCombineExists(std::wstring &path, const wchar_t *file) {
  if (PathFileExistsW(file)) {
    path.assign(file);
    return true;
  }
  path.resize(PATHCCH_MAX_CCH, L'\0');
  auto N = GetModuleFileNameW(nullptr, &path[0], PATHCCH_MAX_CCH);
  path.resize(N);
  auto pos = path.find_last_of(L"\\/");
  if (pos != std::wstring::npos) {
    path.resize(pos);
  }
  path.append(L"\\").append(file);
  if (PathFileExistsW(path.data())) {
    return true;
  }
  path.clear();
  return false;
}

namespace priv {
bool AppAliasInitialize(HWND hbox, priv::alias_t &alias) {
  std::wstring file;
  if (!PathAppImageCombineExists(file, L"Privexec.json")) {
    return false;
  }
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
