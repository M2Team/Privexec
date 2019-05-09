//////
#ifndef CLANGBUILDER_CHARCONV_HPP
#define CLANGBUILDER_CHARCONV_HPP

#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <system_error>

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

} // namespace numbers_internal

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

struct to_chars_result {
  wchar_t *ptr;
  std::errc ec;
};

template <class _RawTy>
[[nodiscard]] inline to_chars_result
_Integer_to_chars(wchar_t *_First, wchar_t *const _Last,
                  const _RawTy _Raw_value,
                  const int _Base) noexcept { // strengthened

  using _Unsigned = std::make_unsigned_t<_RawTy>;

  _Unsigned _Value = static_cast<_Unsigned>(_Raw_value);

  if constexpr (std::is_signed_v<_RawTy>) {
    if (_Raw_value < 0) {
      if (_First == _Last) {
        return {_Last, std::errc::value_too_large};
      }

      *_First++ = '-';

      _Value = static_cast<_Unsigned>(0 - _Value);
    }
  }

  constexpr size_t _Buff_size =
      sizeof(_Unsigned) * CHAR_BIT; // enough for base 2
  wchar_t _Buff[_Buff_size];
  wchar_t *const _Buff_end = _Buff + _Buff_size;
  wchar_t *_RNext = _Buff_end;

  static constexpr wchar_t _Digits[] = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
      'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
  static_assert(std::size(_Digits) == 36);

  switch (_Base) {
  case 10: { // Derived from _UIntegral_to_buff()
    // Performance note: Ryu's digit table should be faster here.
    constexpr bool _Use_chunks = sizeof(_Unsigned) > sizeof(size_t);

    if constexpr (_Use_chunks) { // For 64-bit numbers on 32-bit platforms, work
                                 // in chunks to avoid 64-bit divisions.
      while (_Value > 0xFFFF'FFFFU) {
        // Performance note: Ryu's division workaround would be faster here.
        unsigned long _Chunk =
            static_cast<unsigned long>(_Value % 1'000'000'000);
        _Value = static_cast<_Unsigned>(_Value / 1'000'000'000);

        for (int _Idx = 0; _Idx != 9; ++_Idx) {
          *--_RNext = static_cast<wchar_t>('0' + _Chunk % 10);
          _Chunk /= 10;
        }
      }
    }

    using _Truncated =
        std::conditional_t<_Use_chunks, unsigned long, _Unsigned>;

    _Truncated _Trunc = static_cast<_Truncated>(_Value);

    do {
      *--_RNext = static_cast<wchar_t>('0' + _Trunc % 10);
      _Trunc /= 10;
    } while (_Trunc != 0);
    break;
  }

  case 2:
    do {
      *--_RNext = static_cast<wchar_t>('0' + (_Value & 0b1));
      _Value >>= 1;
    } while (_Value != 0);
    break;

  case 4:
    do {
      *--_RNext = static_cast<wchar_t>('0' + (_Value & 0b11));
      _Value >>= 2;
    } while (_Value != 0);
    break;

  case 8:
    do {
      *--_RNext = static_cast<wchar_t>('0' + (_Value & 0b111));
      _Value >>= 3;
    } while (_Value != 0);
    break;

  case 16:
    do {
      *--_RNext = _Digits[_Value & 0b1111];
      _Value >>= 4;
    } while (_Value != 0);
    break;

  case 32:
    do {
      *--_RNext = _Digits[_Value & 0b11111];
      _Value >>= 5;
    } while (_Value != 0);
    break;

  default:
    do {
      *--_RNext = _Digits[_Value % _Base];
      _Value = static_cast<_Unsigned>(_Value / _Base);
    } while (_Value != 0);
    break;
  }

  const ptrdiff_t _Digits_written = _Buff_end - _RNext;

  if (_Last - _First < _Digits_written) {
    return {_Last, std::errc::value_too_large};
  }

  std::memcpy(_First, _RNext, static_cast<size_t>(_Digits_written) * 2);

  return {_First + _Digits_written, std::errc{}};
}

inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const char _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const signed char _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const unsigned char _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const short _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const unsigned short _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const int _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const unsigned int _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const long _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const unsigned long _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const long long _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const unsigned long long _Value,
                                const int _Base = 10) noexcept { // strengthened
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}

// STRUCT from_chars_result
struct from_chars_result {
  const wchar_t *ptr;
  std::errc ec;
};

// FUNCTION from_chars (STRING TO INTEGER)
[[nodiscard]] inline unsigned char
_Digit_from_char(const unsigned char _Ch) noexcept { // strengthened
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

template <class _RawTy>
[[nodiscard]] inline from_chars_result
_Integer_from_chars(const wchar_t *const _First, const wchar_t *const _Last,
                    _RawTy &_Raw_value,
                    const int _Base) noexcept { // strengthened

  bool _Minus_sign = false;

  const wchar_t *_Next = _First;

  if constexpr (std::is_signed_v<_RawTy>) {
    if (_Next != _Last && *_Next == '-') {
      _Minus_sign = true;
      ++_Next;
    }
  }

  using _Unsigned = std::make_unsigned_t<_RawTy>;

  constexpr _Unsigned _Uint_max = static_cast<_Unsigned>(-1);
  constexpr _Unsigned _Int_max = static_cast<_Unsigned>(_Uint_max >> 1);
  constexpr _Unsigned _Abs_int_min = static_cast<_Unsigned>(_Int_max + 1);

  _Unsigned _Risky_val;
  _Unsigned _Max_digit;

  if constexpr (std::is_signed_v<_RawTy>) {
    if (_Minus_sign) {
      _Risky_val = static_cast<_Unsigned>(_Abs_int_min / _Base);
      _Max_digit = static_cast<_Unsigned>(_Abs_int_min % _Base);
    } else {
      _Risky_val = static_cast<_Unsigned>(_Int_max / _Base);
      _Max_digit = static_cast<_Unsigned>(_Int_max % _Base);
    }
  } else {
    _Risky_val = static_cast<_Unsigned>(_Uint_max / _Base);
    _Max_digit = static_cast<_Unsigned>(_Uint_max % _Base);
  }

  _Unsigned _Value = 0;

  bool _Overflowed = false;

  for (; _Next != _Last; ++_Next) {
    auto _Digit = _Digit_from_char(static_cast<const unsigned char>(*_Next));

    if (_Digit >= _Base) {
      break;
    }

    if (_Value < _Risky_val // never overflows
        || (_Value == _Risky_val &&
            _Digit <= _Max_digit)) { // overflows for certain digits
      _Value = static_cast<_Unsigned>(_Value * _Base + _Digit);
    } else {              // _Value > _Risky_val always overflows
      _Overflowed = true; // keep going, _Next still needs to be updated, _Value
                          // is now irrelevant
    }
  }

  if (_Next - _First == static_cast<ptrdiff_t>(_Minus_sign)) {
    return {_First, std::errc::invalid_argument};
  }

  if (_Overflowed) {
    return {_Next, std::errc::result_out_of_range};
  }

  if constexpr (std::is_signed_v<_RawTy>) {
    if (_Minus_sign) {
      _Value = static_cast<_Unsigned>(0 - _Value);
    }
  }

  _Raw_value =
      static_cast<_RawTy>(_Value); // implementation-defined for negative,
                                   // N4713 7.8 [conv.integral]/3

  return {_Next, std::errc{}};
}

inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           char &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           signed char &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           unsigned char &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           short &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           unsigned short &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last, int &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           unsigned int &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           long &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           unsigned long &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           long long &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
inline from_chars_result
from_chars(const wchar_t *const _First, const wchar_t *const _Last,
           unsigned long long &_Value,
           const int _Base = 10) noexcept { // strengthened
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
template <typename Integer>
inline from_chars_result from_chars(std::wstring_view sv, Integer &i,
                                    const int base = 10) noexcept {
  return from_chars(sv.data(), sv.data() + sv.size(), i, base);
}
} // namespace base

#endif
