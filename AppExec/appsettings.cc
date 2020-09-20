/////////////
#include <json.hpp>
#include <file.hpp>
#include <bela/path.hpp>
#include <bela/color.hpp>
#include <vfsenv.hpp>
#include <filesystem>
#include "app.hpp"

namespace priv {

bool AppInitializeSettings(AppSettings &as) {
  auto file = PathSearcher::Instance().JoinEtc(L"AppExec.json");
  FD fd;
  if (_wfopen_s(&fd.fd, file.data(), L"rb") != 0) {
    return false;
  }
  try {
    auto j = nlohmann::json::parse(fd.fd, nullptr, true, true);
    auto root = j["AppExec"];
    auto scolor = root["Background"].get<std::string_view>();
    bela::color c;
    if (bela::color::Decode(scolor, c)) {
      as.bk = c;
      as.textcolor = calcLuminance(c.r, c.g, c.b);
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}

bool AppApplySettings(const AppSettings &as) {
  auto file = PathSearcher::Instance().JoinEtc(L"AppExec.json");
  std::filesystem::path p(file);
  auto parent = p.parent_path();
  std::error_code e;
  if (!std::filesystem::exists(parent, e)) {
    if (!std::filesystem::create_directories(parent, e)) {
      auto ec = bela::from_std_error_code(e);
      return false;
    }
  }
  nlohmann::json j;
  try {
    FD fd;
    if (_wfopen_s(&fd.fd, file.data(), L"rb") == 0) {
      j = nlohmann::json::parse(fd.fd, nullptr, true, true);
    }
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
  }

  try {
    auto it = j.find("AppExec");
    nlohmann::json a;
    if (it != j.end()) {
      a = *it;
    }
    a["Background"] = bela::color(as.bk).NarrowEncode();
    j["AppExec"] = a;
    auto buf = j.dump(4);
    FD fd;
    if (_wfopen_s(&fd.fd, file.data(), L"w+") != 0) {
      return false;
    }
    fwrite(buf.data(), 1, buf.size(), fd);
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}

} // namespace priv
