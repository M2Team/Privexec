/////////////
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <Shlwapi.h>
#include <PathCch.h>
#include <string>
#include <fstream>
#include <json.hpp>
#include "app.hpp"

/// PathAppImageCombineExists
bool PathAppImageCombineExists(std::wstring &path, const wchar_t *file) {
  if (PathFileExistsW(file)) {
    path.assign(file);
    return true;
  }
  path.resize(PATHCCH_MAX_CCH, L'\0');
  auto pszFile = &path[0];
  auto N = GetModuleFileNameW(nullptr, pszFile, PATHCCH_MAX_CCH);
  if (PathCchRemoveExtension(pszFile, (size_t)N + 8) != S_OK) {
    return false;
  }

  if (PathCchAddExtension(pszFile, (size_t)N + 8, L".json") != S_OK) {
    return false;
  }
  auto k = wcslen(pszFile);
  path.resize(k);
  if (PathFileExistsW(path.data())) {
    return true;
  }
  path.clear();
  return false;
}

inline std::wstring utf8towide(std::string_view str) {
  std::wstring wstr;
  auto N =
      MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
  if (N > 0) {
    wstr.resize(N);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], N);
  }
  return wstr;
}

bool AppAlias(HWND hbox, priv::alias_t &alias) {
  std::wstring file;
  if (!PathAppImageCombineExists(file, L"Privexec.json")) {
    return false;
  }
  try {
    std::ifstream fs;
    fs.open(file);
    auto json = nlohmann::json::parse(fs);
    auto cmds = json["Alias"];
    for (auto &cmd : cmds) {
      auto name = utf8towide(cmd["Name"].get<std::string>());
      auto command = utf8towide(cmd["Command"].get<std::string>());
      alias.insert(std::make_pair(name, command));
      ::SendMessage(hbox, CB_ADDSTRING, 0, (LPARAM)name.data());
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}