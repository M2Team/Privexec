#include "stdafx.h"
#include "Privexec.Console.hpp"
#include <unordered_map>

std::string wchar2utf8(const wchar_t *buf, size_t len) {
  std::string str;
  auto N = WideCharToMultiByte(CP_UTF8, 0, buf, (int)len, nullptr, 0, nullptr,
                               nullptr);
  str.resize(N);
  WideCharToMultiByte(CP_UTF8, 0, buf, (int)len, &str[0], N, nullptr, nullptr);
  return str;
}

struct TerminalsColorTable {
  int index;
  bool blod;
};

namespace vt {
namespace fg {
enum Color {
  Black = 30,
  Red = 31,
  Green = 32,
  Yellow = 33,
  Blue = 34,
  Magenta = 35,
  Cyan = 36,
  Gray = 37,
  Reset = 39
};
}
namespace bg {
enum Color {
  Black = 40,
  Red = 41,
  Green = 42,
  Yellow = 43,
  Blue = 44,
  Magenta = 45,
  Cyan = 46,
  Gray = 47,
  Reset = 49
};
}
}

bool TerminalsConvertColor(int color, TerminalsColorTable &co) {

  std::unordered_map<int, TerminalsColorTable> tables = {
      {console::fc::Black, {vt::fg::Black, false}},
      {console::fc::DarkBlue, {vt::fg::Blue, false}},
      {console::fc::DarkGreen, {vt::fg::Green, false}},
      {console::fc::DarkCyan, {vt::fg::Cyan, false}},
      {console::fc::DarkRed, {vt::fg::Red, false}},
      {console::fc::DarkMagenta, {vt::fg::Magenta, false}},
      {console::fc::DarkYellow, {vt::fg::Yellow, false}},
      {console::fc::DarkGray, {vt::fg::Gray, false}},
      {console::fc::Blue, {vt::fg::Blue, true}},
      {console::fc::Green, {vt::fg::Green, true}},
      {console::fc::Cyan, {vt::fg::Cyan, true}},
      {console::fc::Red, {vt::fg::Red, true}},
      {console::fc::Magenta, {vt::fg::Magenta, true}},
      {console::fc::Yellow, {vt::fg::Yellow, true}},
      {console::fc::White, {vt::fg::Gray, true}},
      {console::bc::Black, {vt::bg::Black, false}},
      {console::bc::Blue, {vt::bg::Blue, false}},
      {console::bc::Green, {vt::bg::Green, false}},
      {console::bc::Cyan, {vt::bg::Cyan, false}},
      {console::bc::Red, {vt::bg::Red, false}},
      {console::bc::Magenta, {vt::bg::Magenta, false}},
      {console::bc::Yellow, {vt::bg::Yellow, false}},
      {console::bc::DarkGray, {vt::bg::Gray, false}},
      {console::bc::LightBlue, {vt::bg::Blue, true}},
      {console::bc::LightGreen, {vt::bg::Green, true}},
      {console::bc::LightCyan, {vt::bg::Cyan, true}},
      {console::bc::LightRed, {vt::fg::Red, true}},
      {console::bc::LightMagenta, {vt::bg::Magenta, true}},
      {console::bc::LightYellow, {vt::bg::Yellow, true}},
      {console::bc::LightWhite, {vt::bg::Gray, true}},
  };
  auto iter = tables.find(color);
  if (iter == tables.end()) {
    return false;
  }
  co = iter->second;
  return true;
}

int WriteConsoleInternal(const wchar_t *buffer, DWORD len) {
  DWORD dwWrite = 0;
  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if (WriteConsoleW(hConsole, buffer, len, &dwWrite, nullptr)) {
    return static_cast<int>(dwWrite);
  }
  return 0;
}

int WriteTerminals(int color, const wchar_t *data, size_t len) {
  TerminalsColorTable co;
  auto str = wchar2utf8(data, len);
  if (!TerminalsConvertColor(color, co)) {
    return static_cast<int>(fwrite(str.data(), 1, str.size(), stdout));
  }
  if (co.blod) {
    fprintf(stdout, "\33[1;%dm", co.index);
  } else {
    fprintf(stdout, "\33[%dm", co.index);
  }
  auto l = fwrite(str.data(), 1, str.size(), stdout);
  fwrite("\33[0m", 1, sizeof("\33[0m") - 1, stdout);
  return static_cast<int>(l);
}

// https://msdn.microsoft.com/en-us/library/windows/desktop/mt638032(v=vs.85).aspx
// VT

int WriteVTConsole(int color, const wchar_t *data, size_t len) {
  TerminalsColorTable co;
  if (!TerminalsConvertColor(color, co)) {
    return WriteConsoleInternal(data, (DWORD)len);
  }
  std::wstring buf(L"\x1b[");
  if (co.blod) {
    buf.append(L"1;").append(std::to_wstring(co.index)).push_back(L'm');
  } else {
    buf.append(std::to_wstring(co.index)).push_back(L'm');
  }
  WriteConsoleInternal(buf.data(), (DWORD)buf.size());
  auto N = WriteConsoleInternal(data, (DWORD)len);
  WriteConsoleInternal(L"\x1b[0m", (sizeof("\x1b[0m") - 1));
  return static_cast<int>(N);
}

/// if is a Conhost
int WriteConhost(int color, const wchar_t *data, size_t len) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  WORD color_ = static_cast<WORD>(color);
  WORD newColor;
  if (color > console::fc::White) {
    newColor = (oldColor & 0x0F) | color_;
  } else {
    newColor = (oldColor & 0xF0) | color_;
  }
  SetConsoleTextAttribute(hConsole, newColor);
  DWORD dwWrite;
  WriteConsoleW(hConsole, data, (DWORD)len, &dwWrite, nullptr);
  SetConsoleTextAttribute(hConsole, oldColor);
  return static_cast<int>(dwWrite);
}

int WriteFiles(int color, const wchar_t *data, size_t len) {
  auto buf = wchar2utf8(data, len);
  auto N = fwrite(buf.data(), 1, buf.size(), stdout); //// write UTF8 to output
  return static_cast<int>(N);
}

bool IsWindowsConhost(HANDLE hConsole, bool &isvt) {
  if (GetFileType(hConsole) != FILE_TYPE_CHAR) {
    return false;
  }
  DWORD mode;
  if (!GetConsoleMode(hConsole, &mode)) {
    return false;
  }
  if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
    isvt = true;
  }
  return true;
}

class ConsoleInternal {
public:
  ConsoleInternal() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
      impl = WriteFiles;
      return;
    }
    if (GetFileType(hConsole) == FILE_TYPE_DISK) {
      impl = WriteFiles;
      return;
    }
    bool isvt = false;
    if (IsWindowsConhost(hConsole, isvt)) {
      if (isvt) {
        impl = WriteVTConsole;
        return;
      }
      impl = WriteConhost;
      return;
    }
    impl = WriteTerminals;
  }
  int WriteRealize(int color, const wchar_t *data, size_t len) {
    return this->impl(color, data, len);
  }

private:
  int (*impl)(int color, const wchar_t *data, size_t len);
};

namespace console {
bool EnableVTMode() {
  // Set output mode to handle virtual terminal sequences
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) {
    return false;
  }

  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode)) {
    return false;
  }
  return true;
}
int WriteInternal(int color, const wchar_t *buf, size_t len) {
  static ConsoleInternal provider;
  return provider.WriteRealize(color, buf, len);
}
}