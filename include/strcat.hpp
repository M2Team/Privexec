///////
#ifndef CLANGBUILDER_STRCAT_HPP
#define CLANGBUILDER_STRCAT_HPP
#pragma once
#include <string_view>
#include <string>
#include <cstdio>
#include <cstring>

namespace base {

namespace numbers_internal {
// Writes a two-character representation of 'i' to 'buf'. 'i' must be in the
// range 0 <= i < 100, and buf must have space for two characters. Example:
//   char buf[2];
//   PutTwoDigits(42, buf);
//   // buf[0] == '4'
//   // buf[1] == '2'
inline void PutTwoDigits(size_t i, wchar_t *buf) {
  static const wchar_t two_ASCII_digits[100][2] = {
      {'0', '0'}, {'0', '1'}, {'0', '2'}, {'0', '3'}, {'0', '4'}, {'0', '5'},
      {'0', '6'}, {'0', '7'}, {'0', '8'}, {'0', '9'}, {'1', '0'}, {'1', '1'},
      {'1', '2'}, {'1', '3'}, {'1', '4'}, {'1', '5'}, {'1', '6'}, {'1', '7'},
      {'1', '8'}, {'1', '9'}, {'2', '0'}, {'2', '1'}, {'2', '2'}, {'2', '3'},
      {'2', '4'}, {'2', '5'}, {'2', '6'}, {'2', '7'}, {'2', '8'}, {'2', '9'},
      {'3', '0'}, {'3', '1'}, {'3', '2'}, {'3', '3'}, {'3', '4'}, {'3', '5'},
      {'3', '6'}, {'3', '7'}, {'3', '8'}, {'3', '9'}, {'4', '0'}, {'4', '1'},
      {'4', '2'}, {'4', '3'}, {'4', '4'}, {'4', '5'}, {'4', '6'}, {'4', '7'},
      {'4', '8'}, {'4', '9'}, {'5', '0'}, {'5', '1'}, {'5', '2'}, {'5', '3'},
      {'5', '4'}, {'5', '5'}, {'5', '6'}, {'5', '7'}, {'5', '8'}, {'5', '9'},
      {'6', '0'}, {'6', '1'}, {'6', '2'}, {'6', '3'}, {'6', '4'}, {'6', '5'},
      {'6', '6'}, {'6', '7'}, {'6', '8'}, {'6', '9'}, {'7', '0'}, {'7', '1'},
      {'7', '2'}, {'7', '3'}, {'7', '4'}, {'7', '5'}, {'7', '6'}, {'7', '7'},
      {'7', '8'}, {'7', '9'}, {'8', '0'}, {'8', '1'}, {'8', '2'}, {'8', '3'},
      {'8', '4'}, {'8', '5'}, {'8', '6'}, {'8', '7'}, {'8', '8'}, {'8', '9'},
      {'9', '0'}, {'9', '1'}, {'9', '2'}, {'9', '3'}, {'9', '4'}, {'9', '5'},
      {'9', '6'}, {'9', '7'}, {'9', '8'}, {'9', '9'}};
  memcpy(buf, two_ASCII_digits[i], 2 * sizeof(wchar_t));
}

inline wchar_t *FastIntToBuffer(uint32_t i, wchar_t *buffer) {
  // Used to optimize printing a decimal number's final digit.
  const wchar_t one_ASCII_final_digits[10][2]{
      {'0', 0}, {'1', 0}, {'2', 0}, {'3', 0}, {'4', 0},
      {'5', 0}, {'6', 0}, {'7', 0}, {'8', 0}, {'9', 0},
  };
  uint32_t digits;
  // The idea of this implementation is to trim the number of divides to as few
  // as possible, and also reducing memory stores and branches, by going in
  // steps of two digits at a time rather than one whenever possible.
  // The huge-number case is first, in the hopes that the compiler will output
  // that case in one branch-free block of code, and only output conditional
  // branches into it from below.
  if (i >= 1000000000) {    // >= 1,000,000,000
    digits = i / 100000000; //      100,000,000
    i -= digits * 100000000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt100_000_000:
    digits = i / 1000000; // 1,000,000
    i -= digits * 1000000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt1_000_000:
    digits = i / 10000; // 10,000
    i -= digits * 10000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt10_000:
    digits = i / 100;
    i -= digits * 100;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt100:
    digits = i;
    PutTwoDigits(digits, buffer);
    buffer += 2;
    *buffer = 0;
    return buffer;
  }

  if (i < 100) {
    digits = i;
    if (i >= 10)
      goto lt100;
    memcpy(buffer, one_ASCII_final_digits[i], 2 * sizeof(wchar_t));
    return buffer + 1;
  }
  if (i < 10000) { //    10,000
    if (i >= 1000)
      goto lt10_000;
    digits = i / 100;
    i -= digits * 100;
    *buffer++ = static_cast<wchar_t>('0' + digits);
    goto lt100;
  }
  if (i < 1000000) { //    1,000,000
    if (i >= 100000)
      goto lt1_000_000;
    digits = i / 10000; //    10,000
    i -= digits * 10000;
    *buffer++ = static_cast<wchar_t>('0' + digits);
    goto lt10_000;
  }
  if (i < 100000000) { //    100,000,000
    if (i >= 10000000)
      goto lt100_000_000;
    digits = i / 1000000; //   1,000,000
    i -= digits * 1000000;
    *buffer++ = static_cast<wchar_t>('0' + digits);
    goto lt1_000_000;
  }
  // we already know that i < 1,000,000,000
  digits = i / 100000000; //   100,000,000
  i -= digits * 100000000;
  *buffer++ = static_cast<wchar_t>('0' + digits);
  goto lt100_000_000;
}

inline wchar_t *FastIntToBuffer(int32_t i, wchar_t *buffer) {
  uint32_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    // We need to do the negation in modular (i.e., "unsigned")
    // arithmetic; MSVC++ apprently warns for plain "-u", so
    // we write the equivalent expression "0 - u" instead.
    u = 0 - u;
  }
  return FastIntToBuffer(u, buffer);
}

inline wchar_t *FastIntToBuffer(uint64_t i, wchar_t *buffer) {
  uint32_t u32 = static_cast<uint32_t>(i);
  if (u32 == i)
    return FastIntToBuffer(u32, buffer);

  // Here we know i has at least 10 decimal digits.
  uint64_t top_1to11 = i / 1000000000;
  u32 = static_cast<uint32_t>(i - top_1to11 * 1000000000);
  uint32_t top_1to11_32 = static_cast<uint32_t>(top_1to11);

  if (top_1to11_32 == top_1to11) {
    buffer = numbers_internal::FastIntToBuffer(top_1to11_32, buffer);
  } else {
    // top_1to11 has more than 32 bits too; print it in two steps.
    uint32_t top_8to9 = static_cast<uint32_t>(top_1to11 / 100);
    uint32_t mid_2 = static_cast<uint32_t>(top_1to11 - top_8to9 * 100);
    buffer = numbers_internal::FastIntToBuffer(top_8to9, buffer);
    PutTwoDigits(mid_2, buffer);
    buffer += 2;
  }

  // We have only 9 digits now, again the maximum uint32_t can handle fully.
  uint32_t digits = u32 / 10000000; // 10,000,000
  u32 -= digits * 10000000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 100000; // 100,000
  u32 -= digits * 100000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 1000; // 1,000
  u32 -= digits * 1000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 10;
  u32 -= digits * 10;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  // Used to optimize printing a decimal number's final digit.
  const wchar_t one_ASCII_final_digits[10][2]{
      {L'0', 0}, {L'1', 0}, {L'2', 0}, {L'3', 0}, {L'4', 0},
      {L'5', 0}, {L'6', 0}, {L'7', 0}, {L'8', 0}, {L'9', 0},
  };
  memcpy(buffer, one_ASCII_final_digits[u32], 2 * sizeof(wchar_t));
  return buffer + 1;
}

inline wchar_t *FastIntToBuffer(int64_t i, wchar_t *buffer) {
  uint64_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    u = 0 - u;
  }
  return FastIntToBuffer(u, buffer);
}
// For enums and integer types that are not an exact match for the types above,
// use templates to call the appropriate one of the four overloads above.
template <typename int_type>
wchar_t *FastIntToBuffer(int_type i, wchar_t *buffer) {
  static_assert(sizeof(i) <= 64 / 8,
                "FastIntToBuffer works only with 64-bit-or-less integers.");
  // TODO(jorg): This signed-ness check is used because it works correctly
  // with enums, and it also serves to check that int_type is not a pointer.
  // If one day something like std::is_signed<enum E> works, switch to it.
  if (static_cast<int_type>(1) - 2 < 0) { // Signed
    if (sizeof(i) > 32 / 8) {             // 33-bit to 64-bit
      return numbers_internal::FastIntToBuffer(static_cast<int64_t>(i), buffer);
    } else { // 32-bit or less
      return numbers_internal::FastIntToBuffer(static_cast<int32_t>(i), buffer);
    }
  } else {                    // Unsigned
    if (sizeof(i) > 32 / 8) { // 33-bit to 64-bit
      return numbers_internal::FastIntToBuffer(static_cast<uint64_t>(i),
                                               buffer);
    } else { // 32-bit or less
      return numbers_internal::FastIntToBuffer(static_cast<uint32_t>(i),
                                               buffer);
    }
  }
}
} // namespace numbers_internal

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
  AlphaNum(short x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned short x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(int x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned int x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(long x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}

  AlphaNum(unsigned long x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(long long x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned long long x)
      : piece_(digits_,
               numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
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
