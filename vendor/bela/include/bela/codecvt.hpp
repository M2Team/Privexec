//////////
#ifndef BELA_CODECVT_HPP
#define BELA_CODECVT_HPP
#include <string>
#include <vector>
#include "ucwidth.hpp"

namespace bela {
size_t char32tochar16(char32_t rune, char16_t *dest, size_t dlen);
size_t char32tochar8(char32_t rune, char *dest, size_t dlen);
// UTF-8/UTF-16 codecvt
std::string c16tomb(const char16_t *data, size_t len);
std::wstring mbrtowc(const unsigned char *str, size_t len);
std::u16string mbrtoc16(const unsigned char *str, size_t len);
// Narrow std::wstring_view to UTF-8
inline std::string ToNarrow(std::wstring_view uw) {
  return c16tomb(reinterpret_cast<const char16_t *>(uw.data()), uw.size());
}
// Narrow const wchar_t* to UTF-8
inline std::string ToNarrow(const wchar_t *data, size_t len) {
  return c16tomb(reinterpret_cast<const char16_t *>(data), len);
}
// Narrow std::u16string_view to UTF-8
inline std::string ToNarrow(std::u16string_view uw) {
  return c16tomb(uw.data(), uw.size());
}
// Narrow const char16_t* to UTF-8
inline std::string ToNarrow(const char16_t *data, size_t len) {
  return c16tomb(data, len);
}

inline std::wstring ToWide(const char *str, size_t len) {
  return mbrtowc(reinterpret_cast<const unsigned char *>(str), len);
}

inline std::wstring ToWide(std::string_view sv) {
  return mbrtowc(reinterpret_cast<const unsigned char *>(sv.data()), sv.size());
}

// Escape Unicode Non Basic Multilingual Plane
std::string EscapeNonBMP(std::string_view sv);
std::wstring EscapeNonBMP(std::wstring_view sv);
std::u16string EscapeNonBMP(std::u16string_view sv);

// Calculate UTF-8 string display width
size_t StringWidth(std::string_view str);
// Calculate UTF-16 string display width
size_t StringWidth(std::u16string_view str);
// Calculate UTF-16 string display width (Windows)
inline size_t StringWidth(std::wstring_view str) {
  return StringWidth(std::u16string_view{
      reinterpret_cast<const char16_t *>(str.data()), str.size()});
}
} // namespace bela

#endif
