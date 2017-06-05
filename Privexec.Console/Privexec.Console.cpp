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

std::string wchar2acp(const wchar_t *buf, size_t len) {
  std::string str;
  static auto cp = GetConsoleCP();
  auto N =
      WideCharToMultiByte(cp, 0, buf, (int)len, nullptr, 0, nullptr, nullptr);
  str.resize(N);
  WideCharToMultiByte(cp, 0, buf, (int)len, &str[0], N, nullptr, nullptr);
  return str;
}

struct TerminalsColorTable {
  int index;
  bool blod;
};

bool TerminalsConvertColor(int color, TerminalsColorTable &co) {
  std::unordered_map<int, TerminalsColorTable> tables = {
      {console::fc::Black, {30, false}},
      {console::fc::Blue, {34, false}},
      {console::fc::Green, {32, false}},
      {console::fc::Cyan, {36, false}},
      {console::fc::Red, {31, false}},
      {console::fc::Purple, {35, false}},
      {console::fc::Yellow, {33, false}},
      {console::fc::White, {37, false}},
      {console::fc::LightBlue, {34, true}},
      {console::fc::LightGreen, {32, true}},
      {console::fc::LightCyan, {36, true}},
      {console::fc::LightRed, {31, true}},
      {console::fc::LightMagenta, {35, true}},
      {console::fc::LightYellow, {33, true}},
      {console::fc::LightWhite, {37, true}},
      {console::bc::Black, {40, false}},
      {console::bc::Blue, {44, false}},
      {console::bc::Green, {42, false}},
      {console::bc::Cyan, {46, false}},
      {console::bc::Red, {41, false}},
      {console::bc::Magenta, {45, false}},
      {console::bc::Yellow, {43, false}},
      {console::bc::White, {47, false}},
      {console::bc::LightBlue, {44, true}},
      {console::bc::LightGreen, {42, true}},
      {console::bc::LightCyan, {46, true}},
      {console::bc::LightRed, {41, true}},
      {console::bc::LightMagenta, {45, true}},
      {console::bc::LightYellow, {46, true}},
      {console::bc::LightWhite, {47, true}},
  };
  auto iter = tables.find(color);
  if (iter == tables.end()) {
    return false;
  }
  co = iter->second;
  return true;
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

// https://msdn.microsoft.com/en-us/library/windows/desktop/mt638032(v=vs.85).aspx VT
int WriteVTConsole(int color, const wchar_t *data, size_t len) {
  TerminalsColorTable co;
  auto str = wchar2acp(data, len);
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

/// if is a Conhost
int WriteConhost(int color, const wchar_t *data, size_t len) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  WORD color_ = static_cast<WORD>(color);
  WORD newColor = (oldColor & 0xF0) | color_;
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
  //// VT module
  if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
    isvt = true;
  }
  return true;
}

class WriteProvider {
public:
  WriteProvider() {
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
      impl = WriteConhost;
      return;
    }
    impl = WriteTerminals;
  }
  int WriteImpl(int color, const wchar_t *data, size_t len) {
    return this->impl(color, data, len);
  }

private:
  int (*impl)(int color, const wchar_t *data, size_t len);
};

namespace console {
int WriteInternal(int color, const wchar_t *buf, size_t len) {
  static WriteProvider provider;
  return provider.WriteImpl(color, buf, len);
}
}