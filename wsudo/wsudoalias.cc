////
#include <json.hpp>
#include "wsudoalias.hpp"
#include <Shlwapi.h>
#include <PathCch.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <console/console.hpp>

/// PathAppImageCombineExists
bool PathAppImageCombineExists(std::wstring &path, const wchar_t *file) {
  if (PathFileExistsW(file)) {
    path.assign(file);
    return true;
  }
  path.resize(PATHCCH_MAX_CCH, L'\0');
  auto N = GetModuleFileNameW(nullptr, &path[0], PATHCCH_MAX_CCH);
  path.resize(N);
  auto pos = path.rfind(L'\\');
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

inline std::string wide2u8(std::wstring_view wstr) {
  auto l = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(),
                               nullptr, 0, nullptr, nullptr);
  std::string s(l + 1, '\0');
  auto N = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &s[0],
                               (int)s.size(), nullptr, nullptr);
  //
  s.resize(N);
  return s;
}

wsudo::AliasEngine::~AliasEngine() {
  if (updated) {
    Apply();
  }
}

bool wsudo::AliasEngine::Initialize() {
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
      AliasTarget at;
      auto key = utf8towide(cmd["Alias"].get<std::string>());
      at.target = utf8towide(cmd["Target"].get<std::string>());
      at.desc = utf8towide(cmd["Desc"].get<std::string>());
      alias.insert(std::make_pair(key, at));
    }
  } catch (const std::exception &e) {
    fprintf(stderr, "AliasEngine::Initialize: %s\n", e.what());
    return false;
  }
  return true;
}

std::optional<std::wstring> wsudo::AliasEngine::Target(const std::wstring &al) {
  auto it = alias.find(al);
  if (it == alias.end()) {
    return std::nullopt;
  }
  return std::optional<std::wstring>(it->second.target);
}

bool wsudo::AliasEngine::Apply() {
  std::wstring file;
  if (!PathAppImageCombineExists(file, L"Privexec.json")) {
    priv::Print(priv::fc::Red, L"Privexec.json not found: %s\n", file);
    return false;
  }
  try {
    nlohmann::json root, av;
    for (const auto &i : alias) {
      nlohmann::json a;
      a["Alias"] = wide2u8(i.first);
      a["Target"] = wide2u8(i.second.target);
      a["Desc"] = wide2u8(i.second.desc);
      av.push_back(a);
    }
    root["Alias"] = av;
    std::ofstream o(file);
    o << std::setw(4) << root << std::endl;
    updated = false;
  } catch (const std::exception &e) {
    fprintf(stderr, "AliasEngine::Apply: %s\n", e.what());
    return false;
  }
  return true;
}

int wsudo::AliasSubcmd(const std::vector<std::wstring_view> &argv,
                       bool verbose) {
  if (argv.size() < 2) {
    priv::Print(priv::fc::Red,
                L"wsudo alias missing argument, current have: %ld\n",
                argv.size());
    return 1;
  }
  AliasEngine ae;
  if (!ae.Initialize()) {
    priv::Print(
        priv::fc::Red,
        L"wsudo Alias Engine Initialize error, Please check it exists\n");
    return 1;
  }
  if (argv[1] == L"delete") {
    for (size_t i = 2; i < argv.size(); i++) {
      ae.Delete(argv[i].data());
      if (verbose) {
        priv::Print(priv::fc::Yellow, L"wsudo alias delete: %s\n",
                    argv[i].data());
      }
    }
    return 0;
  }
  if (argv[1] == L"add") {
    if (argv.size() < 5) {
      priv::Print(priv::fc::Red,
                  L"wsudo alias add command style is:  Alias Target Des\n");
      return 1;
    }
    ae.Set(argv[2].data(), argv[3].data(), argv[4].data());
    if (verbose) {
      priv::Print(priv::fc::Yellow, L"wsudo alias set: %s to %s (%s)\n",
                  argv[2].data(), argv[3].data(), argv[4].data());
    }
    return 0;
  }
  priv::Print(priv::fc::Red, L"wsudo alias unsupported command '%s'\n",
              argv[1].data());
  return 1;
}