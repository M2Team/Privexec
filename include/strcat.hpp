///////
#ifndef CLANGBUILDER_STRCAT_HPP
#define CLANGBUILDER_STRCAT_HPP
#pragma once
#include <string_view>
#include <string>
#include <cstdio>
#include <cstring>
#include "charconv.hpp"

namespace base {

namespace strings_internal {
// AlphaNumBuffer allows a way to pass a string to StrCat without having to do
// memory allocation.  It is simply a pair of a fixed-size character array, and
// a size.  Please don't use outside of absl, yet.
template <size_t max_size> struct AlphaNumBuffer {
  std::array<wchar_t, max_size> data;
  size_t size;
};

} // namespace strings_internal

// Enum that specifies the number of significant digits to return in a `Hex` or
// `Dec` conversion and fill character to use. A `kZeroPad2` value, for example,
// would produce hexadecimal strings such as "0a","0f" and a 'kSpacePad5' value
// would produce hexadecimal strings such as "    a","    f".
enum PadSpec : uint8_t {
  kNoPad = 1,
  kZeroPad2,
  kZeroPad3,
  kZeroPad4,
  kZeroPad5,
  kZeroPad6,
  kZeroPad7,
  kZeroPad8,
  kZeroPad9,
  kZeroPad10,
  kZeroPad11,
  kZeroPad12,
  kZeroPad13,
  kZeroPad14,
  kZeroPad15,
  kZeroPad16,
  kZeroPad17,
  kZeroPad18,
  kZeroPad19,
  kZeroPad20,

  kSpacePad2 = kZeroPad2 + 64,
  kSpacePad3,
  kSpacePad4,
  kSpacePad5,
  kSpacePad6,
  kSpacePad7,
  kSpacePad8,
  kSpacePad9,
  kSpacePad10,
  kSpacePad11,
  kSpacePad12,
  kSpacePad13,
  kSpacePad14,
  kSpacePad15,
  kSpacePad16,
  kSpacePad17,
  kSpacePad18,
  kSpacePad19,
  kSpacePad20,
};

// -----------------------------------------------------------------------------
// Hex
// -----------------------------------------------------------------------------
//
// `Hex` stores a set of hexadecimal string conversion parameters for use
// within `AlphaNum` string conversions.
struct Hex {
  uint64_t value;
  uint8_t width;
  wchar_t fill;

  template <typename Int>
  explicit Hex(
      Int v, PadSpec spec = kNoPad,
      typename std::enable_if<sizeof(Int) == 1 &&
                              !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint8_t>(v)) {}
  template <typename Int>
  explicit Hex(
      Int v, PadSpec spec = kNoPad,
      typename std::enable_if<sizeof(Int) == 2 &&
                              !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint16_t>(v)) {}
  template <typename Int>
  explicit Hex(
      Int v, PadSpec spec = kNoPad,
      typename std::enable_if<sizeof(Int) == 4 &&
                              !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint32_t>(v)) {}
  template <typename Int>
  explicit Hex(
      Int v, PadSpec spec = kNoPad,
      typename std::enable_if<sizeof(Int) == 8 &&
                              !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint64_t>(v)) {}
  template <typename Pointee>
  explicit Hex(Pointee *v, PadSpec spec = kNoPad)
      : Hex(spec, reinterpret_cast<uintptr_t>(v)) {}

private:
  Hex(PadSpec spec, uint64_t v)
      : value(v),
        width(spec == kNoPad ? 1
                             : spec >= kSpacePad2 ? spec - kSpacePad2 + 2
                                                  : spec - kZeroPad2 + 2),
        fill(spec >= kSpacePad2 ? L' ' : L'0') {}
};

// -----------------------------------------------------------------------------
// Dec
// -----------------------------------------------------------------------------
//
// `Dec` stores a set of decimal string conversion parameters for use
// within `AlphaNum` string conversions.  Dec is slower than the default
// integer conversion, so use it only if you need padding.
struct Dec {
  uint64_t value;
  uint8_t width;
  wchar_t fill;
  bool neg;

  template <typename Int>
  explicit Dec(Int v, PadSpec spec = kNoPad,
               typename std::enable_if<(sizeof(Int) <= 8)>::type * = nullptr)
      : value(v >= 0 ? static_cast<uint64_t>(v)
                     : uint64_t{0} - static_cast<uint64_t>(v)),
        width(spec == kNoPad ? 1
                             : spec >= kSpacePad2 ? spec - kSpacePad2 + 2
                                                  : spec - kZeroPad2 + 2),
        fill(spec >= kSpacePad2 ? L' ' : L'0'), neg(v < 0) {}
};

constexpr size_t kFastToBufferSize = 32;

class AlphaNum {
public:
  AlphaNum(bool v) : piece_(v ? L"true" : L"false") {} // TRUE FALSE
  AlphaNum(short x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(unsigned short x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(int x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(unsigned int x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(long x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }

  AlphaNum(unsigned long x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(long long x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(unsigned long long x) {
    auto ret = to_chars(digits_, digits_ + 32, x, 10);
    piece_ = std::wstring_view(digits_, ret.ptr - digits_);
  }
  // https://docs.microsoft.com/en-us/cpp/c-runtime-library/format-specification-syntax-printf-and-wprintf-functions?view=vs-2019
  // Integer types such as short, int, long, long long, and their unsigned
  // variants, are specified by using d, i, o, u, x, and X. Floating-point types
  // such as float, double, and long double, are specified by using a, A, e, E,
  // f, F, g, and G. By default, unless they are modified by a size prefix,
  // integer arguments are coerced to int type, and floating-point arguments are
  // coerced to double. On 64-bit systems, an int is a 32-bit value; therefore,
  // 64-bit integers will be truncated when they are formatted for output unless
  // a size prefix of ll or I64 is used. Pointer types that are specified by p
  // use the default pointer size for the platform.
  AlphaNum(float f) {
    auto l = _snwprintf_s(digits_, 32, L"%g", f);
    piece_ = std::wstring_view(digits_, l);
  }
  AlphaNum(double d) {
    auto l = _snwprintf_s(digits_, 32, L"%g", d);
    piece_ = std::wstring_view(digits_, l);
  }
  // NOLINT(runtime/explicit)
  AlphaNum(Hex hex) {
    wchar_t *const end = &digits_[kFastToBufferSize];
    wchar_t *writer = end;
    uint64_t value = hex.value;
    static const wchar_t hexdigits[] = L"0123456789abcdef";
    do {
      *--writer = hexdigits[value & 0xF];
      value >>= 4;
    } while (value != 0);

    wchar_t *beg;
    if (end - writer < hex.width) {
      beg = end - hex.width;
      std::fill_n(beg, writer - beg, hex.fill);
    } else {
      beg = writer;
    }

    piece_ = std::wstring_view(beg, end - beg);
  }
  AlphaNum(Dec dec) {
    wchar_t *const end = &digits_[kFastToBufferSize];
    wchar_t *const minfill = end - dec.width;
    wchar_t *writer = end;
    uint64_t value = dec.value;
    bool neg = dec.neg;
    while (value > 9) {
      *--writer = static_cast<wchar_t>(L'0' + (value % 10));
      value /= 10;
    }
    *--writer = static_cast<wchar_t>(L'0' + value);
    if (neg)
      *--writer = '-';

    ptrdiff_t fillers = writer - minfill;
    if (fillers > 0) {
      // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
      // But...: if the fill character is '0', then it's <+/-><fill><digits>
      bool add_sign_again = false;
      if (neg && dec.fill == L'0') { // If filling with '0',
        ++writer;                    // ignore the sign we just added
        add_sign_again = true;       // and re-add the sign later.
      }
      writer -= fillers;
      std::fill_n(writer, fillers, dec.fill);
      if (add_sign_again)
        *--writer = L'-';
    }

    piece_ = std::wstring_view(writer, end - writer);
  }

  template <size_t size>
  AlphaNum( // NOLINT(runtime/explicit)
      const strings_internal::AlphaNumBuffer<size> &buf)
      : piece_(&buf.data[0], buf.size) {}

  AlphaNum(const wchar_t *cstr) : piece_(cstr) {}
  AlphaNum(std::wstring_view sv) : piece_(sv) {}
  template <typename Allocator>
  AlphaNum( // NOLINT(runtime/explicit)
      const std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator>
          &str)
      : piece_(str) {}
  AlphaNum(wchar_t c) = delete;
  AlphaNum(const AlphaNum &) = delete;
  AlphaNum &operator=(const AlphaNum &) = delete;
  std::wstring_view::size_type size() const { return piece_.size(); }
  const wchar_t *data() const { return piece_.data(); }
  std::wstring_view Piece() const { return piece_; }

private:
  std::wstring_view piece_;
  wchar_t digits_[kFastToBufferSize];
};

namespace strings_internal {

inline void AppendPieces(std::wstring *result,
                         std::initializer_list<std::wstring_view> pieces) {
  auto old_size = result->size();
  size_t total_size = old_size;
  for (const std::wstring_view piece : pieces) {
    total_size += piece.size();
  }
  result->resize(total_size);
  wchar_t *const begin = &*result->begin() + old_size;
  wchar_t *out = begin;
  for (const std::wstring_view piece : pieces) {
    const size_t this_size = piece.size();
    wmemcpy(out, piece.data(), this_size);
    out += this_size;
  }
}

inline std::wstring CatPieces(std::initializer_list<std::wstring_view> pieces) {
  std::wstring result;
  size_t total_size = 0;
  for (const std::wstring_view piece : pieces) {
    total_size += piece.size();
  }
  result.resize(total_size);

  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  for (const std::wstring_view piece : pieces) {
    const size_t this_size = piece.size();
    wmemcpy(out, piece.data(), this_size);
    out += this_size;
  }
  return result;
}

static inline wchar_t *Append(wchar_t *out, const AlphaNum &x) {
  // memcpy is allowed to overwrite arbitrary memory, so doing this after the
  // call would force an extra fetch of x.size().
  wchar_t *after = out + x.size();
  wmemcpy(out, x.data(), x.size());
  return after;
}

} // namespace strings_internal
inline std::wstring StringCat() { return std::wstring(); }

inline std::wstring StringCat(const AlphaNum &a) {
  return std::wstring(a.Piece());
}

inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b) {
  std::wstring result;
  result.resize(a.size() + b.size());
  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  out = strings_internal::Append(out, a);
  out = strings_internal::Append(out, b);
  return result;
}
inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b,
                              const AlphaNum &c) {
  std::wstring result;
  result.resize(a.size() + b.size() + c.size());
  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  out = strings_internal::Append(out, a);
  out = strings_internal::Append(out, b);
  out = strings_internal::Append(out, c);
  return result;
}
inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b,
                              const AlphaNum &c, const AlphaNum &d) {
  std::wstring result;
  result.resize(a.size() + b.size() + c.size() + d.size());
  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  out = strings_internal::Append(out, a);
  out = strings_internal::Append(out, b);
  out = strings_internal::Append(out, c);
  out = strings_internal::Append(out, d);
  return result;
}

// Support 5 or more arguments
template <typename... AV>
inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b,
                              const AlphaNum &c, const AlphaNum &d,
                              const AlphaNum &e, const AV &... args) {
  return strings_internal::CatPieces(
      {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
       static_cast<const AlphaNum &>(args).Piece()...});
}

inline void StrAppend(std::wstring *dest, const AlphaNum &a) {
  dest->append(a.data(), a.size());
}

inline void StrAppend(std::wstring *dest, const AlphaNum &a,
                      const AlphaNum &b) {
  auto old_size = dest->size();
  dest->resize(old_size + a.size() + b.size());
  wchar_t *const begin = &*dest->begin();
  wchar_t *out = begin + old_size;
  out = strings_internal::Append(out, a);
  out = strings_internal::Append(out, b);
}

inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b,
                      const AlphaNum &c) {
  auto old_size = dest->size();
  dest->resize(old_size + a.size() + b.size() + c.size());
  wchar_t *const begin = &*dest->begin();
  wchar_t *out = begin + old_size;
  out = strings_internal::Append(out, a);
  out = strings_internal::Append(out, b);
  out = strings_internal::Append(out, c);
}

inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b,
                      const AlphaNum &c, const AlphaNum &d) {
  auto old_size = dest->size();
  dest->resize(old_size + a.size() + b.size() + c.size() + d.size());
  wchar_t *const begin = &*dest->begin();
  wchar_t *out = begin + old_size;
  out = strings_internal::Append(out, a);
  out = strings_internal::Append(out, b);
  out = strings_internal::Append(out, c);
  out = strings_internal::Append(out, d);
}

template <typename... AV>
inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b,
                      const AlphaNum &c, const AlphaNum &d, const AlphaNum &e,
                      const AV &... args) {
  strings_internal::AppendPieces(
      dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
             static_cast<const AlphaNum &>(args).Piece()...});
}

} // namespace base

#endif
