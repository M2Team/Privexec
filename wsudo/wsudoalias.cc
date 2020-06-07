////
#include <json.hpp>
#include "wsudoalias.hpp"
#include <Shlwapi.h>
#include <PathCch.h>
#include <bela/path.hpp>
#include <bela/terminal.hpp>
#include <file.hpp>

std::wstring wsudo::ExecutableFinalPathParent;

wsudo::AliasEngine::~AliasEngine() {
  if (updated) {
    Apply();
  }
}

bool wsudo::AliasEngine::Initialize(bool verbose) {
  auto file = bela::StringCat(ExecutableFinalPathParent, L"\\Privexec.json");
  try {
    priv::FD fd;
    if (_wfopen_s(&fd.fd, file.data(), L"rb") != 0) {
      return false;
    }
    auto json = nlohmann::json::parse(fd.fd);
    auto cmds = json["Alias"];
    for (auto &cmd : cmds) {
      alias.emplace(
          bela::ToWide(cmd["Alias"].get<std::string_view>()),
          AliasTarget(cmd["Target"].get<std::string_view>(), cmd["Desc"].get<std::string_view>()));
    }
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"\x1b[31mAliasEngine::Initialize: %s\x1b[0m\n", e.what());
    return false;
  }
  return true;
}

std::optional<std::wstring> wsudo::AliasEngine::Target(std::wstring_view al) {
  auto it = alias.find(al);
  if (it == alias.end()) {
    return std::nullopt;
  }
  return std::make_optional(it->second.target);
}

bool wsudo::AliasEngine::Apply() {
  auto file = bela::StringCat(ExecutableFinalPathParent, L"\\Privexec.json");
  try {
    nlohmann::json root, av;
    for (const auto &i : alias) {
      nlohmann::json a;
      a["Alias"] = bela::ToNarrow(i.first);
      a["Target"] = bela::ToNarrow(i.second.target);
      a["Desc"] = bela::ToNarrow(i.second.desc);
      av.emplace_back(std::move(a));
    }
    root["Alias"] = av;
    priv::FD fd;
    if (!_wfopen_s(&fd.fd, file.data(), L"w+") != 0) {
      return false;
    }
    auto buf = root.dump(4);
    fwrite(buf.data(), 1, buf.size(), fd);
    updated = false;
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"\x1b[31mAliasEngine::Apply: %s\x1b[0m\n", e.what());
    return false;
  }
  return true;
}

int wsudo::AliasSubcmd(const std::vector<std::wstring_view> &argv, bool verbose) {
  if (argv.size() < 2) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo alias missing argument, current have: %ld\x1b[0m\n",
                  argv.size());
    return 1;
  }
  AliasEngine ae;
  if (!ae.Initialize()) {
    bela::FPrintF(stderr, L"\x1b[31mwsudo Alias Engine Initialize error, "
                          L"Please check it exists\x1b[0m\n");
    return 1;
  }
  if (argv[1] == L"delete") {
    for (size_t i = 2; i < argv.size(); i++) {
      ae.Delete(argv[i].data());
      if (verbose) {
        bela::FPrintF(stderr, L"\x1b[31mwsudo alias delete: %s\x1b[0m\n", argv[i].data());
      }
    }
    return 0;
  }
  if (argv[1] == L"add") {
    if (argv.size() < 5) {
      bela::FPrintF(stderr, L"\x1b[31mwsudo alias add command style is:  Alias "
                            L"Target Des\x1b[0m\n");
      return 1;
    }
    ae.Set(argv[2], argv[3], argv[4]);
    if (verbose) {
      bela::FPrintF(stderr, L"\x1b[33mwsudo alias set: %s to %s (%s)\x1b[0m\n", argv[2], argv[3],
                    argv[4]);
    }
    return 0;
  }
  bela::FPrintF(stderr, L"\x1b[31mwsudo alias unsupported command '%s'\x1b[0m\n", argv[1]);
  return 1;
}
