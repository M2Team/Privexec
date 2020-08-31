/////////////
#include <json.hpp>
#include <file.hpp>
#include <bela/path.hpp>
#include <vfsenv.hpp>
#include <filesystem>
#include <charconv>
#include "app.hpp"

namespace priv {

namespace color {
inline bool decode(std::string_view sr, COLORREF &cr) {
  if (sr.empty()) {
    return false;
  }
  if (sr.front() == '#') {
    sr.remove_prefix(1);
  }
  if (sr.size() != 6) {
    return false;
  }
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  auto r1 = std::from_chars(sr.data(), sr.data() + 2, r, 16);
  auto r2 = std::from_chars(sr.data() + 2, sr.data() + 4, g, 16);
  auto r3 = std::from_chars(sr.data() + 4, sr.data() + 6, b, 16);
  if (r1.ec != std::errc{} || r2.ec != std::errc{} || r3.ec != std::errc{}) {
    return false;
  }
  cr = RGB(r, g, b);
  return true;
}

inline std::string encode(COLORREF cr) {
  std::string s;
  s.resize(8);
  uint8_t r = GetRValue(cr);
  uint8_t g = GetGValue(cr);
  uint8_t b = GetBValue(cr);
  _snprintf(s.data(), 8, "#%02x%02x%02x", r, g, b);
  s.resize(7);
  return s;
}
} // namespace color

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
    COLORREF cr;
    if (color::decode(scolor, cr)) {
      as.bk = cr;
      as.textcolor = calcLuminance(cr);
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
    a["Background"] = color::encode(as.bk);
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
