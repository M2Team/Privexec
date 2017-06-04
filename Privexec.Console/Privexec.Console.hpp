#ifndef PRIVEXEC_CONSOLE_HPP
#define PRIVEXEC_CONSOLE_HPP

#pragma once

#ifndef _WINDOWS_
#include <Windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <string>

namespace console {
// enum ConsoleColor {
//	Black
//};
namespace fc {
enum Color : WORD {
  Black = 0,
  Blue = FOREGROUND_BLUE,
  Green = FOREGROUND_GREEN,
  Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
  Red = FOREGROUND_RED,
  Purple = FOREGROUND_RED | FOREGROUND_BLUE,
  Yellow = FOREGROUND_RED | FOREGROUND_GREEN,
  White = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
  Gray = FOREGROUND_INTENSITY,
  LightBlue = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  LightGreen = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
  LightCyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  LightRed = FOREGROUND_RED | FOREGROUND_INTENSITY,
  LightMagenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  LightYellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
  LightWhite =
      FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY
};
}
namespace bc {
enum Color : WORD {
  Black = 0,
  White = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
  Blue = BACKGROUND_BLUE,
  Green = BACKGROUND_GREEN,
  Red = BACKGROUND_RED,
  Yellow = BACKGROUND_RED | BACKGROUND_GREEN,
  Magenta = BACKGROUND_RED | BACKGROUND_BLUE,
  Cyan = BACKGROUND_GREEN | BACKGROUND_BLUE,
  LightWhite = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED |
               BACKGROUND_INTENSITY,
  LightBlue = BACKGROUND_BLUE | BACKGROUND_INTENSITY,
  LightGreen = BACKGROUND_GREEN | BACKGROUND_INTENSITY,
  LightRed = BACKGROUND_RED | BACKGROUND_INTENSITY,
  LightYellow = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
  LightMagenta = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
  LightCyan = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
};
}

int WriteInternal(int color, const wchar_t *buf, size_t len);

template <typename T> T Argument(T value) noexcept { return value; }
template <typename T>
T const *Argument(std::basic_string<T> const &value) noexcept {
  return value.c_str();
}
template <typename... Args>
int StringPrint(wchar_t *const buffer, size_t const bufferCount,
                wchar_t const *const format, Args const &... args) noexcept {
  int const result = swprintf(buffer, bufferCount, format, Argument(args)...);
  // ASSERT(-1 != result);
  return result;
}

template <typename... Args>
int Print(int color, const wchar_t *format, Args... args) {
  std::wstring buffer;
  size_t size = StringPrint(nullptr, 0, format, args...);
  buffer.resize(size);
  size = StringPrint(&buffer[0], buffer.size() + 1, format, args...);
  return WriteInternal(color, buffer.data(), size);
}
}
//

#endif