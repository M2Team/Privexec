//////
#include <cstring>
#include <wchar.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bela/fmt.hpp>
#include <bela/codecvt.hpp>

namespace bela {
namespace format_internal {
constexpr const size_t npos = static_cast<size_t>(-1);
size_t memsearch(const wchar_t *begin, const wchar_t *end, int ch) {
  for (auto it = begin; it != end; it++) {
    if (*it == ch) {
      return it - begin;
    }
  }
  return npos;
}

class buffer {
public:
  buffer(wchar_t *data_, size_t cap_) : data(data_), cap(cap_) {}
  buffer(const buffer &) = delete;
  ~buffer() {
    if (len < cap) {
      data[len] = 0;
    }
  }
  void push_back(wchar_t ch) {
    if (len < cap) {
      data[len++] = ch;
    } else {
      overflow = true;
    }
  }
  buffer &append(std::wstring_view s) {
    if (len + s.size() < cap) {
      memcpy(data + len, s.data(), s.size() * 2);
      len += s.size();
    } else {
      overflow = true;
    }
    return *this;
  }
  buffer &append(const wchar_t *str, size_t dl) {
    if (len + dl < cap) {
      memcpy(data + len, str, dl * 2);
      len += dl;
    } else {
      overflow = true;
    }
    return *this;
  }
  bool IsOverflow() const { return overflow; }
  size_t length() const { return len; }

private:
  wchar_t *data{nullptr};
  size_t len{0};
  size_t cap{0};
  bool overflow{false};
};

constexpr const size_t kFastToBufferSize = 32;
const wchar_t *AlphaNumber(uint64_t value, wchar_t *digits, size_t width,
                           int base, char fill, bool u = false) {
  wchar_t *const end = digits + kFastToBufferSize;
  wchar_t *writer = end;
  constexpr const wchar_t hex[] = L"0123456789abcdef";
  constexpr const wchar_t uhex[] = L"0123456789ABCDEF";
  auto w = (std::min)(width, kFastToBufferSize);
  switch (base) {
  case 8:
    do {
      *--writer = static_cast<wchar_t>('0' + (value & 0x7));
      value >>= 3;
    } while (value != 0);
    break;
  case 16:
    if (u) {
      do {
        *--writer = uhex[value & 0xF];
        value >>= 4;
      } while (value != 0);
    } else {
      do {
        *--writer = hex[value & 0xF];
        value >>= 4;
      } while (value != 0);
    }
    break;
  default:
    do {
      *--writer = hex[value % 10];
      value = value / 10;
    } while (value != 0);
    break;
  }
  wchar_t *beg;
  if ((size_t)(end - writer) < w) {
    beg = end - w;
    std::fill_n(beg, writer - beg, fill);
  } else {
    beg = writer;
  }
  return beg;
}

template <typename T> class Writer {
public:
  Writer(T &t_) : t(t_) {}
  Writer(const Writer &) = delete;
  Writer &operator=(const Writer &) = delete;
  void Pad(size_t pad, bool pz = false) {
    constexpr size_t padlen = 32;
    constexpr const wchar_t padzero[padlen] = {'0'};
    constexpr const wchar_t padspace[padlen] = {' '};
    t.append(pz ? padzero : padspace, (std::min)(pad, padlen));
  }
  void Append(const wchar_t *str, size_t len) { t.append(str, len); }
  void Out(wchar_t ch) { t.push_back(ch); }
  bool Overflow();
  void Floating(double d, uint32_t width, uint32_t frac_width, bool zero) {
    if (std::signbit(d)) {
      Out('-');
      d = -d;
    }
    if (std::isnan(d)) {
      Append(L"nan", 3);
      return;
    }
    if (std::isinf(d)) {
      Append(L"inf", 3);
      return;
    }
    wchar_t digits[kFastToBufferSize];
    const auto dend = digits + kFastToBufferSize;
    auto ui64 = static_cast<int64_t>(d);
    uint64_t frac = 0;
    uint32_t scale = 0;
    if (frac_width > 0) {
      scale = 1;
      for (int n = frac_width; n != 0; n--) {
        scale *= 10;
      }
      frac = (uint64_t)(std::round((d - (double)ui64) * scale));
      if (frac == scale) {
        ui64++;
        frac = 0;
      }
    }
    auto p = AlphaNumber(ui64, digits, width, 10, zero ? '0' : ' ');
    Append(p, dend - p);
    if (frac_width != 0) {
      Out('.');
      p = AlphaNumber(frac, digits, frac_width, 10, zero ? '0' : ' ');
      Append(p, dend - p);
    }
  }
  // Char set pad is space
  void AddUnicode(char32_t ch, size_t width, size_t chw) {
    constexpr size_t kMaxEncodedUTF16Size = 2;
    wchar_t digits[kMaxEncodedUTF16Size + 2];
    if (chw <= 2) {
      if (width > 1) {
        Pad(width - 1, false);
      }
      Out(static_cast<wchar_t>(ch));
      return;
    }
    auto n = char32tochar16(reinterpret_cast<char16_t *>(digits),
                            kFastToBufferSize, ch);
    if (width > n) {
      Pad(width - n, false);
    }
    Append(digits, n);
  }

  void AddUnicodePoint(char32_t ch) {
    wchar_t digits[kFastToBufferSize + 1];
    const auto dend = digits + kFastToBufferSize;
    auto val = static_cast<uint32_t>(ch);
    if (val > 0xFFFF) {
      Append(L"U+", 2);
      auto p = AlphaNumber(val, digits, 8, 16, '0', true);
      Append(p, dend - p);
    } else {
      Append(L"u+", 2);
      auto p = AlphaNumber(val, digits, 4, 16, '0', true);
      Append(p, dend - p);
    }
  }
  void AddBoolean(bool b) {
    if (b) {
      Append(L"true", sizeof("true") - 1);
    } else {
      Append(L"false", sizeof("false") - 1);
    }
  }

private:
  T &t;
};

template <> inline bool Writer<std::wstring>::Overflow() { return false; }
template <> inline bool Writer<buffer>::Overflow() { return t.IsOverflow(); }

using StringWriter = Writer<std::wstring>;
using BufferWriter = Writer<buffer>;

/// because format string is Null-terminated_string
template <typename T>
bool StrFormatInternal(Writer<T> &w, const wchar_t *fmt, const FormatArg *args,
                       size_t max_args) {
  if (args == nullptr || max_args == 0) {
    return false;
  }
  wchar_t digits[kFastToBufferSize];
  const auto dend = digits + kFastToBufferSize;
  auto it = fmt;
  auto end = it + wcslen(fmt);
  size_t ca = 0;
  bool zero;
  uint32_t width;
  uint32_t frac_width;
  while (it < end) {
    ///  Fast search %,
    auto pos = memsearch(it, end, '%');
    if (pos == npos) {
      w.Append(it, end - it);
      return !w.Overflow();
    }
    w.Append(it, pos);
    /// fmt endswith '\0'
    it += pos + 1;
    if (it >= end) {
      break;
    }
    // Parse ---
    zero = (*it == '0');
    width = 0;
    frac_width = 0;
    while (*it >= '0' && *it <= '9') {
      width = width * 10 + (*it++ - '0');
    }
    if (*it == '.') {
      it++;
      while (*it >= '0' && *it <= '9') {
        frac_width = frac_width * 10 + (*it++ - '0');
      }
    }
    switch (*it) {
    case 'b':
      if (ca >= max_args) {
        return false;
      }
      switch (args[ca].at) {
      case ArgType::BOOLEAN:
      case ArgType::CHARACTER:
        w.AddBoolean(args[ca].character.c != 0);
        break;
      case ArgType::INTEGER:
      case ArgType::UINTEGER:
        w.AddBoolean(args[ca].integer.i != 0);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'c':
      if (ca >= max_args) {
        return false;
      }
      switch (args[ca].at) {
      case ArgType::CHARACTER:
        w.AddUnicode(args[ca].character.c, width, args[ca].character.width);
        break;
      case ArgType::UINTEGER:
      case ArgType::INTEGER:
        w.AddUnicode(static_cast<char32_t>(args[ca].integer.i), width,
                     args[ca].integer.width > 2 ? 4 : args[ca].integer.width);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 's':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::STRING) {
        if (width > args[ca].strings.len) {
          w.Pad(width - args[ca].strings.len, zero);
        }
        w.Append(args[ca].strings.data, args[ca].strings.len);
      } else if (args[ca].at == ArgType::USTRING) {
        auto ws = bela::ToWide(args[ca].ustring.data, args[ca].ustring.len);
        if (width > ws.size()) {
          w.Pad(width - static_cast<uint32_t>(ws.size()), zero);
        }
        w.Append(ws.data(), ws.size());
      }
      ca++;
      break;
    case 'd':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        bool sign = false;
        auto val = args[ca].ToInteger(&sign);
        auto p = AlphaNumber(val, digits, width, 10, zero ? '0' : ' ');
        if (sign) {
          w.Out('-');
        }
        w.Append(p, dend - p);
      }
      ca++;
      break;
    case 'o':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        auto val = args[ca].ToInteger();
        auto p = AlphaNumber(val, digits, width, 8, zero ? '0' : ' ');
        w.Append(p, dend - p);
      }
      ca++;
      break;
    case 'x':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        auto val = args[ca].ToInteger();
        auto p = AlphaNumber(val, digits, width, 16, zero ? '0' : ' ');
        w.Append(p, dend - p);
      }
      ca++;
      break;
    case 'X':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        auto val = args[ca].ToInteger();
        auto p = AlphaNumber(val, digits, width, 16, zero ? '0' : ' ', true);
        w.Append(p, dend - p);
      }
      ca++;
      break;
    case 'U':
      if (ca >= max_args) {
        return false;
      }
      switch (args[ca].at) {
      case ArgType::CHARACTER:
        w.AddUnicodePoint(args[ca].character.c);
        break;
      case ArgType::INTEGER:
      case ArgType::UINTEGER:
        w.AddUnicodePoint(static_cast<char32_t>(args[ca].integer.i));
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'f':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::FLOAT) {
        w.Floating(args[ca].floating.d, width, frac_width, zero);
      }
      ca++;
      break;
    case 'a':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::FLOAT) {
        union {
          double d;
          uint64_t i;
        } x;
        x.d = args[ca].floating.d;
        auto p = AlphaNumber(x.i, digits, width, 16, zero ? '0' : ' ', true);
        w.Append(p, dend - p);
      }
      ca++;
      break;
    case 'p':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::POINTER) {
        auto ptr = reinterpret_cast<ptrdiff_t>(args[ca].ptr);
        auto p = AlphaNumber(ptr, digits, width, 16, zero ? '0' : ' ', true);
        w.Append(L"0x", 2);    /// Force append 0x to pointer
        w.Append(p, dend - p); // 0xffff00000;
      }
      ca++;
      break;
    default:
      // % and other
      w.Out(*it);
      break;
    }
    it++;
  }
  return !w.Overflow();
}

std::wstring StrFormatInternal(const wchar_t *fmt, const FormatArg *args,
                               size_t max_args) {
  std::wstring s;
  StringWriter sw(s);
  if (!StrFormatInternal(sw, fmt, args, max_args)) {
    return L"";
  }
  return s;
}

ssize_t StrFormatInternal(wchar_t *buf, size_t N, const wchar_t *fmt,
                          const FormatArg *args, size_t max_args) {
  buffer buffer_(buf, N);
  BufferWriter bw(buffer_);
  if (!StrFormatInternal(bw, fmt, args, max_args)) {
    return -1;
  }
  return static_cast<ssize_t>(buffer_.length());
}
} // namespace format_internal

ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt) {
  format_internal::buffer buffer_(buf, N);
  std::wstring s;
  const wchar_t *src = fmt;
  for (; *src != 0; ++src) {
    buffer_.push_back(*src);
    if (src[0] == '%' && src[1] == '%') {
      ++src;
    }
  }
  return buffer_.IsOverflow() ? -1 : static_cast<ssize_t>(buffer_.length());
}

std::wstring StrFormat(const wchar_t *fmt) {
  std::wstring s;
  const wchar_t *src = fmt;
  for (; *src != 0; ++src) {
    s.push_back(*src);
    if (src[0] == '%' && src[1] == '%') {
      ++src;
    }
  }
  return s;
}
} // namespace bela
