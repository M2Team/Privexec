/////////////
#include <json.hpp>
#include <Shlwapi.h>
#include <PathCch.h>
#include <file.hpp>
#include "app.hpp"

[[nodiscard]] inline unsigned char
_Digit_from_char(const unsigned char _Ch) noexcept {
  // strengthened
  // convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
  static constexpr unsigned char _Digit_from_byte[] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
      255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
      20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
      35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,
      18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
      33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255};
  static_assert(std::size(_Digit_from_byte) == 256);

  return _Digit_from_byte[_Ch];
}

BYTE HexDecode(const char *p) {
  auto a = _Digit_from_char(p[0]);
  auto b = _Digit_from_char(p[1]);

  if (a == 255 || b == 255) {
    return 0;
  }
  return a * 16 | b;
}

inline bool HexColorDecode(std::string_view sr, COLORREF &cr) {
  if (sr.empty()) {
    return false;
  }
  if (sr.front() == '#') {
    sr.remove_prefix(1);
  }
  if (sr.size() != 6) {
    return false;
  }
  //
  auto r = HexDecode(sr.data());
  auto g = HexDecode(sr.data() + 2);
  auto b = HexDecode(sr.data() + 4);
  cr = RGB(r, g, b);
  return true;
}

inline std::string EncodeColor(COLORREF cr) {
  std::string s;
  auto r = GetRValue(cr);
  auto g = GetGValue(cr);
  auto b = GetBValue(cr);
  char buf[8];
  _snprintf_s(buf, 16, "#%02X%02X%02X", r, g, b);
  s.assign(buf);
  return s;
}

bool PathAppImageCombine(std::wstring &path, const wchar_t *file) {
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
  return true;
}

/// PathAppImageCombineExists
bool PathAppImageCombineExists(std::wstring &path, const wchar_t *file) {
  if (!PathAppImageCombine(path, file)) {
    return false;
  }
  if (PathFileExistsW(path.data())) {
    return true;
  }
  path.clear();
  return false;
}

namespace priv {

bool AppJSON(nlohmann::json &j) {
  std::wstring file;
  if (!PathAppImageCombineExists(file, L"AppExec.json")) {
    return false;
  }
  try {
    FD fd;
    if (_wfopen_s(&fd.fd, file.data(), L"rb") != 0) {
      return false;
    }

    j = nlohmann::json::parse(fd.fd);
  } catch (const std::exception &e) {
    OutputDebugStringA(e.what());
    return false;
  }
  return true;
}

bool AppInitializeSettings(AppSettings &as) {
  try {
    nlohmann::json j;
    if (!AppJSON(j)) {
      return false;
    }
    auto root = j["AppExec"];
    auto scolor = root["Background"].get<std::string>();
    COLORREF cr;
    if (HexColorDecode(scolor, cr)) {
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
  std::wstring file;
  if (!PathAppImageCombine(file, L"AppExec.json")) {
    return false;
  }
  try {
    nlohmann::json j;
    AppJSON(j);
    auto it = j.find("AppExec");
    nlohmann::json a;
    if (it != j.end()) {
      a = *it;
    }
    a["Background"] = EncodeColor(as.bk);
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