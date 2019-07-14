// charconv for wchar_t. base Visual Studio 2019 C++ charconv. Thanks STL
#ifndef BELA_CHARCONV_HPP
#define BELA_CHARCONV_HPP
#pragma once
#include <cfloat>
#include <cstring>
#include <cstdint>
#include <utility>
#include <system_error>
#include <type_traits>
#include <cassert>
#include <algorithm>
#include <climits>
#include <limits>
#ifdef _M_X64
#include <intrin0.h> // for _umul128() and __shiftright128()
#endif               // _M_X64

namespace bela {

template <typename T> void ArrayCopy(void *dst, const T *src, size_t n) {
  memcpy(dst, src, n * sizeof(T));
}

enum class chars_format {
  scientific = 0b001,
  fixed = 0b010,
  hex = 0b100,
  general = fixed | scientific,
};

[[nodiscard]] constexpr chars_format operator&(chars_format L,
                                               chars_format R) noexcept {
  using I = std::underlying_type_t<chars_format>;
  return static_cast<chars_format>(static_cast<I>(L) & static_cast<I>(R));
}

struct to_chars_result {
  wchar_t *ptr;
  std::errc ec;
};

inline constexpr wchar_t _Charconv_digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

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
      *--_RNext = _Charconv_digits[_Value & 0b1111];
      _Value >>= 4;
    } while (_Value != 0);
    break;

  case 32:
    do {
      *--_RNext = _Charconv_digits[_Value & 0b11111];
      _Value >>= 5;
    } while (_Value != 0);
    break;

  default:
    do {
      *--_RNext = _Charconv_digits[_Value % _Base];
      _Value = static_cast<_Unsigned>(_Value / _Base);
    } while (_Value != 0);
    break;
  }

  const ptrdiff_t _Digits_written = _Buff_end - _RNext;

  if (_Last - _First < _Digits_written) {
    return {_Last, std::errc::value_too_large};
  }
  ArrayCopy(_First, _RNext, static_cast<size_t>(_Digits_written));
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

// FUNCTION TEMPLATE _Bit_cast
template <class _To, class _From,
          std::enable_if_t<sizeof(_To) == sizeof(_From) &&
                               std::is_trivially_copyable_v<_To> &&
                               std::is_trivially_copyable_v<_From>,
                           int> = 0>
[[nodiscard]] /*constexpr*/ _To _Bit_cast(const _From &_From_obj) noexcept {
  _To _To_obj; // assumes default-init
  memcpy(std::addressof(_To_obj), std::addressof(_From_obj), sizeof(_To));
  return _To_obj;
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
    // numbers aways < CHAR_MAX

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
//

// Returns __e == 0 ? 1 : ceil(log_2(5^__e)).
[[nodiscard]] inline int32_t __pow5bits(const int32_t __e) {
  // This approximation works up to the point that the multiplication overflows
  // at __e = 3529. If the multiplication were done in 64 bits, it would fail at
  // 5^4004 which is just greater than 2^9297.
  assert(__e >= 0);
  assert(__e <= 3528);
  return static_cast<int32_t>(((static_cast<uint32_t>(__e) * 1217359) >> 19) +
                              1);
}

// Returns floor(log_10(2^__e)).
[[nodiscard]] inline uint32_t __log10Pow2(const int32_t __e) {
  // The first value this approximation fails for is 2^1651 which is just
  // greater than 10^297.
  assert(__e >= 0);
  assert(__e <= 1650);
  return (static_cast<uint32_t>(__e) * 78913) >> 18;
}

// Returns floor(log_10(5^__e)).
[[nodiscard]] inline uint32_t __log10Pow5(const int32_t __e) {
  // The first value this approximation fails for is 5^2621 which is just
  // greater than 10^1832.
  assert(__e >= 0);
  assert(__e <= 2620);
  return (static_cast<uint32_t>(__e) * 732923) >> 20;
}

[[nodiscard]] inline uint32_t __float_to_bits(const float __f) {
  uint32_t __bits = 0;
  memcpy(&__bits, &__f, sizeof(float));
  return __bits;
}

[[nodiscard]] inline uint64_t __double_to_bits(const double __d) {
  uint64_t __bits = 0;
  memcpy(&__bits, &__d, sizeof(double));
  return __bits;
}

// ^^^^^^^^^^ DERIVED FROM common.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM digit_table.h vvvvvvvvvv

// A table of all two-digit numbers. This is used to speed up decimal digit
// generation by copying pairs of digits into the final output.
inline constexpr wchar_t __DIGIT_TABLE[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0',
    '7', '0', '8', '0', '9', '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
    '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', '2', '0', '2', '1', '2',
    '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3',
    '7', '3', '8', '3', '9', '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
    '4', '5', '4', '6', '4', '7', '4', '8', '4', '9', '5', '0', '5', '1', '5',
    '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6',
    '7', '6', '8', '6', '9', '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
    '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', '8', '0', '8', '1', '8',
    '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9',
    '7', '9', '8', '9', '9'};

// ^^^^^^^^^^ DERIVED FROM digit_table.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM f2s.c vvvvvvvvvv

inline constexpr int __FLOAT_MANTISSA_BITS = 23;
inline constexpr int __FLOAT_EXPONENT_BITS = 8;
inline constexpr int __FLOAT_BIAS = 127;

// This table is generated by PrintFloatLookupTable.
inline constexpr int __FLOAT_POW5_INV_BITCOUNT = 59;
inline constexpr uint64_t __FLOAT_POW5_INV_SPLIT[31] = {
    576460752303423489u, 461168601842738791u, 368934881474191033u,
    295147905179352826u, 472236648286964522u, 377789318629571618u,
    302231454903657294u, 483570327845851670u, 386856262276681336u,
    309485009821345069u, 495176015714152110u, 396140812571321688u,
    316912650057057351u, 507060240091291761u, 405648192073033409u,
    324518553658426727u, 519229685853482763u, 415383748682786211u,
    332306998946228969u, 531691198313966350u, 425352958651173080u,
    340282366920938464u, 544451787073501542u, 435561429658801234u,
    348449143727040987u, 557518629963265579u, 446014903970612463u,
    356811923176489971u, 570899077082383953u, 456719261665907162u,
    365375409332725730u};
inline constexpr int __FLOAT_POW5_BITCOUNT = 61;
inline constexpr uint64_t __FLOAT_POW5_SPLIT[47] = {
    1152921504606846976u, 1441151880758558720u, 1801439850948198400u,
    2251799813685248000u, 1407374883553280000u, 1759218604441600000u,
    2199023255552000000u, 1374389534720000000u, 1717986918400000000u,
    2147483648000000000u, 1342177280000000000u, 1677721600000000000u,
    2097152000000000000u, 1310720000000000000u, 1638400000000000000u,
    2048000000000000000u, 1280000000000000000u, 1600000000000000000u,
    2000000000000000000u, 1250000000000000000u, 1562500000000000000u,
    1953125000000000000u, 1220703125000000000u, 1525878906250000000u,
    1907348632812500000u, 1192092895507812500u, 1490116119384765625u,
    1862645149230957031u, 1164153218269348144u, 1455191522836685180u,
    1818989403545856475u, 2273736754432320594u, 1421085471520200371u,
    1776356839400250464u, 2220446049250313080u, 1387778780781445675u,
    1734723475976807094u, 2168404344971008868u, 1355252715606880542u,
    1694065894508600678u, 2117582368135750847u, 1323488980084844279u,
    1654361225106055349u, 2067951531382569187u, 1292469707114105741u,
    1615587133892632177u, 2019483917365790221u};

[[nodiscard]] inline uint32_t __pow5Factor(uint32_t __value) {
  uint32_t __count = 0;
  for (;;) {
    assert(__value != 0);
    const uint32_t __q = __value / 5;
    const uint32_t __r = __value % 5;
    if (__r != 0) {
      break;
    }
    __value = __q;
    ++__count;
  }
  return __count;
}

// Returns true if __value is divisible by 5^__p.
[[nodiscard]] inline bool __multipleOfPowerOf5(const uint32_t __value,
                                               const uint32_t __p) {
  return __pow5Factor(__value) >= __p;
}

// Returns true if __value is divisible by 2^__p.
[[nodiscard]] inline bool __multipleOfPowerOf2(const uint32_t __value,
                                               const uint32_t __p) {
  // return __builtin_ctz(__value) >= __p;
  return (__value & ((1u << __p) - 1)) == 0;
}

[[nodiscard]] inline uint32_t
__mulShift(const uint32_t __m, const uint64_t __factor, const int32_t __shift) {
  assert(__shift > 32);

  // The casts here help MSVC to avoid calls to the __allmul library
  // function.
  const uint32_t __factorLo = static_cast<uint32_t>(__factor);
  const uint32_t __factorHi = static_cast<uint32_t>(__factor >> 32);
  const uint64_t __bits0 = static_cast<uint64_t>(__m) * __factorLo;
  const uint64_t __bits1 = static_cast<uint64_t>(__m) * __factorHi;

#ifndef _WIN64
  // On 32-bit platforms we can avoid a 64-bit shift-right since we only
  // need the upper 32 bits of the result and the shift value is > 32.
  const uint32_t __bits0Hi = static_cast<uint32_t>(__bits0 >> 32);
  uint32_t __bits1Lo = static_cast<uint32_t>(__bits1);
  uint32_t __bits1Hi = static_cast<uint32_t>(__bits1 >> 32);
  __bits1Lo += __bits0Hi;
  __bits1Hi += (__bits1Lo < __bits0Hi);
  const int32_t __s = __shift - 32;
  return (__bits1Hi << (32 - __s)) | (__bits1Lo >> __s);
#else  // ^^^ 32-bit ^^^ / vvv 64-bit vvv
  const uint64_t __sum = (__bits0 >> 32) + __bits1;
  const uint64_t __shiftedSum = __sum >> (__shift - 32);
  assert(__shiftedSum <= UINT32_MAX);
  return static_cast<uint32_t>(__shiftedSum);
#endif // ^^^ 64-bit ^^^
}

[[nodiscard]] inline uint32_t
__mulPow5InvDivPow2(const uint32_t __m, const uint32_t __q, const int32_t __j) {
  return __mulShift(__m, __FLOAT_POW5_INV_SPLIT[__q], __j);
}

[[nodiscard]] inline uint32_t
__mulPow5divPow2(const uint32_t __m, const uint32_t __i, const int32_t __j) {
  return __mulShift(__m, __FLOAT_POW5_SPLIT[__i], __j);
}

[[nodiscard]] inline uint32_t __decimalLength(const uint32_t __v) {
  // Function precondition: __v is not a 10-digit number.
  // (9 digits are sufficient for round-tripping.)
  assert(__v < 1000000000);
  if (__v >= 100000000) {
    return 9;
  }
  if (__v >= 10000000) {
    return 8;
  }
  if (__v >= 1000000) {
    return 7;
  }
  if (__v >= 100000) {
    return 6;
  }
  if (__v >= 10000) {
    return 5;
  }
  if (__v >= 1000) {
    return 4;
  }
  if (__v >= 100) {
    return 3;
  }
  if (__v >= 10) {
    return 2;
  }
  return 1;
}

// A floating decimal representing m * 10^e.
struct __floating_decimal_32 {
  uint32_t __mantissa;
  int32_t __exponent;
};

[[nodiscard]] inline __floating_decimal_32
__f2d(const uint32_t __ieeeMantissa, const uint32_t __ieeeExponent) {
  int32_t __e2;
  uint32_t __m2;
  if (__ieeeExponent == 0) {
    // We subtract 2 so that the bounds computation has 2 additional bits.
    __e2 = 1 - __FLOAT_BIAS - __FLOAT_MANTISSA_BITS - 2;
    __m2 = __ieeeMantissa;
  } else {
    __e2 = static_cast<int32_t>(__ieeeExponent) - __FLOAT_BIAS -
           __FLOAT_MANTISSA_BITS - 2;
    __m2 = (1u << __FLOAT_MANTISSA_BITS) | __ieeeMantissa;
  }
  const bool __even = (__m2 & 1) == 0;
  const bool __acceptBounds = __even;

  // Step 2: Determine the interval of valid decimal representations.
  const uint32_t __mv = 4 * __m2;
  const uint32_t __mp = 4 * __m2 + 2;
  // Implicit bool -> int conversion. True is 1, false is 0.
  const uint32_t __mmShift = __ieeeMantissa != 0 || __ieeeExponent <= 1;
  const uint32_t __mm = 4 * __m2 - 1 - __mmShift;

  // Step 3: Convert to a decimal power base using 64-bit arithmetic.
  uint32_t __vr, __vp, __vm;
  int32_t __e10;
  bool __vmIsTrailingZeros = false;
  bool __vrIsTrailingZeros = false;
  uint8_t __lastRemovedDigit = 0;
  if (__e2 >= 0) {
    const uint32_t __q = __log10Pow2(__e2);
    __e10 = static_cast<int32_t>(__q);
    const int32_t __k =
        __FLOAT_POW5_INV_BITCOUNT + __pow5bits(static_cast<int32_t>(__q)) - 1;
    const int32_t __i = -__e2 + static_cast<int32_t>(__q) + __k;
    __vr = __mulPow5InvDivPow2(__mv, __q, __i);
    __vp = __mulPow5InvDivPow2(__mp, __q, __i);
    __vm = __mulPow5InvDivPow2(__mm, __q, __i);
    if (__q != 0 && (__vp - 1) / 10 <= __vm / 10) {
      // We need to know one removed digit even if we are not going to loop
      // below. We could use
      // __q = X - 1 above, except that would require 33 bits for the result,
      // and we've found that 32-bit arithmetic is faster even on 64-bit
      // machines.
      const int32_t __l = __FLOAT_POW5_INV_BITCOUNT +
                          __pow5bits(static_cast<int32_t>(__q - 1)) - 1;
      __lastRemovedDigit = static_cast<uint8_t>(
          __mulPow5InvDivPow2(__mv, __q - 1,
                              -__e2 + static_cast<int32_t>(__q) - 1 + __l) %
          10);
    }
    if (__q <= 9) {
      // The largest power of 5 that fits in 24 bits is 5^10, but __q <= 9 seems
      // to be safe as well. Only one of __mp, __mv, and __mm can be a multiple
      // of 5, if any.
      if (__mv % 5 == 0) {
        __vrIsTrailingZeros = __multipleOfPowerOf5(__mv, __q);
      } else if (__acceptBounds) {
        __vmIsTrailingZeros = __multipleOfPowerOf5(__mm, __q);
      } else {
        __vp -= __multipleOfPowerOf5(__mp, __q);
      }
    }
  } else {
    const uint32_t __q = __log10Pow5(-__e2);
    __e10 = static_cast<int32_t>(__q) + __e2;
    const int32_t __i = -__e2 - static_cast<int32_t>(__q);
    const int32_t __k = __pow5bits(__i) - __FLOAT_POW5_BITCOUNT;
    int32_t __j = static_cast<int32_t>(__q) - __k;
    __vr = __mulPow5divPow2(__mv, static_cast<uint32_t>(__i), __j);
    __vp = __mulPow5divPow2(__mp, static_cast<uint32_t>(__i), __j);
    __vm = __mulPow5divPow2(__mm, static_cast<uint32_t>(__i), __j);
    if (__q != 0 && (__vp - 1) / 10 <= __vm / 10) {
      __j = static_cast<int32_t>(__q) - 1 -
            (__pow5bits(__i + 1) - __FLOAT_POW5_BITCOUNT);
      __lastRemovedDigit = static_cast<uint8_t>(
          __mulPow5divPow2(__mv, static_cast<uint32_t>(__i + 1), __j) % 10);
    }
    if (__q <= 1) {
      // {__vr,__vp,__vm} is trailing zeros if {__mv,__mp,__mm} has at least __q
      // trailing 0 bits.
      // __mv = 4 * __m2, so it always has at least two trailing 0 bits.
      __vrIsTrailingZeros = true;
      if (__acceptBounds) {
        // __mm = __mv - 1 - __mmShift, so it has 1 trailing 0 bit iff __mmShift
        // == 1.
        __vmIsTrailingZeros = __mmShift == 1;
      } else {
        // __mp = __mv + 2, so it always has at least one trailing 0 bit.
        --__vp;
      }
    } else if (__q < 31) { // TODO(ulfjack): Use a tighter bound here.
      __vrIsTrailingZeros = __multipleOfPowerOf2(__mv, __q - 1);
    }
  }

  // Step 4: Find the shortest decimal representation in the interval of valid
  // representations.
  int32_t __removed = 0;
  uint32_t __output;
  if (__vmIsTrailingZeros || __vrIsTrailingZeros) {
    // General case, which happens rarely (~4.0%).
    while (__vp / 10 > __vm / 10) {
#ifdef __clang__ // TRANSITION, LLVM#23106
      __vmIsTrailingZeros &= __vm - (__vm / 10) * 10 == 0;
#else
      __vmIsTrailingZeros &= __vm % 10 == 0;
#endif
      __vrIsTrailingZeros &= __lastRemovedDigit == 0;
      __lastRemovedDigit = static_cast<uint8_t>(__vr % 10);
      __vr /= 10;
      __vp /= 10;
      __vm /= 10;
      ++__removed;
    }
    if (__vmIsTrailingZeros) {
      while (__vm % 10 == 0) {
        __vrIsTrailingZeros &= __lastRemovedDigit == 0;
        __lastRemovedDigit = static_cast<uint8_t>(__vr % 10);
        __vr /= 10;
        __vp /= 10;
        __vm /= 10;
        ++__removed;
      }
    }
    if (__vrIsTrailingZeros && __lastRemovedDigit == 5 && __vr % 2 == 0) {
      // Round even if the exact number is .....50..0.
      __lastRemovedDigit = 4;
    }
    // We need to take __vr + 1 if __vr is outside bounds or we need to round
    // up.
    __output =
        __vr + ((__vr == __vm && (!__acceptBounds || !__vmIsTrailingZeros)) ||
                __lastRemovedDigit >= 5);
  } else {
    // Specialized for the common case (~96.0%). Percentages below are relative
    // to this. Loop iterations below (approximately): 0: 13.6%, 1: 70.7%,
    // 2: 14.1%, 3: 1.39%, 4: 0.14%, 5+: 0.01%
    while (__vp / 10 > __vm / 10) {
      __lastRemovedDigit = static_cast<uint8_t>(__vr % 10);
      __vr /= 10;
      __vp /= 10;
      __vm /= 10;
      ++__removed;
    }
    // We need to take __vr + 1 if __vr is outside bounds or we need to round
    // up.
    __output = __vr + (__vr == __vm || __lastRemovedDigit >= 5);
  }
  const int32_t __exp = __e10 + __removed;

  __floating_decimal_32 __fd;
  __fd.__exponent = __exp;
  __fd.__mantissa = __output;
  return __fd;
}

[[nodiscard]] inline uint64_t __div1e9(uint64_t __x); // TRANSITION, LLVM#37932

[[nodiscard]] inline to_chars_result
__to_chars(wchar_t *const _First, wchar_t *const _Last,
           const __floating_decimal_32 __v, chars_format _Fmt,
           const uint32_t __ieeeMantissa, const uint32_t __ieeeExponent) {
  // Step 5: Print the decimal representation.
  uint32_t __output = __v.__mantissa;
  int32_t _Ryu_exponent = __v.__exponent;
  const uint32_t __olength = __decimalLength(__output);
  int32_t _Scientific_exponent =
      _Ryu_exponent + static_cast<int32_t>(__olength) - 1;

  if (_Fmt == chars_format{}) {
    int32_t _Lower;
    int32_t _Upper;

    if (__olength == 1) {
      // Value | Fixed   | Scientific
      // 1e-3  | "0.001" | "1e-03"
      // 1e4   | "10000" | "1e+04"
      _Lower = -3;
      _Upper = 4;
    } else {
      // Value   | Fixed       | Scientific
      // 1234e-7 | "0.0001234" | "1.234e-04"
      // 1234e5  | "123400000" | "1.234e+08"
      _Lower = -static_cast<int32_t>(__olength + 3);
      _Upper = 5;
    }

    if (_Lower <= _Ryu_exponent && _Ryu_exponent <= _Upper) {
      _Fmt = chars_format::fixed;
    } else {
      _Fmt = chars_format::scientific;
    }
  } else if (_Fmt == chars_format::general) {
    // C11 7.21.6.1 "The fprintf function"/8:
    // "Let P equal [...] 6 if the precision is omitted [...].
    // Then, if a conversion with style E would have an exponent of X:
    // - if P > X >= -4, the conversion is with style f [...].
    // - otherwise, the conversion is with style e [...]."
    if (-4 <= _Scientific_exponent && _Scientific_exponent < 6) {
      _Fmt = chars_format::fixed;
    } else {
      _Fmt = chars_format::scientific;
    }
  }

  if (_Fmt == chars_format::fixed) {
    // Example: __output == 1729, __olength == 4

    // _Ryu_exponent | Printed  | _Whole_digits | _Total_fixed_length  | Notes
    // --------------|----------|---------------|----------------------|---------------------------------------
    //             2 | 172900   |  6            | _Whole_digits        | Ryu
    //             can't be used for printing 1 | 17290    |  5            |
    //             (sometimes adjusted) | when the trimmed digits are nonzero.
    // --------------|----------|---------------|----------------------|---------------------------------------
    //             0 | 1729     |  4            | _Whole_digits        | Unified
    //             length cases.
    // --------------|----------|---------------|----------------------|---------------------------------------
    //            -1 | 172.9    |  3            | __olength + 1        | This
    //            case can't happen for -2 | 17.29    |  2            | |
    //            __olength == 1, but no additional -3 | 1.729    |  1 | | code
    //            is needed to avoid it.
    // --------------|----------|---------------|----------------------|---------------------------------------
    //            -4 | 0.1729   |  0            | 2 - _Ryu_exponent    |
    //            C11 7.21.6.1 "The fprintf function"/8: -5 | 0.01729  | -1 | |
    //            "If a decimal-point character appears, -6 | 0.001729 | -2 | |
    //            at least one digit appears before it."

    const int32_t _Whole_digits =
        static_cast<int32_t>(__olength) + _Ryu_exponent;

    uint32_t _Total_fixed_length;
    if (_Ryu_exponent >= 0) { // cases "172900" and "1729"
      _Total_fixed_length = static_cast<uint32_t>(_Whole_digits);
      if (__output == 1) {
        // Rounding can affect the number of digits.
        // For example, 1e11f is exactly "99999997952" which is 11 digits
        // instead of 12. We can use a lookup table to detect this and adjust
        // the total length.
        static constexpr uint8_t _Adjustment[39] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1,
            0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1};
        _Total_fixed_length -= _Adjustment[_Ryu_exponent];
        // _Whole_digits doesn't need to be adjusted because these cases won't
        // refer to it later.
      }
    } else if (_Whole_digits > 0) { // case "17.29"
      _Total_fixed_length = __olength + 1;
    } else { // case "0.001729"
      _Total_fixed_length = static_cast<uint32_t>(2 - _Ryu_exponent);
    }

    if (_Last - _First < static_cast<ptrdiff_t>(_Total_fixed_length)) {
      return {_Last, std::errc::value_too_large};
    }

    wchar_t *_Mid;
    if (_Ryu_exponent > 0) { // case "172900"
      bool _Can_use_ryu;

      if (_Ryu_exponent > 10) { // 10^10 is the largest power of 10 that's
                                // exactly representable as a float.
        _Can_use_ryu = false;
      } else {
        // Ryu generated X: __v.__mantissa * 10^_Ryu_exponent
        // __v.__mantissa == 2^_Trailing_zero_bits * (__v.__mantissa >>
        // _Trailing_zero_bits) 10^_Ryu_exponent == 2^_Ryu_exponent *
        // 5^_Ryu_exponent

        // _Trailing_zero_bits is [0, 29] (aside: because 2^29 is the largest
        // power of 2 with 9 decimal digits, which is float's round-trip limit.)
        // _Ryu_exponent is [1, 10].
        // Normalization adds [2, 23] (aside: at least 2 because the
        // pre-normalized mantissa is at least 5). This adds up to [3, 62],
        // which is well below float's maximum binary exponent 127.

        // Therefore, we just need to consider (__v.__mantissa >>
        // _Trailing_zero_bits) * 5^_Ryu_exponent.

        // If that product would exceed 24 bits, then X can't be exactly
        // represented as a float. (That's not a problem for round-tripping,
        // because X is close enough to the original float, but X isn't
        // mathematically equal to the original float.) This requires a
        // high-precision fallback.

        // If the product is 24 bits or smaller, then X can be exactly
        // represented as a float (and we don't need to re-synthesize it; the
        // original float must have been X, because Ryu wouldn't produce the
        // same output for two different floats X and Y). This allows Ryu's
        // output to be used (zero-filled).

        // (2^24 - 1) / 5^0 (for indexing), (2^24 - 1) / 5^1, ..., (2^24 - 1) /
        // 5^10
        static constexpr uint32_t _Max_shifted_mantissa[11] = {
            16777215, 3355443, 671088, 134217, 26843, 5368,
            1073,     214,     42,     8,      1};

        unsigned long _Trailing_zero_bits;
        (void)_BitScanForward(
            &_Trailing_zero_bits,
            __v.__mantissa); // __v.__mantissa is guaranteed nonzero
        const uint32_t _Shifted_mantissa =
            __v.__mantissa >> _Trailing_zero_bits;
        _Can_use_ryu =
            _Shifted_mantissa <= _Max_shifted_mantissa[_Ryu_exponent];
      }

      if (!_Can_use_ryu) {
        // Print the integer _M2 * 2^_E2 exactly.
        const uint32_t _M2 =
            __ieeeMantissa |
            (1u << __FLOAT_MANTISSA_BITS); // restore implicit bit
        const uint32_t _E2 = __ieeeExponent - __FLOAT_BIAS -
                             __FLOAT_MANTISSA_BITS; // bias and normalization

        // For nonzero integers, static_cast<int>(_E2) >= -23. (The minimum
        // value occurs when _M2 * 2^_E2 is 1. In that case, _M2 is the implicit
        // 1 bit followed by 23 zeros, so _E2 is -23 to shift away the zeros.)
        // Negative exponents could be handled by shifting _M2, then setting _E2
        // to 0. However, this isn't necessary. The dense range of exactly
        // representable integers has negative or zero exponents (as positive
        // exponents make the range non-dense). For that dense range, Ryu will
        // always be used: every digit is necessary to uniquely identify the
        // value, so Ryu must print them all. Contrapositive: if Ryu can't be
        // used, the exponent must be positive.
        assert(static_cast<int>(_E2) > 0);
        assert(_E2 <= 104); // because __ieeeExponent <= 254

        // Manually represent _M2 * 2^_E2 as a large integer.
        // _M2 is always 24 bits (due to the implicit bit), while _E2 indicates
        // a shift of at most 104 bits. 24 + 104 equals 128 equals 4 * 32, so we
        // need exactly 4 32-bit elements. We use a little-endian
        // representation, visualized like this:

        // << left shift <<
        // most significant
        // _Data[3] _Data[2] _Data[1] _Data[0]
        //                   least significant
        //                   >> right shift >>

        constexpr uint32_t _Data_size = 4;
        uint32_t _Data[_Data_size]{}; // zero-initialized
        uint32_t _Maxidx = ((24 + _E2 + 31) / 32) -
                           1; // index of most significant nonzero element
        assert(_Maxidx < _Data_size);

        const uint32_t _Bit_shift = _E2 % 32;
        if (_Bit_shift <= 8) { // _M2's 24 bits don't cross an element boundary
          _Data[_Maxidx] = _M2 << _Bit_shift;
        } else { // _M2's 24 bits cross an element boundary
          _Data[_Maxidx - 1] = _M2 << _Bit_shift;
          _Data[_Maxidx] = _M2 >> (32 - _Bit_shift);
        }

        // Print the decimal digits within [_First, _First +
        // _Total_fixed_length), right to left.
        _Mid = _First + _Total_fixed_length;

        if (_Maxidx != 0) { // If the integer is actually large, perform long
                            // division. Otherwise, skip to printing _Data[0].
          for (;;) {
            // Loop invariant: _Maxidx != 0 (i.e. the integer is actually large)

            const uint32_t _Most_significant_elem = _Data[_Maxidx];
            const uint32_t _Initial_remainder =
                _Most_significant_elem % 1000000000;
            const uint32_t _Initial_quotient =
                _Most_significant_elem / 1000000000;
            _Data[_Maxidx] = _Initial_quotient;
            uint64_t _Remainder = _Initial_remainder;

            // Process less significant elements.
            uint32_t _Idx = _Maxidx;
            do {
              --_Idx; // Initially, _Remainder is at most 10^9 - 1.

              // Now, _Remainder is at most (10^9 - 1) * 2^32 + 2^32 - 1,
              // simplified to 10^9 * 2^32 - 1.
              _Remainder = (_Remainder << 32) | _Data[_Idx];

              // floor((10^9 * 2^32 - 1) / 10^9) == 2^32 - 1, so uint32_t
              // _Quotient is lossless.
              const uint32_t _Quotient =
                  static_cast<uint32_t>(__div1e9(_Remainder));

              // _Remainder is at most 10^9 - 1 again.
              _Remainder = _Remainder - 1000000000ull * _Quotient;

              _Data[_Idx] = _Quotient;
            } while (_Idx != 0);

            // Print a 0-filled 9-digit block.
            uint32_t _Output = static_cast<uint32_t>(_Remainder);
            const uint32_t __c = _Output % 10000;
            _Output /= 10000;
            const uint32_t __d = _Output % 10000;
            _Output /= 10000;
            const uint32_t __c0 = (__c % 100) << 1;
            const uint32_t __c1 = (__c / 100) << 1;
            const uint32_t __d0 = (__d % 100) << 1;
            const uint32_t __d1 = (__d / 100) << 1;
            ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c0, 2);
            ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c1, 2);
            ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __d0, 2);
            ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __d1, 2);
            *--_Mid = static_cast<char>('0' + _Output);

            if (_Initial_quotient == 0) { // Is the large integer shrinking?
              --_Maxidx; // log2(10^9) is 29.9, so we can't shrink by more than
                         // one element.
              if (_Maxidx == 0) {
                break; // We've finished long division. Now we need to print
                       // _Data[0].
              }
            }
          }
        }

        assert(_Data[0] != 0);
        for (uint32_t _Idx = 1; _Idx < _Data_size; ++_Idx) {
          assert(_Data[_Idx] == 0);
        }

        // Fall through to print _Data[0]. While it's up to 10 digits,
        // which is more than Ryu generates, the code below can handle this.
        __output = _Data[0];
        _Ryu_exponent = 0; // Imitate case "1729" to avoid further processing.
      } else {             // _Can_use_ryu
        // Print the decimal digits, left-aligned within [_First, _First +
        // _Total_fixed_length).
        _Mid = _First + __olength;
      }
    } else { // cases "1729", "17.29", and "0.001729"
      // Print the decimal digits, right-aligned within [_First, _First +
      // _Total_fixed_length).
      _Mid = _First + _Total_fixed_length;
    }

    while (__output >= 10000) {
#ifdef __clang__ // TRANSITION, LLVM#38217
      const uint32_t __c = __output - 10000 * (__output / 10000);
#else
      const uint32_t __c = __output % 10000;
#endif
      __output /= 10000;
      const uint32_t __c0 = (__c % 100) << 1;
      const uint32_t __c1 = (__c / 100) << 1;
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c0, 2);
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c1, 2);
    }
    if (__output >= 100) {
      const uint32_t __c = (__output % 100) << 1;
      __output /= 100;
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c, 2);
    }
    if (__output >= 10) {
      const uint32_t __c = __output << 1;
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c, 2);
    } else {
      *--_Mid = static_cast<char>('0' + __output);
    }

    if (_Ryu_exponent > 0) { // case "172900" with _Can_use_ryu
      // Performance note: it might be more efficient to do this immediately
      // after setting _Mid.
      wmemset(_First + __olength, L'0', static_cast<size_t>(_Ryu_exponent));
    } else if (_Ryu_exponent ==
               0) { // case "1729", and case "172900" with !_Can_use_ryu
      // Done!
    } else if (_Whole_digits > 0) { // case "17.29"
      // Performance note: moving digits might not be optimal.
      wmemmove(_First, _First + 1, static_cast<size_t>(_Whole_digits));
      _First[_Whole_digits] = '.';
    } else { // case "0.001729"
      // Performance note: a larger memset() followed by overwriting '.' might
      // be more efficient.
      _First[0] = '0';
      _First[1] = '.';
      wmemset(_First + 2, '0', static_cast<size_t>(-_Whole_digits));
    }

    return {_First + _Total_fixed_length, std::errc{}};
  }

  const uint32_t _Total_scientific_length =
      __olength + (__olength > 1) +
      4; // digits + possible decimal point + scientific exponent
  if (_Last - _First < static_cast<ptrdiff_t>(_Total_scientific_length)) {
    return {_Last, std::errc::value_too_large};
  }
  wchar_t *const __result = _First;

  // Print the decimal digits.
  uint32_t __i = 0;
  while (__output >= 10000) {
#ifdef __clang__ // TRANSITION, LLVM#38217
    const uint32_t __c = __output - 10000 * (__output / 10000);
#else
    const uint32_t __c = __output % 10000;
#endif
    __output /= 10000;
    const uint32_t __c0 = (__c % 100) << 1;
    const uint32_t __c1 = (__c / 100) << 1;
    ArrayCopy(__result + __olength - __i - 1, __DIGIT_TABLE + __c0, 2);
    ArrayCopy(__result + __olength - __i - 3, __DIGIT_TABLE + __c1, 2);
    __i += 4;
  }
  if (__output >= 100) {
    const uint32_t __c = (__output % 100) << 1;
    __output /= 100;
    ArrayCopy(__result + __olength - __i - 1, __DIGIT_TABLE + __c, 2);
    __i += 2;
  }
  if (__output >= 10) {
    const uint32_t __c = __output << 1;
    // We can't use memcpy here: the decimal dot goes between these two digits.
    __result[2] = __DIGIT_TABLE[__c + 1];
    __result[0] = __DIGIT_TABLE[__c];
  } else {
    __result[0] = static_cast<char>('0' + __output);
  }

  // Print decimal point if needed.
  uint32_t __index;
  if (__olength > 1) {
    __result[1] = '.';
    __index = __olength + 1;
  } else {
    __index = 1;
  }

  // Print the exponent.
  __result[__index++] = 'e';
  if (_Scientific_exponent < 0) {
    __result[__index++] = '-';
    _Scientific_exponent = -_Scientific_exponent;
  } else {
    __result[__index++] = '+';
  }

  ArrayCopy(__result + __index, __DIGIT_TABLE + 2 * _Scientific_exponent, 2);
  __index += 2;

  return {_First + _Total_scientific_length, std::errc{}};
}

[[nodiscard]] inline to_chars_result __f2s_buffered_n(wchar_t *const _First,
                                                      wchar_t *const _Last,
                                                      const float __f,
                                                      const chars_format _Fmt) {

  // Step 1: Decode the floating-point number, and unify normalized and
  // subnormal cases.
  const uint32_t __bits = __float_to_bits(__f);

  // Case distinction; exit early for the easy cases.
  if (__bits == 0) {
    if (_Fmt == chars_format::scientific) {
      if (_Last - _First < 5) {
        return {_Last, std::errc::value_too_large};
      }

      ArrayCopy(_First, L"0e+00", 5);

      return {_First + 5, std::errc{}};
    }

    // Print "0" for chars_format::fixed, chars_format::general, and
    // chars_format{}.
    if (_First == _Last) {
      return {_Last, std::errc::value_too_large};
    }

    *_First = '0';

    return {_First + 1, std::errc{}};
  }

  // Decode __bits into mantissa and exponent.
  const uint32_t __ieeeMantissa = __bits & ((1u << __FLOAT_MANTISSA_BITS) - 1);
  const uint32_t __ieeeExponent = __bits >> __FLOAT_MANTISSA_BITS;

  const __floating_decimal_32 __v = __f2d(__ieeeMantissa, __ieeeExponent);
  return __to_chars(_First, _Last, __v, _Fmt, __ieeeMantissa, __ieeeExponent);
}

// ^^^^^^^^^^ DERIVED FROM f2s.c ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM d2s.h vvvvvvvvvv

inline constexpr int __DOUBLE_MANTISSA_BITS = 52;
inline constexpr int __DOUBLE_EXPONENT_BITS = 11;
inline constexpr int __DOUBLE_BIAS = 1023;

inline constexpr int __DOUBLE_POW5_INV_BITCOUNT = 122;
inline constexpr int __DOUBLE_POW5_BITCOUNT = 121;

// ^^^^^^^^^^ DERIVED FROM d2s.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM d2s_full_table.h vvvvvvvvvv

// These tables are generated by PrintDoubleLookupTable.
inline constexpr uint64_t __DOUBLE_POW5_INV_SPLIT[292][2] = {
    {1u, 288230376151711744u},
    {3689348814741910324u, 230584300921369395u},
    {2951479051793528259u, 184467440737095516u},
    {17118578500402463900u, 147573952589676412u},
    {12632330341676300947u, 236118324143482260u},
    {10105864273341040758u, 188894659314785808u},
    {15463389048156653253u, 151115727451828646u},
    {17362724847566824558u, 241785163922925834u},
    {17579528692795369969u, 193428131138340667u},
    {6684925324752475329u, 154742504910672534u},
    {18074578149087781173u, 247588007857076054u},
    {18149011334012135262u, 198070406285660843u},
    {3451162622983977240u, 158456325028528675u},
    {5521860196774363583u, 253530120045645880u},
    {4417488157419490867u, 202824096036516704u},
    {7223339340677503017u, 162259276829213363u},
    {7867994130342094503u, 259614842926741381u},
    {2605046489531765280u, 207691874341393105u},
    {2084037191625412224u, 166153499473114484u},
    {10713157136084480204u, 265845599156983174u},
    {12259874523609494487u, 212676479325586539u},
    {13497248433629505913u, 170141183460469231u},
    {14216899864323388813u, 272225893536750770u},
    {11373519891458711051u, 217780714829400616u},
    {5409467098425058518u, 174224571863520493u},
    {4965798542738183305u, 278759314981632789u},
    {7661987648932456967u, 223007451985306231u},
    {2440241304404055250u, 178405961588244985u},
    {3904386087046488400u, 285449538541191976u},
    {17880904128604832013u, 228359630832953580u},
    {14304723302883865611u, 182687704666362864u},
    {15133127457049002812u, 146150163733090291u},
    {16834306301794583852u, 233840261972944466u},
    {9778096226693756759u, 187072209578355573u},
    {15201174610838826053u, 149657767662684458u},
    {2185786488890659746u, 239452428260295134u},
    {5437978005854438120u, 191561942608236107u},
    {15418428848909281466u, 153249554086588885u},
    {6222742084545298729u, 245199286538542217u},
    {16046240111861969953u, 196159429230833773u},
    {1768945645263844993u, 156927543384667019u},
    {10209010661905972635u, 251084069415467230u},
    {8167208529524778108u, 200867255532373784u},
    {10223115638361732810u, 160693804425899027u},
    {1599589762411131202u, 257110087081438444u},
    {4969020624670815285u, 205688069665150755u},
    {3975216499736652228u, 164550455732120604u},
    {13739044029062464211u, 263280729171392966u},
    {7301886408508061046u, 210624583337114373u},
    {13220206756290269483u, 168499666669691498u},
    {17462981995322520850u, 269599466671506397u},
    {6591687966774196033u, 215679573337205118u},
    {12652048002903177473u, 172543658669764094u},
    {9175230360419352987u, 276069853871622551u},
    {3650835473593572067u, 220855883097298041u},
    {17678063637842498946u, 176684706477838432u},
    {13527506561580357021u, 282695530364541492u},
    {3443307619780464970u, 226156424291633194u},
    {6443994910566282300u, 180925139433306555u},
    {5155195928453025840u, 144740111546645244u},
    {15627011115008661990u, 231584178474632390u},
    {12501608892006929592u, 185267342779705912u},
    {2622589484121723027u, 148213874223764730u},
    {4196143174594756843u, 237142198758023568u},
    {10735612169159626121u, 189713759006418854u},
    {12277838550069611220u, 151771007205135083u},
    {15955192865369467629u, 242833611528216133u},
    {1696107848069843133u, 194266889222572907u},
    {12424932722681605476u, 155413511378058325u},
    {1433148282581017146u, 248661618204893321u},
    {15903913885032455010u, 198929294563914656u},
    {9033782293284053685u, 159143435651131725u},
    {14454051669254485895u, 254629497041810760u},
    {11563241335403588716u, 203703597633448608u},
    {16629290697806691620u, 162962878106758886u},
    {781423413297334329u, 260740604970814219u},
    {4314487545379777786u, 208592483976651375u},
    {3451590036303822229u, 166873987181321100u},
    {5522544058086115566u, 266998379490113760u},
    {4418035246468892453u, 213598703592091008u},
    {10913125826658934609u, 170878962873672806u},
    {10082303693170474728u, 273406340597876490u},
    {8065842954536379782u, 218725072478301192u},
    {17520720807854834795u, 174980057982640953u},
    {5897060404116273733u, 279968092772225526u},
    {1028299508551108663u, 223974474217780421u},
    {15580034865808528224u, 179179579374224336u},
    {17549358155809824511u, 286687326998758938u},
    {2971440080422128639u, 229349861599007151u},
    {17134547323305344204u, 183479889279205720u},
    {13707637858644275364u, 146783911423364576u},
    {14553522944347019935u, 234854258277383322u},
    {4264120725993795302u, 187883406621906658u},
    {10789994210278856888u, 150306725297525326u},
    {9885293106962350374u, 240490760476040522u},
    {529536856086059653u, 192392608380832418u},
    {7802327114352668369u, 153914086704665934u},
    {1415676938738538420u, 246262538727465495u},
    {1132541550990830736u, 197010030981972396u},
    {15663428499760305882u, 157608024785577916u},
    {17682787970132668764u, 252172839656924666u},
    {10456881561364224688u, 201738271725539733u},
    {15744202878575200397u, 161390617380431786u},
    {17812026976236499989u, 258224987808690858u},
    {3181575136763469022u, 206579990246952687u},
    {13613306553636506187u, 165263992197562149u},
    {10713244041592678929u, 264422387516099439u},
    {12259944048016053467u, 211537910012879551u},
    {6118606423670932450u, 169230328010303641u},
    {2411072648389671274u, 270768524816485826u},
    {16686253377679378312u, 216614819853188660u},
    {13349002702143502650u, 173291855882550928u},
    {17669055508687693916u, 277266969412081485u},
    {14135244406950155133u, 221813575529665188u},
    {240149081334393137u, 177450860423732151u},
    {11452284974360759988u, 283921376677971441u},
    {5472479164746697667u, 227137101342377153u},
    {11756680961281178780u, 181709681073901722u},
    {2026647139541122378u, 145367744859121378u},
    {18000030682233437097u, 232588391774594204u},
    {18089373360528660001u, 186070713419675363u},
    {3403452244197197031u, 148856570735740291u},
    {16513570034941246220u, 238170513177184465u},
    {13210856027952996976u, 190536410541747572u},
    {3189987192878576934u, 152429128433398058u},
    {1414630693863812771u, 243886605493436893u},
    {8510402184574870864u, 195109284394749514u},
    {10497670562401807014u, 156087427515799611u},
    {9417575270359070576u, 249739884025279378u},
    {14912757845771077107u, 199791907220223502u},
    {4551508647133041040u, 159833525776178802u},
    {10971762650154775986u, 255733641241886083u},
    {16156107749607641435u, 204586912993508866u},
    {9235537384944202825u, 163669530394807093u},
    {11087511001168814197u, 261871248631691349u},
    {12559357615676961681u, 209496998905353079u},
    {13736834907283479668u, 167597599124282463u},
    {18289587036911657145u, 268156158598851941u},
    {10942320814787415393u, 214524926879081553u},
    {16132554281313752961u, 171619941503265242u},
    {11054691591134363444u, 274591906405224388u},
    {16222450902391311402u, 219673525124179510u},
    {12977960721913049122u, 175738820099343608u},
    {17075388340318968271u, 281182112158949773u},
    {2592264228029443648u, 224945689727159819u},
    {5763160197165465241u, 179956551781727855u},
    {9221056315464744386u, 287930482850764568u},
    {14755542681855616155u, 230344386280611654u},
    {15493782960226403247u, 184275509024489323u},
    {1326979923955391628u, 147420407219591459u},
    {9501865507812447252u, 235872651551346334u},
    {11290841220991868125u, 188698121241077067u},
    {1653975347309673853u, 150958496992861654u},
    {10025058185179298811u, 241533595188578646u},
    {4330697733401528726u, 193226876150862917u},
    {14532604630946953951u, 154581500920690333u},
    {1116074521063664381u, 247330401473104534u},
    {4582208431592841828u, 197864321178483627u},
    {14733813189500004432u, 158291456942786901u},
    {16195403473716186445u, 253266331108459042u},
    {5577625149489128510u, 202613064886767234u},
    {8151448934333213131u, 162090451909413787u},
    {16731667109675051333u, 259344723055062059u},
    {17074682502481951390u, 207475778444049647u},
    {6281048372501740465u, 165980622755239718u},
    {6360328581260874421u, 265568996408383549u},
    {8777611679750609860u, 212455197126706839u},
    {10711438158542398211u, 169964157701365471u},
    {9759603424184016492u, 271942652322184754u},
    {11497031554089123517u, 217554121857747803u},
    {16576322872755119460u, 174043297486198242u},
    {11764721337440549842u, 278469275977917188u},
    {16790474699436260520u, 222775420782333750u},
    {13432379759549008416u, 178220336625867000u},
    {3045063541568861850u, 285152538601387201u},
    {17193446092222730773u, 228122030881109760u},
    {13754756873778184618u, 182497624704887808u},
    {18382503128506368341u, 145998099763910246u},
    {3586563302416817083u, 233596959622256395u},
    {2869250641933453667u, 186877567697805116u},
    {17052795772514404226u, 149502054158244092u},
    {12527077977055405469u, 239203286653190548u},
    {17400360011128145022u, 191362629322552438u},
    {2852241564676785048u, 153090103458041951u},
    {15631632947708587046u, 244944165532867121u},
    {8815957543424959314u, 195955332426293697u},
    {18120812478965698421u, 156764265941034957u},
    {14235904707377476180u, 250822825505655932u},
    {4010026136418160298u, 200658260404524746u},
    {17965416168102169531u, 160526608323619796u},
    {2919224165770098987u, 256842573317791675u},
    {2335379332616079190u, 205474058654233340u},
    {1868303466092863352u, 164379246923386672u},
    {6678634360490491686u, 263006795077418675u},
    {5342907488392393349u, 210405436061934940u},
    {4274325990713914679u, 168324348849547952u},
    {10528270399884173809u, 269318958159276723u},
    {15801313949391159694u, 215455166527421378u},
    {1573004715287196786u, 172364133221937103u},
    {17274202803427156150u, 275782613155099364u},
    {17508711057483635243u, 220626090524079491u},
    {10317620031244997871u, 176500872419263593u},
    {12818843235250086271u, 282401395870821749u},
    {13944423402941979340u, 225921116696657399u},
    {14844887537095493795u, 180736893357325919u},
    {15565258844418305359u, 144589514685860735u},
    {6457670077359736959u, 231343223497377177u},
    {16234182506113520537u, 185074578797901741u},
    {9297997190148906106u, 148059663038321393u},
    {11187446689496339446u, 236895460861314229u},
    {12639306166338981880u, 189516368689051383u},
    {17490142562555006151u, 151613094951241106u},
    {2158786396894637579u, 242580951921985771u},
    {16484424376483351356u, 194064761537588616u},
    {9498190686444770762u, 155251809230070893u},
    {11507756283569722895u, 248402894768113429u},
    {12895553841597688639u, 198722315814490743u},
    {17695140702761971558u, 158977852651592594u},
    {17244178680193423523u, 254364564242548151u},
    {10105994129412828495u, 203491651394038521u},
    {4395446488788352473u, 162793321115230817u},
    {10722063196803274280u, 260469313784369307u},
    {1198952927958798777u, 208375451027495446u},
    {15716557601334680315u, 166700360821996356u},
    {17767794532651667857u, 266720577315194170u},
    {14214235626121334286u, 213376461852155336u},
    {7682039686155157106u, 170701169481724269u},
    {1223217053622520399u, 273121871170758831u},
    {15735968901865657612u, 218497496936607064u},
    {16278123936234436413u, 174797997549285651u},
    {219556594781725998u, 279676796078857043u},
    {7554342905309201445u, 223741436863085634u},
    {9732823138989271479u, 178993149490468507u},
    {815121763415193074u, 286389039184749612u},
    {11720143854957885429u, 229111231347799689u},
    {13065463898708218666u, 183288985078239751u},
    {6763022304224664610u, 146631188062591801u},
    {3442138057275642729u, 234609900900146882u},
    {13821756890046245153u, 187687920720117505u},
    {11057405512036996122u, 150150336576094004u},
    {6623802375033462826u, 240240538521750407u},
    {16367088344252501231u, 192192430817400325u},
    {13093670675402000985u, 153753944653920260u},
    {2503129006933649959u, 246006311446272417u},
    {13070549649772650937u, 196805049157017933u},
    {17835137349301941396u, 157444039325614346u},
    {2710778055689733971u, 251910462920982955u},
    {2168622444551787177u, 201528370336786364u},
    {5424246770383340065u, 161222696269429091u},
    {1300097203129523457u, 257956314031086546u},
    {15797473021471260058u, 206365051224869236u},
    {8948629602435097724u, 165092040979895389u},
    {3249760919670425388u, 264147265567832623u},
    {9978506365220160957u, 211317812454266098u},
    {15361502721659949412u, 169054249963412878u},
    {2442311466204457120u, 270486799941460606u},
    {16711244431931206989u, 216389439953168484u},
    {17058344360286875914u, 173111551962534787u},
    {12535955717491360170u, 276978483140055660u},
    {10028764573993088136u, 221582786512044528u},
    {15401709288678291155u, 177266229209635622u},
    {9885339602917624555u, 283625966735416996u},
    {4218922867592189321u, 226900773388333597u},
    {14443184738299482427u, 181520618710666877u},
    {4175850161155765295u, 145216494968533502u},
    {10370709072591134795u, 232346391949653603u},
    {15675264887556728482u, 185877113559722882u},
    {5161514280561562140u, 148701690847778306u},
    {879725219414678777u, 237922705356445290u},
    {703780175531743021u, 190338164285156232u},
    {11631070584651125387u, 152270531428124985u},
    {162968861732249003u, 243632850284999977u},
    {11198421533611530172u, 194906280227999981u},
    {5269388412147313814u, 155925024182399985u},
    {8431021459435702103u, 249480038691839976u},
    {3055468352806651359u, 199584030953471981u},
    {17201769941212962380u, 159667224762777584u},
    {16454785461715008838u, 255467559620444135u},
    {13163828369372007071u, 204374047696355308u},
    {17909760324981426303u, 163499238157084246u},
    {2830174816776909822u, 261598781051334795u},
    {2264139853421527858u, 209279024841067836u},
    {16568707141704863579u, 167423219872854268u},
    {4373838538276319787u, 267877151796566830u},
    {3499070830621055830u, 214301721437253464u},
    {6488605479238754987u, 171441377149802771u},
    {3003071137298187333u, 274306203439684434u},
    {6091805724580460189u, 219444962751747547u},
    {15941491023890099121u, 175555970201398037u},
    {10748990379256517301u, 280889552322236860u},
    {8599192303405213841u, 224711641857789488u},
    {14258051472207991719u, 179769313486231590u}};

inline constexpr uint64_t __DOUBLE_POW5_SPLIT[326][2] = {
    {0u, 72057594037927936u},
    {0u, 90071992547409920u},
    {0u, 112589990684262400u},
    {0u, 140737488355328000u},
    {0u, 87960930222080000u},
    {0u, 109951162777600000u},
    {0u, 137438953472000000u},
    {0u, 85899345920000000u},
    {0u, 107374182400000000u},
    {0u, 134217728000000000u},
    {0u, 83886080000000000u},
    {0u, 104857600000000000u},
    {0u, 131072000000000000u},
    {0u, 81920000000000000u},
    {0u, 102400000000000000u},
    {0u, 128000000000000000u},
    {0u, 80000000000000000u},
    {0u, 100000000000000000u},
    {0u, 125000000000000000u},
    {0u, 78125000000000000u},
    {0u, 97656250000000000u},
    {0u, 122070312500000000u},
    {0u, 76293945312500000u},
    {0u, 95367431640625000u},
    {0u, 119209289550781250u},
    {4611686018427387904u, 74505805969238281u},
    {10376293541461622784u, 93132257461547851u},
    {8358680908399640576u, 116415321826934814u},
    {612489549322387456u, 72759576141834259u},
    {14600669991935148032u, 90949470177292823u},
    {13639151471491547136u, 113686837721616029u},
    {3213881284082270208u, 142108547152020037u},
    {4314518811765112832u, 88817841970012523u},
    {781462496279003136u, 111022302462515654u},
    {10200200157203529728u, 138777878078144567u},
    {13292654125893287936u, 86736173798840354u},
    {7392445620511834112u, 108420217248550443u},
    {4628871007212404736u, 135525271560688054u},
    {16728102434789916672u, 84703294725430033u},
    {7075069988205232128u, 105879118406787542u},
    {18067209522111315968u, 132348898008484427u},
    {8986162942105878528u, 82718061255302767u},
    {6621017659204960256u, 103397576569128459u},
    {3664586055578812416u, 129246970711410574u},
    {16125424340018921472u, 80779356694631608u},
    {1710036351314100224u, 100974195868289511u},
    {15972603494424788992u, 126217744835361888u},
    {9982877184015493120u, 78886090522101180u},
    {12478596480019366400u, 98607613152626475u},
    {10986559581596820096u, 123259516440783094u},
    {2254913720070624656u, 77037197775489434u},
    {12042014186943056628u, 96296497219361792u},
    {15052517733678820785u, 120370621524202240u},
    {9407823583549262990u, 75231638452626400u},
    {11759779479436578738u, 94039548065783000u},
    {14699724349295723422u, 117549435082228750u},
    {4575641699882439235u, 73468396926392969u},
    {10331238143280436948u, 91835496157991211u},
    {8302361660673158281u, 114794370197489014u},
    {1154580038986672043u, 143492962746861268u},
    {9944984561221445835u, 89683101716788292u},
    {12431230701526807293u, 112103877145985365u},
    {1703980321626345405u, 140129846432481707u},
    {17205888765512323542u, 87581154020301066u},
    {12283988920035628619u, 109476442525376333u},
    {1519928094762372062u, 136845553156720417u},
    {12479170105294952299u, 85528470722950260u},
    {15598962631618690374u, 106910588403687825u},
    {5663645234241199255u, 133638235504609782u},
    {17374836326682913246u, 83523897190381113u},
    {7883487353071477846u, 104404871487976392u},
    {9854359191339347308u, 130506089359970490u},
    {10770660513014479971u, 81566305849981556u},
    {13463325641268099964u, 101957882312476945u},
    {2994098996302961243u, 127447352890596182u},
    {15706369927971514489u, 79654595556622613u},
    {5797904354682229399u, 99568244445778267u},
    {2635694424925398845u, 124460305557222834u},
    {6258995034005762182u, 77787690973264271u},
    {3212057774079814824u, 97234613716580339u},
    {17850130272881932242u, 121543267145725423u},
    {18073860448192289507u, 75964541966078389u},
    {8757267504958198172u, 94955677457597987u},
    {6334898362770359811u, 118694596821997484u},
    {13182683513586250689u, 74184123013748427u},
    {11866668373555425458u, 92730153767185534u},
    {5609963430089506015u, 115912692208981918u},
    {17341285199088104971u, 72445432630613698u},
    {12453234462005355406u, 90556790788267123u},
    {10954857059079306353u, 113195988485333904u},
    {13693571323849132942u, 141494985606667380u},
    {17781854114260483896u, 88434366004167112u},
    {3780573569116053255u, 110542957505208891u},
    {114030942967678664u, 138178696881511114u},
    {4682955357782187069u, 86361685550944446u},
    {15077066234082509644u, 107952106938680557u},
    {5011274737320973344u, 134940133673350697u},
    {14661261756894078100u, 84337583545844185u},
    {4491519140835433913u, 105421979432305232u},
    {5614398926044292391u, 131777474290381540u},
    {12732371365632458552u, 82360921431488462u},
    {6692092170185797382u, 102951151789360578u},
    {17588487249587022536u, 128688939736700722u},
    {15604490549419276989u, 80430587335437951u},
    {14893927168346708332u, 100538234169297439u},
    {14005722942005997511u, 125672792711621799u},
    {15671105866394830300u, 78545495444763624u},
    {1142138259283986260u, 98181869305954531u},
    {15262730879387146537u, 122727336632443163u},
    {7233363790403272633u, 76704585395276977u},
    {13653390756431478696u, 95880731744096221u},
    {3231680390257184658u, 119850914680120277u},
    {4325643253124434363u, 74906821675075173u},
    {10018740084832930858u, 93633527093843966u},
    {3300053069186387764u, 117041908867304958u},
    {15897591223523656064u, 73151193042065598u},
    {10648616992549794273u, 91438991302581998u},
    {4087399203832467033u, 114298739128227498u},
    {14332621041645359599u, 142873423910284372u},
    {18181260187883125557u, 89295889943927732u},
    {4279831161144355331u, 111619862429909666u},
    {14573160988285219972u, 139524828037387082u},
    {13719911636105650386u, 87203017523366926u},
    {7926517508277287175u, 109003771904208658u},
    {684774848491833161u, 136254714880260823u},
    {7345513307948477581u, 85159196800163014u},
    {18405263671790372785u, 106448996000203767u},
    {18394893571310578077u, 133061245000254709u},
    {13802651491282805250u, 83163278125159193u},
    {3418256308821342851u, 103954097656448992u},
    {4272820386026678563u, 129942622070561240u},
    {2670512741266674102u, 81214138794100775u},
    {17173198981865506339u, 101517673492625968u},
    {3019754653622331308u, 126897091865782461u},
    {4193189667727651020u, 79310682416114038u},
    {14464859121514339583u, 99138353020142547u},
    {13469387883465536574u, 123922941275178184u},
    {8418367427165960359u, 77451838296986365u},
    {15134645302384838353u, 96814797871232956u},
    {471562554271496325u, 121018497339041196u},
    {9518098633274461011u, 75636560836900747u},
    {7285937273165688360u, 94545701046125934u},
    {18330793628311886258u, 118182126307657417u},
    {4539216990053847055u, 73863828942285886u},
    {14897393274422084627u, 92329786177857357u},
    {4786683537745442072u, 115412232722321697u},
    {14520892257159371055u, 72132645451451060u},
    {18151115321449213818u, 90165806814313825u},
    {8853836096529353561u, 112707258517892282u},
    {1843923083806916143u, 140884073147365353u},
    {12681666973447792349u, 88052545717103345u},
    {2017025661527576725u, 110065682146379182u},
    {11744654113764246714u, 137582102682973977u},
    {422879793461572340u, 85988814176858736u},
    {528599741826965425u, 107486017721073420u},
    {660749677283706782u, 134357522151341775u},
    {7330497575943398595u, 83973451344588609u},
    {13774807988356636147u, 104966814180735761u},
    {3383451930163631472u, 131208517725919702u},
    {15949715511634433382u, 82005323578699813u},
    {6102086334260878016u, 102506654473374767u},
    {3015921899398709616u, 128133318091718459u},
    {18025852251620051174u, 80083323807324036u},
    {4085571240815512351u, 100104154759155046u},
    {14330336087874166247u, 125130193448943807u},
    {15873989082562435760u, 78206370905589879u},
    {15230800334775656796u, 97757963631987349u},
    {5203442363187407284u, 122197454539984187u},
    {946308467778435600u, 76373409087490117u},
    {5794571603150432404u, 95466761359362646u},
    {16466586540792816313u, 119333451699203307u},
    {7985773578781816244u, 74583407312002067u},
    {5370530955049882401u, 93229259140002584u},
    {6713163693812353001u, 116536573925003230u},
    {18030785363914884337u, 72835358703127018u},
    {13315109668038829614u, 91044198378908773u},
    {2808829029766373305u, 113805247973635967u},
    {17346094342490130344u, 142256559967044958u},
    {6229622945628943561u, 88910349979403099u},
    {3175342663608791547u, 111137937474253874u},
    {13192550366365765242u, 138922421842817342u},
    {3633657960551215372u, 86826513651760839u},
    {18377130505971182927u, 108533142064701048u},
    {4524669058754427043u, 135666427580876311u},
    {9745447189362598758u, 84791517238047694u},
    {2958436949848472639u, 105989396547559618u},
    {12921418224165366607u, 132486745684449522u},
    {12687572408530742033u, 82804216052780951u},
    {11247779492236039638u, 103505270065976189u},
    {224666310012885835u, 129381587582470237u},
    {2446259452971747599u, 80863492239043898u},
    {12281196353069460307u, 101079365298804872u},
    {15351495441336825384u, 126349206623506090u},
    {14206370669262903769u, 78968254139691306u},
    {8534591299723853903u, 98710317674614133u},
    {15279925143082205283u, 123387897093267666u},
    {14161639232853766206u, 77117435683292291u},
    {13090363022639819853u, 96396794604115364u},
    {16362953778299774816u, 120495993255144205u},
    {12532689120651053212u, 75309995784465128u},
    {15665861400813816515u, 94137494730581410u},
    {10358954714162494836u, 117671868413226763u},
    {4168503687137865320u, 73544917758266727u},
    {598943590494943747u, 91931147197833409u},
    {5360365506546067587u, 114913933997291761u},
    {11312142901609972388u, 143642417496614701u},
    {9375932322719926695u, 89776510935384188u},
    {11719915403399908368u, 112220638669230235u},
    {10038208235822497557u, 140275798336537794u},
    {10885566165816448877u, 87672373960336121u},
    {18218643725697949000u, 109590467450420151u},
    {18161618638695048346u, 136988084313025189u},
    {13656854658398099168u, 85617552695640743u},
    {12459382304570236056u, 107021940869550929u},
    {1739169825430631358u, 133777426086938662u},
    {14922039196176308311u, 83610891304336663u},
    {14040862976792997485u, 104513614130420829u},
    {3716020665709083144u, 130642017663026037u},
    {4628355925281870917u, 81651261039391273u},
    {10397130925029726550u, 102064076299239091u},
    {8384727637859770284u, 127580095374048864u},
    {5240454773662356427u, 79737559608780540u},
    {6550568467077945534u, 99671949510975675u},
    {3576524565420044014u, 124589936888719594u},
    {6847013871814915412u, 77868710555449746u},
    {17782139376623420074u, 97335888194312182u},
    {13004302183924499284u, 121669860242890228u},
    {17351060901807587860u, 76043662651806392u},
    {3242082053549933210u, 95054578314757991u},
    {17887660622219580224u, 118818222893447488u},
    {11179787888887237640u, 74261389308404680u},
    {13974734861109047050u, 92826736635505850u},
    {8245046539531533005u, 116033420794382313u},
    {16682369133275677888u, 72520887996488945u},
    {7017903361312433648u, 90651109995611182u},
    {17995751238495317868u, 113313887494513977u},
    {8659630992836983623u, 141642359368142472u},
    {5412269370523114764u, 88526474605089045u},
    {11377022731581281359u, 110658093256361306u},
    {4997906377621825891u, 138322616570451633u},
    {14652906532082110942u, 86451635356532270u},
    {9092761128247862869u, 108064544195665338u},
    {2142579373455052779u, 135080680244581673u},
    {12868327154477877747u, 84425425152863545u},
    {2250350887815183471u, 105531781441079432u},
    {2812938609768979339u, 131914726801349290u},
    {6369772649532999991u, 82446704250843306u},
    {17185587848771025797u, 103058380313554132u},
    {3035240737254230630u, 128822975391942666u},
    {6508711479211282048u, 80514359619964166u},
    {17359261385868878368u, 100642949524955207u},
    {17087390713908710056u, 125803686906194009u},
    {3762090168551861929u, 78627304316371256u},
    {4702612710689827411u, 98284130395464070u},
    {15101637925217060072u, 122855162994330087u},
    {16356052730901744401u, 76784476871456304u},
    {1998321839917628885u, 95980596089320381u},
    {7109588318324424010u, 119975745111650476u},
    {13666864735807540814u, 74984840694781547u},
    {12471894901332038114u, 93731050868476934u},
    {6366496589810271835u, 117163813585596168u},
    {3979060368631419896u, 73227383490997605u},
    {9585511479216662775u, 91534229363747006u},
    {2758517312166052660u, 114417786704683758u},
    {12671518677062341634u, 143022233380854697u},
    {1002170145522881665u, 89388895863034186u},
    {10476084718758377889u, 111736119828792732u},
    {13095105898447972362u, 139670149785990915u},
    {5878598177316288774u, 87293843616244322u},
    {16571619758500136775u, 109117304520305402u},
    {11491152661270395161u, 136396630650381753u},
    {264441385652915120u, 85247894156488596u},
    {330551732066143900u, 106559867695610745u},
    {5024875683510067779u, 133199834619513431u},
    {10058076329834874218u, 83249896637195894u},
    {3349223375438816964u, 104062370796494868u},
    {4186529219298521205u, 130077963495618585u},
    {14145795808130045513u, 81298727184761615u},
    {13070558741735168987u, 101623408980952019u},
    {11726512408741573330u, 127029261226190024u},
    {7329070255463483331u, 79393288266368765u},
    {13773023837756742068u, 99241610332960956u},
    {17216279797195927585u, 124052012916201195u},
    {8454331864033760789u, 77532508072625747u},
    {5956228811614813082u, 96915635090782184u},
    {7445286014518516353u, 121144543863477730u},
    {9264989777501460624u, 75715339914673581u},
    {16192923240304213684u, 94644174893341976u},
    {1794409976670715490u, 118305218616677471u},
    {8039035263060279037u, 73940761635423419u},
    {5437108060397960892u, 92425952044279274u},
    {16019757112352226923u, 115532440055349092u},
    {788976158365366019u, 72207775034593183u},
    {14821278253238871236u, 90259718793241478u},
    {9303225779693813237u, 112824648491551848u},
    {11629032224617266546u, 141030810614439810u},
    {11879831158813179495u, 88144256634024881u},
    {1014730893234310657u, 110180320792531102u},
    {10491785653397664129u, 137725400990663877u},
    {8863209042587234033u, 86078375619164923u},
    {6467325284806654637u, 107597969523956154u},
    {17307528642863094104u, 134497461904945192u},
    {10817205401789433815u, 84060913690590745u},
    {18133192770664180173u, 105076142113238431u},
    {18054804944902837312u, 131345177641548039u},
    {18201782118205355176u, 82090736025967524u},
    {4305483574047142354u, 102613420032459406u},
    {14605226504413703751u, 128266775040574257u},
    {2210737537617482988u, 80166734400358911u},
    {16598479977304017447u, 100208418000448638u},
    {11524727934775246001u, 125260522500560798u},
    {2591268940807140847u, 78287826562850499u},
    {17074144231291089770u, 97859783203563123u},
    {16730994270686474309u, 122324729004453904u},
    {10456871419179046443u, 76452955627783690u},
    {3847717237119032246u, 95566194534729613u},
    {9421332564826178211u, 119457743168412016u},
    {5888332853016361382u, 74661089480257510u},
    {16583788103125227536u, 93326361850321887u},
    {16118049110479146516u, 116657952312902359u},
    {16991309721690548428u, 72911220195563974u},
    {12015765115258409727u, 91139025244454968u},
    {15019706394073012159u, 113923781555568710u},
    {9551260955736489391u, 142404726944460888u},
    {5969538097335305869u, 89002954340288055u},
    {2850236603241744433u, 111253692925360069u}};

// ^^^^^^^^^^ DERIVED FROM d2s_full_table.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM d2s_intrinsics.h vvvvvvvvvv

#ifndef _M_X64

[[nodiscard]] __forceinline uint64_t
__ryu_umul128(const uint64_t __a, const uint64_t __b,
              uint64_t *const __productHi) {
  // TRANSITION, VSO#634761
  // The casts here help MSVC to avoid calls to the __allmul library function.
  const uint32_t __aLo = static_cast<uint32_t>(__a);
  const uint32_t __aHi = static_cast<uint32_t>(__a >> 32);
  const uint32_t __bLo = static_cast<uint32_t>(__b);
  const uint32_t __bHi = static_cast<uint32_t>(__b >> 32);

  const uint64_t __b00 = static_cast<uint64_t>(__aLo) * __bLo;
  const uint64_t __b01 = static_cast<uint64_t>(__aLo) * __bHi;
  const uint64_t __b10 = static_cast<uint64_t>(__aHi) * __bLo;
  const uint64_t __b11 = static_cast<uint64_t>(__aHi) * __bHi;

  const uint32_t __b00Lo = static_cast<uint32_t>(__b00);
  const uint32_t __b00Hi = static_cast<uint32_t>(__b00 >> 32);

  const uint64_t __mid1 = __b10 + __b00Hi;
  const uint32_t __mid1Lo = static_cast<uint32_t>(__mid1);
  const uint32_t __mid1Hi = static_cast<uint32_t>(__mid1 >> 32);

  const uint64_t __mid2 = __b01 + __mid1Lo;
  const uint32_t __mid2Lo = static_cast<uint32_t>(__mid2);
  const uint32_t __mid2Hi = static_cast<uint32_t>(__mid2 >> 32);

  const uint64_t __pHi = __b11 + __mid1Hi + __mid2Hi;
  const uint64_t __pLo = (static_cast<uint64_t>(__mid2Lo) << 32) + __b00Lo;

  *__productHi = __pHi;
  return __pLo;
}

[[nodiscard]] inline uint64_t __ryu_shiftright128(const uint64_t __lo,
                                                  const uint64_t __hi,
                                                  const uint32_t __dist) {
  // For the __shiftright128 intrinsic, the shift value is always
  // modulo 64.
  // In the current implementation of the double-precision version
  // of Ryu, the shift value is always < 64.
  // (The shift value is in the range [49, 58].)
  // Check this here in case a future change requires larger shift
  // values. In this case this function needs to be adjusted.
  assert(__dist < 64);
#ifdef _WIN64
  assert(__dist > 0);
  return (__hi << (64 - __dist)) | (__lo >> __dist);
#else  // ^^^ 64-bit ^^^ / vvv 32-bit vvv
  // Avoid a 64-bit shift by taking advantage of the range of shift values.
  assert(__dist >= 32);
  return (__hi << (64 - __dist)) |
         (static_cast<uint32_t>(__lo >> 32) >> (__dist - 32));
#endif // ^^^ 32-bit ^^^
}

#endif // ^^^ intrinsics unavailable ^^^

#ifndef _WIN64

// Returns the high 64 bits of the 128-bit product of __a and __b.
[[nodiscard]] inline uint64_t __umulh(const uint64_t __a, const uint64_t __b) {
  // Reuse the __ryu_umul128 implementation.
  // Optimizers will likely eliminate the instructions used to compute the
  // low part of the product.
  uint64_t __hi;
  (void)__ryu_umul128(__a, __b, &__hi);
  return __hi;
}

// On 32-bit platforms, compilers typically generate calls to library
// functions for 64-bit divisions, even if the divisor is a constant.
//
// TRANSITION, LLVM#37932
//
// The functions here perform division-by-constant using multiplications
// in the same way as 64-bit compilers would do.
//
// NB:
// The multipliers and shift values are the ones generated by clang x64
// for expressions like x/5, x/10, etc.

[[nodiscard]] inline uint64_t __div5(const uint64_t __x) {
  return __umulh(__x, 0xCCCCCCCCCCCCCCCDu) >> 2;
}

[[nodiscard]] inline uint64_t __div10(const uint64_t __x) {
  return __umulh(__x, 0xCCCCCCCCCCCCCCCDu) >> 3;
}

[[nodiscard]] inline uint64_t __div100(const uint64_t __x) {
  return __umulh(__x >> 2, 0x28F5C28F5C28F5C3u) >> 2;
}

[[nodiscard]] inline uint64_t __div1e8(const uint64_t __x) {
  return __umulh(__x, 0xABCC77118461CEFDu) >> 26;
}

[[nodiscard]] inline uint64_t __div1e9(const uint64_t __x) {
  return __umulh(__x >> 9, 0x44B82FA09B5A53u) >> 11;
}

#else // ^^^ 32-bit ^^^ / vvv 64-bit vvv

[[nodiscard]] inline uint64_t __div5(const uint64_t __x) { return __x / 5; }

[[nodiscard]] inline uint64_t __div10(const uint64_t __x) { return __x / 10; }

[[nodiscard]] inline uint64_t __div100(const uint64_t __x) { return __x / 100; }

[[nodiscard]] inline uint64_t __div1e8(const uint64_t __x) {
  return __x / 100000000;
}

[[nodiscard]] inline uint64_t __div1e9(const uint64_t __x) {
  return __x / 1000000000;
}

#endif // ^^^ 64-bit ^^^

// ^^^^^^^^^^ DERIVED FROM d2s_intrinsics.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM d2s.c vvvvvvvvvv

[[nodiscard]] inline uint32_t __pow5Factor(uint64_t __value) {
  uint32_t __count = 0;
  for (;;) {
    assert(__value != 0);
    const uint64_t __q = __div5(__value);
    const uint32_t __r = static_cast<uint32_t>(__value - 5 * __q);
    if (__r != 0) {
      break;
    }
    __value = __q;
    ++__count;
  }
  return __count;
}

// Returns true if __value is divisible by 5^__p.
[[nodiscard]] inline bool __multipleOfPowerOf5(const uint64_t __value,
                                               const uint32_t __p) {
  // I tried a case distinction on __p, but there was no performance difference.
  return __pow5Factor(__value) >= __p;
}

// Returns true if __value is divisible by 2^__p.
[[nodiscard]] inline bool __multipleOfPowerOf2(const uint64_t __value,
                                               const uint32_t __p) {
  // return __builtin_ctzll(__value) >= __p;
  return (__value & ((1ull << __p) - 1)) == 0;
}

// We need a 64x128-bit multiplication and a subsequent 128-bit shift.
// Multiplication:
//   The 64-bit factor is variable and passed in, the 128-bit factor comes
//   from a lookup table. We know that the 64-bit factor only has 55
//   significant bits (i.e., the 9 topmost bits are zeros). The 128-bit
//   factor only has 124 significant bits (i.e., the 4 topmost bits are
//   zeros).
// Shift:
//   In principle, the multiplication result requires 55 + 124 = 179 bits to
//   represent. However, we then shift this value to the right by __j, which is
//   at least __j >= 115, so the result is guaranteed to fit into 179 - 115 = 64
//   bits. This means that we only need the topmost 64 significant bits of
//   the 64x128-bit multiplication.
//
// There are several ways to do this:
// 1. Best case: the compiler exposes a 128-bit type.
//    We perform two 64x64-bit multiplications, add the higher 64 bits of the
//    lower result to the higher result, and shift by __j - 64 bits.
//
//    We explicitly cast from 64-bit to 128-bit, so the compiler can tell
//    that these are only 64-bit inputs, and can map these to the best
//    possible sequence of assembly instructions.
//    x64 machines happen to have matching assembly instructions for
//    64x64-bit multiplications and 128-bit shifts.
//
// 2. Second best case: the compiler exposes intrinsics for the x64 assembly
//    instructions mentioned in 1.
//
// 3. We only have 64x64 bit instructions that return the lower 64 bits of
//    the result, i.e., we have to use plain C.
//    Our inputs are less than the full width, so we have three options:
//    a. Ignore this fact and just implement the intrinsics manually.
//    b. Split both into 31-bit pieces, which guarantees no internal overflow,
//       but requires extra work upfront (unless we change the lookup table).
//    c. Split only the first factor into 31-bit pieces, which also guarantees
//       no internal overflow, but requires extra work since the intermediate
//       results are not perfectly aligned.
#ifdef _M_X64

[[nodiscard]] inline uint64_t
__mulShift(const uint64_t __m, const uint64_t *const __mul, const int32_t __j) {
  // __m is maximum 55 bits
  uint64_t __high1;                                          // 128
  const uint64_t __low1 = _umul128(__m, __mul[1], &__high1); // 64
  uint64_t __high0;                                          // 64
  (void)_umul128(__m, __mul[0], &__high0);                   // 0
  const uint64_t __sum = __high0 + __low1;
  if (__sum < __high0) {
    ++__high1; // overflow into __high1
  }
  return __shiftright128(__sum, __high1, static_cast<unsigned char>(__j - 64));
}

[[nodiscard]] inline uint64_t
__mulShiftAll(const uint64_t __m, const uint64_t *const __mul,
              const int32_t __j, uint64_t *const __vp, uint64_t *const __vm,
              const uint32_t __mmShift) {
  *__vp = __mulShift(4 * __m + 2, __mul, __j);
  *__vm = __mulShift(4 * __m - 1 - __mmShift, __mul, __j);
  return __mulShift(4 * __m, __mul, __j);
}

#else // ^^^ intrinsics available ^^^ / vvv intrinsics unavailable vvv

[[nodiscard]] __forceinline uint64_t
__mulShiftAll(uint64_t __m, const uint64_t *const __mul, const int32_t __j,
              uint64_t *const __vp, uint64_t *const __vm,
              const uint32_t __mmShift) { // TRANSITION, VSO#634761
  __m <<= 1;
  // __m is maximum 55 bits
  uint64_t __tmp;
  const uint64_t __lo = __ryu_umul128(__m, __mul[0], &__tmp);
  uint64_t __hi;
  const uint64_t __mid = __tmp + __ryu_umul128(__m, __mul[1], &__hi);
  __hi += __mid < __tmp; // overflow into __hi

  const uint64_t __lo2 = __lo + __mul[0];
  const uint64_t __mid2 = __mid + __mul[1] + (__lo2 < __lo);
  const uint64_t __hi2 = __hi + (__mid2 < __mid);
  *__vp =
      __ryu_shiftright128(__mid2, __hi2, static_cast<uint32_t>(__j - 64 - 1));

  if (__mmShift == 1) {
    const uint64_t __lo3 = __lo - __mul[0];
    const uint64_t __mid3 = __mid - __mul[1] - (__lo3 > __lo);
    const uint64_t __hi3 = __hi - (__mid3 > __mid);
    *__vm =
        __ryu_shiftright128(__mid3, __hi3, static_cast<uint32_t>(__j - 64 - 1));
  } else {
    const uint64_t __lo3 = __lo + __lo;
    const uint64_t __mid3 = __mid + __mid + (__lo3 < __lo);
    const uint64_t __hi3 = __hi + __hi + (__mid3 < __mid);
    const uint64_t __lo4 = __lo3 - __mul[0];
    const uint64_t __mid4 = __mid3 - __mul[1] - (__lo4 > __lo3);
    const uint64_t __hi4 = __hi3 - (__mid4 > __mid3);
    *__vm = __ryu_shiftright128(__mid4, __hi4, static_cast<uint32_t>(__j - 64));
  }

  return __ryu_shiftright128(__mid, __hi, static_cast<uint32_t>(__j - 64 - 1));
}

#endif // ^^^ intrinsics unavailable ^^^

[[nodiscard]] inline uint32_t __decimalLength(const uint64_t __v) {
  // This is slightly faster than a loop.
  // The average output length is 16.38 digits, so we check high-to-low.
  // Function precondition: __v is not an 18, 19, or 20-digit number.
  // (17 digits are sufficient for round-tripping.)
  assert(__v < 100000000000000000L);
  if (__v >= 10000000000000000L) {
    return 17;
  }
  if (__v >= 1000000000000000L) {
    return 16;
  }
  if (__v >= 100000000000000L) {
    return 15;
  }
  if (__v >= 10000000000000L) {
    return 14;
  }
  if (__v >= 1000000000000L) {
    return 13;
  }
  if (__v >= 100000000000L) {
    return 12;
  }
  if (__v >= 10000000000L) {
    return 11;
  }
  if (__v >= 1000000000L) {
    return 10;
  }
  if (__v >= 100000000L) {
    return 9;
  }
  if (__v >= 10000000L) {
    return 8;
  }
  if (__v >= 1000000L) {
    return 7;
  }
  if (__v >= 100000L) {
    return 6;
  }
  if (__v >= 10000L) {
    return 5;
  }
  if (__v >= 1000L) {
    return 4;
  }
  if (__v >= 100L) {
    return 3;
  }
  if (__v >= 10L) {
    return 2;
  }
  return 1;
}

// A floating decimal representing m * 10^e.
struct __floating_decimal_64 {
  uint64_t __mantissa;
  int32_t __exponent;
};

[[nodiscard]] inline __floating_decimal_64
__d2d(const uint64_t __ieeeMantissa, const uint32_t __ieeeExponent) {
  int32_t __e2;
  uint64_t __m2;
  if (__ieeeExponent == 0) {
    // We subtract 2 so that the bounds computation has 2 additional bits.
    __e2 = 1 - __DOUBLE_BIAS - __DOUBLE_MANTISSA_BITS - 2;
    __m2 = __ieeeMantissa;
  } else {
    __e2 = static_cast<int32_t>(__ieeeExponent) - __DOUBLE_BIAS -
           __DOUBLE_MANTISSA_BITS - 2;
    __m2 = (1ull << __DOUBLE_MANTISSA_BITS) | __ieeeMantissa;
  }
  const bool __even = (__m2 & 1) == 0;
  const bool __acceptBounds = __even;

  // Step 2: Determine the interval of valid decimal representations.
  const uint64_t __mv = 4 * __m2;
  // Implicit bool -> int conversion. True is 1, false is 0.
  const uint32_t __mmShift = __ieeeMantissa != 0 || __ieeeExponent <= 1;
  // We would compute __mp and __mm like this:
  // uint64_t __mp = 4 * __m2 + 2;
  // uint64_t __mm = __mv - 1 - __mmShift;

  // Step 3: Convert to a decimal power base using 128-bit arithmetic.
  uint64_t __vr, __vp, __vm;
  int32_t __e10;
  bool __vmIsTrailingZeros = false;
  bool __vrIsTrailingZeros = false;
  if (__e2 >= 0) {
    // I tried special-casing __q == 0, but there was no effect on performance.
    // This expression is slightly faster than max(0, __log10Pow2(__e2) - 1).
    const uint32_t __q = __log10Pow2(__e2) - (__e2 > 3);
    __e10 = static_cast<int32_t>(__q);
    const int32_t __k =
        __DOUBLE_POW5_INV_BITCOUNT + __pow5bits(static_cast<int32_t>(__q)) - 1;
    const int32_t __i = -__e2 + static_cast<int32_t>(__q) + __k;
    __vr = __mulShiftAll(__m2, __DOUBLE_POW5_INV_SPLIT[__q], __i, &__vp, &__vm,
                         __mmShift);
    if (__q <= 21) {
      // This should use __q <= 22, but I think 21 is also safe. Smaller values
      // may still be safe, but it's more difficult to reason about them.
      // Only one of __mp, __mv, and __mm can be a multiple of 5, if any.
      const uint32_t __mvMod5 = static_cast<uint32_t>(__mv - 5 * __div5(__mv));
      if (__mvMod5 == 0) {
        __vrIsTrailingZeros = __multipleOfPowerOf5(__mv, __q);
      } else if (__acceptBounds) {
        // Same as min(__e2 + (~__mm & 1), __pow5Factor(__mm)) >= __q
        // <=> __e2 + (~__mm & 1) >= __q && __pow5Factor(__mm) >= __q
        // <=> true && __pow5Factor(__mm) >= __q, since __e2 >= __q.
        __vmIsTrailingZeros = __multipleOfPowerOf5(__mv - 1 - __mmShift, __q);
      } else {
        // Same as min(__e2 + 1, __pow5Factor(__mp)) >= __q.
        __vp -= __multipleOfPowerOf5(__mv + 2, __q);
      }
    }
  } else {
    // This expression is slightly faster than max(0, __log10Pow5(-__e2) - 1).
    const uint32_t __q = __log10Pow5(-__e2) - (-__e2 > 1);
    __e10 = static_cast<int32_t>(__q) + __e2;
    const int32_t __i = -__e2 - static_cast<int32_t>(__q);
    const int32_t __k = __pow5bits(__i) - __DOUBLE_POW5_BITCOUNT;
    const int32_t __j = static_cast<int32_t>(__q) - __k;
    __vr = __mulShiftAll(__m2, __DOUBLE_POW5_SPLIT[__i], __j, &__vp, &__vm,
                         __mmShift);
    if (__q <= 1) {
      // {__vr,__vp,__vm} is trailing zeros if {__mv,__mp,__mm} has at least __q
      // trailing 0 bits.
      // __mv = 4 * __m2, so it always has at least two trailing 0 bits.
      __vrIsTrailingZeros = true;
      if (__acceptBounds) {
        // __mm = __mv - 1 - __mmShift, so it has 1 trailing 0 bit iff __mmShift
        // == 1.
        __vmIsTrailingZeros = __mmShift == 1;
      } else {
        // __mp = __mv + 2, so it always has at least one trailing 0 bit.
        --__vp;
      }
    } else if (__q < 63) { // TODO(ulfjack): Use a tighter bound here.
      // We need to compute min(ntz(__mv), __pow5Factor(__mv) - __e2) >= __q - 1
      // <=> ntz(__mv) >= __q - 1 && __pow5Factor(__mv) - __e2 >= __q - 1
      // <=> ntz(__mv) >= __q - 1 (__e2 is negative and -__e2 >= __q)
      // <=> (__mv & ((1 << (__q - 1)) - 1)) == 0
      // We also need to make sure that the left shift does not overflow.
      __vrIsTrailingZeros = __multipleOfPowerOf2(__mv, __q - 1);
    }
  }

  // Step 4: Find the shortest decimal representation in the interval of valid
  // representations.
  int32_t __removed = 0;
  uint8_t __lastRemovedDigit = 0;
  uint64_t __output;
  // On average, we remove ~2 digits.
  if (__vmIsTrailingZeros || __vrIsTrailingZeros) {
    // General case, which happens rarely (~0.7%).
    for (;;) {
      const uint64_t __vpDiv10 = __div10(__vp);
      const uint64_t __vmDiv10 = __div10(__vm);
      if (__vpDiv10 <= __vmDiv10) {
        break;
      }
      const uint32_t __vmMod10 = static_cast<uint32_t>(__vm - 10 * __vmDiv10);
      const uint64_t __vrDiv10 = __div10(__vr);
      const uint32_t __vrMod10 = static_cast<uint32_t>(__vr - 10 * __vrDiv10);
      __vmIsTrailingZeros &= __vmMod10 == 0;
      __vrIsTrailingZeros &= __lastRemovedDigit == 0;
      __lastRemovedDigit = static_cast<uint8_t>(__vrMod10);
      __vr = __vrDiv10;
      __vp = __vpDiv10;
      __vm = __vmDiv10;
      ++__removed;
    }
    if (__vmIsTrailingZeros) {
      for (;;) {
        const uint64_t __vmDiv10 = __div10(__vm);
        const uint32_t __vmMod10 = static_cast<uint32_t>(__vm - 10 * __vmDiv10);
        if (__vmMod10 != 0) {
          break;
        }
        const uint64_t __vpDiv10 = __div10(__vp);
        const uint64_t __vrDiv10 = __div10(__vr);
        const uint32_t __vrMod10 = static_cast<uint32_t>(__vr - 10 * __vrDiv10);
        __vrIsTrailingZeros &= __lastRemovedDigit == 0;
        __lastRemovedDigit = static_cast<uint8_t>(__vrMod10);
        __vr = __vrDiv10;
        __vp = __vpDiv10;
        __vm = __vmDiv10;
        ++__removed;
      }
    }
    if (__vrIsTrailingZeros && __lastRemovedDigit == 5 && __vr % 2 == 0) {
      // Round even if the exact number is .....50..0.
      __lastRemovedDigit = 4;
    }
    // We need to take __vr + 1 if __vr is outside bounds or we need to round
    // up.
    __output =
        __vr + ((__vr == __vm && (!__acceptBounds || !__vmIsTrailingZeros)) ||
                __lastRemovedDigit >= 5);
  } else {
    // Specialized for the common case (~99.3%). Percentages below are relative
    // to this.
    bool __roundUp = false;
    const uint64_t __vpDiv100 = __div100(__vp);
    const uint64_t __vmDiv100 = __div100(__vm);
    if (__vpDiv100 >
        __vmDiv100) { // Optimization: remove two digits at a time (~86.2%).
      const uint64_t __vrDiv100 = __div100(__vr);
      const uint32_t __vrMod100 =
          static_cast<uint32_t>(__vr - 100 * __vrDiv100);
      __roundUp = __vrMod100 >= 50;
      __vr = __vrDiv100;
      __vp = __vpDiv100;
      __vm = __vmDiv100;
      __removed += 2;
    }
    // Loop iterations below (approximately), without optimization above:
    // 0: 0.03%, 1: 13.8%, 2: 70.6%, 3: 14.0%, 4: 1.40%, 5: 0.14%, 6+: 0.02%
    // Loop iterations below (approximately), with optimization above:
    // 0: 70.6%, 1: 27.8%, 2: 1.40%, 3: 0.14%, 4+: 0.02%
    for (;;) {
      const uint64_t __vpDiv10 = __div10(__vp);
      const uint64_t __vmDiv10 = __div10(__vm);
      if (__vpDiv10 <= __vmDiv10) {
        break;
      }
      const uint64_t __vrDiv10 = __div10(__vr);
      const uint32_t __vrMod10 = static_cast<uint32_t>(__vr - 10 * __vrDiv10);
      __roundUp = __vrMod10 >= 5;
      __vr = __vrDiv10;
      __vp = __vpDiv10;
      __vm = __vmDiv10;
      ++__removed;
    }
    // We need to take __vr + 1 if __vr is outside bounds or we need to round
    // up.
    __output = __vr + (__vr == __vm || __roundUp);
  }
  const int32_t __exp = __e10 + __removed;

  __floating_decimal_64 __fd;
  __fd.__exponent = __exp;
  __fd.__mantissa = __output;
  return __fd;
}

[[nodiscard]] inline to_chars_result
__to_chars(wchar_t *const _First, wchar_t *const _Last,
           const __floating_decimal_64 __v, chars_format _Fmt,
           const uint64_t __ieeeMantissa, const uint32_t __ieeeExponent) {
  // Step 5: Print the decimal representation.
  uint64_t __output = __v.__mantissa;
  int32_t _Ryu_exponent = __v.__exponent;
  const uint32_t __olength = __decimalLength(__output);
  int32_t _Scientific_exponent =
      _Ryu_exponent + static_cast<int32_t>(__olength) - 1;

  if (_Fmt == chars_format{}) {
    int32_t _Lower;
    int32_t _Upper;

    if (__olength == 1) {
      // Value | Fixed   | Scientific
      // 1e-3  | "0.001" | "1e-03"
      // 1e4   | "10000" | "1e+04"
      _Lower = -3;
      _Upper = 4;
    } else {
      // Value   | Fixed       | Scientific
      // 1234e-7 | "0.0001234" | "1.234e-04"
      // 1234e5  | "123400000" | "1.234e+08"
      _Lower = -static_cast<int32_t>(__olength + 3);
      _Upper = 5;
    }

    if (_Lower <= _Ryu_exponent && _Ryu_exponent <= _Upper) {
      _Fmt = chars_format::fixed;
    } else {
      _Fmt = chars_format::scientific;
    }
  } else if (_Fmt == chars_format::general) {
    // C11 7.21.6.1 "The fprintf function"/8:
    // "Let P equal [...] 6 if the precision is omitted [...].
    // Then, if a conversion with style E would have an exponent of X:
    // - if P > X >= -4, the conversion is with style f [...].
    // - otherwise, the conversion is with style e [...]."
    if (-4 <= _Scientific_exponent && _Scientific_exponent < 6) {
      _Fmt = chars_format::fixed;
    } else {
      _Fmt = chars_format::scientific;
    }
  }

  if (_Fmt == chars_format::fixed) {
    // Example: __output == 1729, __olength == 4

    // _Ryu_exponent | Printed  | _Whole_digits | _Total_fixed_length  | Notes
    // --------------|----------|---------------|----------------------|---------------------------------------
    //             2 | 172900   |  6            | _Whole_digits        | Ryu
    //             can't be used for printing 1 | 17290    |  5            |
    //             (sometimes adjusted) | when the trimmed digits are nonzero.
    // --------------|----------|---------------|----------------------|---------------------------------------
    //             0 | 1729     |  4            | _Whole_digits        | Unified
    //             length cases.
    // --------------|----------|---------------|----------------------|---------------------------------------
    //            -1 | 172.9    |  3            | __olength + 1        | This
    //            case can't happen for -2 | 17.29    |  2            | |
    //            __olength == 1, but no additional -3 | 1.729    |  1 | | code
    //            is needed to avoid it.
    // --------------|----------|---------------|----------------------|---------------------------------------
    //            -4 | 0.1729   |  0            | 2 - _Ryu_exponent    |
    //            C11 7.21.6.1 "The fprintf function"/8: -5 | 0.01729  | -1 | |
    //            "If a decimal-point character appears, -6 | 0.001729 | -2 | |
    //            at least one digit appears before it."

    const int32_t _Whole_digits =
        static_cast<int32_t>(__olength) + _Ryu_exponent;

    uint32_t _Total_fixed_length;
    if (_Ryu_exponent >= 0) { // cases "172900" and "1729"
      _Total_fixed_length = static_cast<uint32_t>(_Whole_digits);
      if (__output == 1) {
        // Rounding can affect the number of digits.
        // For example, 1e23 is exactly "99999999999999991611392" which is 23
        // digits instead of 24. We can use a lookup table to detect this and
        // adjust the total length.
        static constexpr uint8_t _Adjustment[309] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1,
            0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0,
            0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0,
            0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0,
            0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1,
            0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0,
            0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1,
            0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0,
            1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1,
            0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 0, 0, 0, 1, 1, 0, 1, 0};
        _Total_fixed_length -= _Adjustment[_Ryu_exponent];
        // _Whole_digits doesn't need to be adjusted because these cases won't
        // refer to it later.
      }
    } else if (_Whole_digits > 0) { // case "17.29"
      _Total_fixed_length = __olength + 1;
    } else { // case "0.001729"
      _Total_fixed_length = static_cast<uint32_t>(2 - _Ryu_exponent);
    }

    if (_Last - _First < static_cast<ptrdiff_t>(_Total_fixed_length)) {
      return {_Last, std::errc::value_too_large};
    }

    wchar_t *_Mid;
    if (_Ryu_exponent > 0) { // case "172900"
      bool _Can_use_ryu;

      if (_Ryu_exponent > 22) { // 10^22 is the largest power of 10 that's
                                // exactly representable as a double.
        _Can_use_ryu = false;
      } else {
        // Ryu generated X: __v.__mantissa * 10^_Ryu_exponent
        // __v.__mantissa == 2^_Trailing_zero_bits * (__v.__mantissa >>
        // _Trailing_zero_bits) 10^_Ryu_exponent == 2^_Ryu_exponent *
        // 5^_Ryu_exponent

        // _Trailing_zero_bits is [0, 56] (aside: because 2^56 is the largest
        // power of 2 with 17 decimal digits, which is double's round-trip
        // limit.) _Ryu_exponent is [1, 22]. Normalization adds [2, 52] (aside:
        // at least 2 because the pre-normalized mantissa is at least 5). This
        // adds up to [3, 130], which is well below double's maximum binary
        // exponent 1023.

        // Therefore, we just need to consider (__v.__mantissa >>
        // _Trailing_zero_bits) * 5^_Ryu_exponent.

        // If that product would exceed 53 bits, then X can't be exactly
        // represented as a double. (That's not a problem for round-tripping,
        // because X is close enough to the original double, but X isn't
        // mathematically equal to the original double.) This requires a
        // high-precision fallback.

        // If the product is 53 bits or smaller, then X can be exactly
        // represented as a double (and we don't need to re-synthesize it; the
        // original double must have been X, because Ryu wouldn't produce the
        // same output for two different doubles X and Y). This allows Ryu's
        // output to be used (zero-filled).

        // (2^53 - 1) / 5^0 (for indexing), (2^53 - 1) / 5^1, ..., (2^53 - 1) /
        // 5^22
        static constexpr uint64_t _Max_shifted_mantissa[23] = {
            9007199254740991u,
            1801439850948198u,
            360287970189639u,
            72057594037927u,
            14411518807585u,
            2882303761517u,
            576460752303u,
            115292150460u,
            23058430092u,
            4611686018u,
            922337203u,
            184467440u,
            36893488u,
            7378697u,
            1475739u,
            295147u,
            59029u,
            11805u,
            2361u,
            472u,
            94u,
            18u,
            3u};

        unsigned long _Trailing_zero_bits;
#ifdef _WIN64
        (void)_BitScanForward64(
            &_Trailing_zero_bits,
            __v.__mantissa); // __v.__mantissa is guaranteed nonzero
#else                        // ^^^ 64-bit ^^^ / vvv 32-bit vvv
        const uint32_t _Low_mantissa = static_cast<uint32_t>(__v.__mantissa);
        if (_Low_mantissa != 0) {
          (void)_BitScanForward(&_Trailing_zero_bits, _Low_mantissa);
        } else {
          const uint32_t _High_mantissa =
              static_cast<uint32_t>(__v.__mantissa >> 32); // nonzero here
          (void)_BitScanForward(&_Trailing_zero_bits, _High_mantissa);
          _Trailing_zero_bits += 32;
        }
#endif                       // ^^^ 32-bit ^^^
        const uint64_t _Shifted_mantissa =
            __v.__mantissa >> _Trailing_zero_bits;
        _Can_use_ryu =
            _Shifted_mantissa <= _Max_shifted_mantissa[_Ryu_exponent];
      }

      if (!_Can_use_ryu) {
        // Print the integer _M2 * 2^_E2 exactly.
        const uint64_t _M2 =
            __ieeeMantissa |
            (1ull << __DOUBLE_MANTISSA_BITS); // restore implicit bit
        const uint32_t _E2 = __ieeeExponent - __DOUBLE_BIAS -
                             __DOUBLE_MANTISSA_BITS; // bias and normalization

        // For nonzero integers, static_cast<int>(_E2) >= -52. (The minimum
        // value occurs when _M2 * 2^_E2 is 1. In that case, _M2 is the implicit
        // 1 bit followed by 52 zeros, so _E2 is -52 to shift away the zeros.)
        // Negative exponents could be handled by shifting _M2, then setting _E2
        // to 0. However, this isn't necessary. The dense range of exactly
        // representable integers has negative or zero exponents (as positive
        // exponents make the range non-dense). For that dense range, Ryu will
        // always be used: every digit is necessary to uniquely identify the
        // value, so Ryu must print them all. Contrapositive: if Ryu can't be
        // used, the exponent must be positive.
        assert(static_cast<int>(_E2) > 0);
        assert(_E2 <= 971); // because __ieeeExponent <= 2046

        // Manually represent _M2 * 2^_E2 as a large integer.
        // _M2 is always 53 bits (due to the implicit bit), while _E2 indicates
        // a shift of at most 971 bits. 53 + 971 equals 1024 equals 32 * 32, so
        // we need exactly 32 32-bit elements. We use a little-endian
        // representation, visualized like this:

        // << left shift <<
        // most significant
        // _Data[31] _Data[30] ... _Data[1] _Data[0]
        //                         least significant
        //                         >> right shift >>

        constexpr uint32_t _Data_size = 32;
        uint32_t _Data[_Data_size]{}; // zero-initialized; performance note:
                                      // might be suboptimal
        uint32_t _Maxidx = ((53 + _E2 + 31) / 32) -
                           1; // index of most significant nonzero element
        assert(_Maxidx >= 1); // For double, the integer is always large.
        assert(_Maxidx < _Data_size);

        const uint32_t _Bit_shift = _E2 % 32;
        const uint64_t _Shifted = _M2 << _Bit_shift;
        if (_Bit_shift <= 11) { // _M2's 53 bits occupy two elements
          _Data[_Maxidx - 1] = static_cast<uint32_t>(_Shifted);
          _Data[_Maxidx] = static_cast<uint32_t>(_Shifted >> 32);
        } else { // _M2's 53 bits occupy three elements
          _Data[_Maxidx - 2] = static_cast<uint32_t>(_Shifted);
          _Data[_Maxidx - 1] = static_cast<uint32_t>(_Shifted >> 32);
          _Data[_Maxidx] =
              static_cast<uint32_t>(_M2 >> 32) >> (32 - _Bit_shift);
        }

        // Print the decimal digits within [_First, _First +
        // _Total_fixed_length), right to left.
        _Mid = _First + _Total_fixed_length;

        // For double, we always perform long division.
        for (;;) {
          // Loop invariant: _Maxidx != 0 (i.e. the integer is actually large)

          const uint32_t _Most_significant_elem = _Data[_Maxidx];
          const uint32_t _Initial_remainder =
              _Most_significant_elem % 1000000000;
          const uint32_t _Initial_quotient =
              _Most_significant_elem / 1000000000;
          _Data[_Maxidx] = _Initial_quotient;
          uint64_t _Remainder = _Initial_remainder;

          // Process less significant elements.
          uint32_t _Idx = _Maxidx;
          do {
            --_Idx; // Initially, _Remainder is at most 10^9 - 1.

            // Now, _Remainder is at most (10^9 - 1) * 2^32 + 2^32 - 1,
            // simplified to 10^9 * 2^32 - 1.
            _Remainder = (_Remainder << 32) | _Data[_Idx];

            // floor((10^9 * 2^32 - 1) / 10^9) == 2^32 - 1, so uint32_t
            // _Quotient is lossless.
            const uint32_t _Quotient =
                static_cast<uint32_t>(__div1e9(_Remainder));

            // _Remainder is at most 10^9 - 1 again.
            _Remainder = _Remainder - 1000000000ull * _Quotient;

            _Data[_Idx] = _Quotient;
          } while (_Idx != 0);

          // Print a 0-filled 9-digit block.
          uint32_t _Output = static_cast<uint32_t>(_Remainder);
          const uint32_t __c = _Output % 10000;
          _Output /= 10000;
          const uint32_t __d = _Output % 10000;
          _Output /= 10000;
          const uint32_t __c0 = (__c % 100) << 1;
          const uint32_t __c1 = (__c / 100) << 1;
          const uint32_t __d0 = (__d % 100) << 1;
          const uint32_t __d1 = (__d / 100) << 1;
          ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c0, 2);
          ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c1, 2);
          ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __d0, 2);
          ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __d1, 2);
          *--_Mid = static_cast<char>('0' + _Output);

          if (_Initial_quotient == 0) { // Is the large integer shrinking?
            --_Maxidx; // log2(10^9) is 29.9, so we can't shrink by more than
                       // one element.
            if (_Maxidx == 0) {
              break; // We've finished long division. Now we need to print
                     // _Data[0].
            }
          }
        }

        assert(_Data[0] != 0);
        for (uint32_t _Idx = 1; _Idx < _Data_size; ++_Idx) {
          assert(_Data[_Idx] == 0);
        }

        // Fall through to print _Data[0]. The code below can handle
        // the full range of uint32_t (but not the full range of uint64_t).
        __output = _Data[0];
        _Ryu_exponent = 0; // Imitate case "1729" to avoid further processing.
      } else {             // _Can_use_ryu
        // Print the decimal digits, left-aligned within [_First, _First +
        // _Total_fixed_length).
        _Mid = _First + __olength;
      }
    } else { // cases "1729", "17.29", and "0.001729"
      // Print the decimal digits, right-aligned within [_First, _First +
      // _Total_fixed_length).
      _Mid = _First + _Total_fixed_length;
    }

    // We prefer 32-bit operations, even on 64-bit platforms.
    // We have at most 17 digits, and uint32_t can store 9 digits.
    // If __output doesn't fit into uint32_t, we cut off 8 digits,
    // so the rest will fit into uint32_t.
    if ((__output >> 32) != 0) {
      // Expensive 64-bit division.
      const uint64_t __q = __div1e8(__output);
      uint32_t __output2 = static_cast<uint32_t>(__output - 100000000 * __q);
      __output = __q;

      const uint32_t __c = __output2 % 10000;
      __output2 /= 10000;
      const uint32_t __d = __output2 % 10000;
      const uint32_t __c0 = (__c % 100) << 1;
      const uint32_t __c1 = (__c / 100) << 1;
      const uint32_t __d0 = (__d % 100) << 1;
      const uint32_t __d1 = (__d / 100) << 1;

      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c0, 2);
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c1, 2);
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __d0, 2);
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __d1, 2);
    }
    uint32_t __output2 = static_cast<uint32_t>(__output);
    while (__output2 >= 10000) {
#ifdef __clang__ // TRANSITION, LLVM#38217
      const uint32_t __c = __output2 - 10000 * (__output2 / 10000);
#else
      const uint32_t __c = __output2 % 10000;
#endif
      __output2 /= 10000;
      const uint32_t __c0 = (__c % 100) << 1;
      const uint32_t __c1 = (__c / 100) << 1;
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c0, 2);
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c1, 2);
    }
    if (__output2 >= 100) {
      const uint32_t __c = (__output2 % 100) << 1;
      __output2 /= 100;
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c, 2);
    }
    if (__output2 >= 10) {
      const uint32_t __c = __output2 << 1;
      ArrayCopy(_Mid -= 2, __DIGIT_TABLE + __c, 2);
    } else {
      *--_Mid = static_cast<wchar_t>('0' + __output2);
    }

    if (_Ryu_exponent > 0) { // case "172900" with _Can_use_ryu
      // Performance note: it might be more efficient to do this immediately
      // after setting _Mid.
      wmemset(_First + __olength, '0', static_cast<size_t>(_Ryu_exponent));
    } else if (_Ryu_exponent ==
               0) { // case "1729", and case "172900" with !_Can_use_ryu
      // Done!
    } else if (_Whole_digits > 0) { // case "17.29"
      // Performance note: moving digits might not be optimal.
      wmemmove(_First, _First + 1, static_cast<size_t>(_Whole_digits));
      _First[_Whole_digits] = '.';
    } else { // case "0.001729"
      // Performance note: a larger memset() followed by overwriting '.' might
      // be more efficient.
      _First[0] = '0';
      _First[1] = '.';
      wmemset(_First + 2, L'0', static_cast<size_t>(-_Whole_digits));
    }

    return {_First + _Total_fixed_length, std::errc{}};
  }

  const uint32_t _Total_scientific_length =
      __olength + (__olength > 1) // digits + possible decimal point
      + (-100 < _Scientific_exponent && _Scientific_exponent < 100
             ? 4
             : 5); // + scientific exponent
  if (_Last - _First < static_cast<ptrdiff_t>(_Total_scientific_length)) {
    return {_Last, std::errc::value_too_large};
  }
  wchar_t *const __result = _First;

  // Print the decimal digits.
  uint32_t __i = 0;
  // We prefer 32-bit operations, even on 64-bit platforms.
  // We have at most 17 digits, and uint32_t can store 9 digits.
  // If __output doesn't fit into uint32_t, we cut off 8 digits,
  // so the rest will fit into uint32_t.
  if ((__output >> 32) != 0) {
    // Expensive 64-bit division.
    const uint64_t __q = __div1e8(__output);
    uint32_t __output2 = static_cast<uint32_t>(__output - 100000000 * __q);
    __output = __q;

    const uint32_t __c = __output2 % 10000;
    __output2 /= 10000;
    const uint32_t __d = __output2 % 10000;
    const uint32_t __c0 = (__c % 100) << 1;
    const uint32_t __c1 = (__c / 100) << 1;
    const uint32_t __d0 = (__d % 100) << 1;
    const uint32_t __d1 = (__d / 100) << 1;
    ArrayCopy(__result + __olength - __i - 1, __DIGIT_TABLE + __c0, 2);
    ArrayCopy(__result + __olength - __i - 3, __DIGIT_TABLE + __c1, 2);
    ArrayCopy(__result + __olength - __i - 5, __DIGIT_TABLE + __d0, 2);
    ArrayCopy(__result + __olength - __i - 7, __DIGIT_TABLE + __d1, 2);
    __i += 8;
  }
  uint32_t __output2 = static_cast<uint32_t>(__output);
  while (__output2 >= 10000) {
#ifdef __clang__ // TRANSITION, LLVM#38217
    const uint32_t __c = __output2 - 10000 * (__output2 / 10000);
#else
    const uint32_t __c = __output2 % 10000;
#endif
    __output2 /= 10000;
    const uint32_t __c0 = (__c % 100) << 1;
    const uint32_t __c1 = (__c / 100) << 1;
    ArrayCopy(__result + __olength - __i - 1, __DIGIT_TABLE + __c0, 2);
    ArrayCopy(__result + __olength - __i - 3, __DIGIT_TABLE + __c1, 2);
    __i += 4;
  }
  if (__output2 >= 100) {
    const uint32_t __c = (__output2 % 100) << 1;
    __output2 /= 100;
    ArrayCopy(__result + __olength - __i - 1, __DIGIT_TABLE + __c, 2);
    __i += 2;
  }
  if (__output2 >= 10) {
    const uint32_t __c = __output2 << 1;
    // We can't use memcpy here: the decimal dot goes between these two digits.
    __result[2] = __DIGIT_TABLE[__c + 1];
    __result[0] = __DIGIT_TABLE[__c];
  } else {
    __result[0] = static_cast<char>('0' + __output2);
  }

  // Print decimal point if needed.
  uint32_t __index;
  if (__olength > 1) {
    __result[1] = '.';
    __index = __olength + 1;
  } else {
    __index = 1;
  }

  // Print the exponent.
  __result[__index++] = 'e';
  if (_Scientific_exponent < 0) {
    __result[__index++] = '-';
    _Scientific_exponent = -_Scientific_exponent;
  } else {
    __result[__index++] = '+';
  }

  if (_Scientific_exponent >= 100) {
    const int32_t __c = _Scientific_exponent % 10;
    ArrayCopy(__result + __index,
              __DIGIT_TABLE + 2 * (_Scientific_exponent / 10), 2);
    __result[__index + 2] = static_cast<wchar_t>('0' + __c);
    __index += 3;
  } else {
    ArrayCopy(__result + __index, __DIGIT_TABLE + 2 * _Scientific_exponent, 2);
    __index += 2;
  }

  return {_First + _Total_scientific_length, std::errc{}};
}

[[nodiscard]] inline to_chars_result __d2s_buffered_n(wchar_t *const _First,
                                                      wchar_t *const _Last,
                                                      const double __f,
                                                      const chars_format _Fmt) {

  // Step 1: Decode the floating-point number, and unify normalized and
  // subnormal cases.
  const uint64_t __bits = __double_to_bits(__f);

  // Case distinction; exit early for the easy cases.
  if (__bits == 0) {
    if (_Fmt == chars_format::scientific) {
      if (_Last - _First < 5) {
        return {_Last, std::errc::value_too_large};
      }

      ArrayCopy(_First, L"0e+00", 5);

      return {_First + 5, std::errc{}};
    }

    // Print "0" for chars_format::fixed, chars_format::general, and
    // chars_format{}.
    if (_First == _Last) {
      return {_Last, std::errc::value_too_large};
    }

    *_First = '0';

    return {_First + 1, std::errc{}};
  }

  // Decode __bits into mantissa and exponent.
  const uint64_t __ieeeMantissa =
      __bits & ((1ull << __DOUBLE_MANTISSA_BITS) - 1);
  const uint32_t __ieeeExponent =
      static_cast<uint32_t>(__bits >> __DOUBLE_MANTISSA_BITS);

  const __floating_decimal_64 __v = __d2d(__ieeeMantissa, __ieeeExponent);
  return __to_chars(_First, _Last, __v, _Fmt, __ieeeMantissa, __ieeeExponent);
}

// ^^^^^^^^^^ DERIVED FROM d2s.c ^^^^^^^^^^

// clang-format on

template <class _Floating>
[[nodiscard]] inline to_chars_result
_Floating_to_chars_ryu(wchar_t *const _First, wchar_t *const _Last,
                       const _Floating _Value,
                       const chars_format _Fmt) noexcept { // strengthened
  if constexpr (std::is_same_v<_Floating, float>) {
    return __f2s_buffered_n(_First, _Last, _Value, _Fmt);
  } else {
    return __d2s_buffered_n(_First, _Last, _Value, _Fmt);
  }
}

struct _Big_integer_flt {
  _Big_integer_flt() noexcept : _Myused(0) {}

  _Big_integer_flt(const _Big_integer_flt &_Other) noexcept
      : _Myused(_Other._Myused) {
    memcpy(_Mydata, _Other._Mydata, _Other._Myused * sizeof(uint32_t));
  }

  _Big_integer_flt &operator=(const _Big_integer_flt &_Other) noexcept {
    _Myused = _Other._Myused;
    memmove(_Mydata, _Other._Mydata, _Other._Myused * sizeof(uint32_t));
    return *this;
  }

  [[nodiscard]] bool operator<(const _Big_integer_flt &_Rhs) const noexcept {
    if (_Myused != _Rhs._Myused) {
      return _Myused < _Rhs._Myused;
    }

    for (uint32_t _Ix = _Myused - 1; _Ix != static_cast<uint32_t>(-1); --_Ix) {
      if (_Mydata[_Ix] != _Rhs._Mydata[_Ix]) {
        return _Mydata[_Ix] < _Rhs._Mydata[_Ix];
      }
    }

    return false;
  }

  static constexpr uint32_t _Maximum_bits =
      1074   // 1074 bits required to represent 2^1074
      + 2552 // ceil(log2(10^768))
      + 32;  // shift space

  static constexpr uint32_t _Element_bits = 32;

  static constexpr uint32_t _Element_count =
      (_Maximum_bits + _Element_bits - 1) / _Element_bits;

  uint32_t _Myused;                 // The number of elements currently in use
  uint32_t _Mydata[_Element_count]; // The number, stored in little-endian form
};

[[nodiscard]] inline _Big_integer_flt _Make_big_integer_flt_one() noexcept {
  _Big_integer_flt _Xval{};
  _Xval._Mydata[0] = 1;
  _Xval._Myused = 1;
  return _Xval;
}

[[nodiscard]] inline _Big_integer_flt
_Make_big_integer_flt_u32(const uint32_t _Value) noexcept {
  _Big_integer_flt _Xval{};
  _Xval._Mydata[0] = _Value;
  _Xval._Myused = 1;
  return _Xval;
}

[[nodiscard]] inline _Big_integer_flt
_Make_big_integer_flt_u64(const uint64_t _Value) noexcept {
  _Big_integer_flt _Xval{};
  _Xval._Mydata[0] = static_cast<uint32_t>(_Value);
  _Xval._Mydata[1] = static_cast<uint32_t>(_Value >> 32);
  _Xval._Myused = _Xval._Mydata[1] == 0 ? 1u : 2u;
  return _Xval;
}

[[nodiscard]] inline _Big_integer_flt
_Make_big_integer_flt_power_of_two(const uint32_t _Power) noexcept {
  const uint32_t _Element_index = _Power / _Big_integer_flt::_Element_bits;
  const uint32_t _Bit_index = _Power % _Big_integer_flt::_Element_bits;

  _Big_integer_flt _Xval{};
  memset(_Xval._Mydata, 0, _Element_index * sizeof(uint32_t));
  _Xval._Mydata[_Element_index] = 1u << _Bit_index;
  _Xval._Myused = _Element_index + 1;
  return _Xval;
}

[[nodiscard]] inline uint32_t
_Bit_scan_reverse(const uint32_t _Value) noexcept {
  unsigned long _Index; // Intentionally uninitialized for better codegen

  if (_BitScanReverse(&_Index, _Value)) {
    return _Index + 1;
  }

  return 0;
}

[[nodiscard]] inline uint32_t
_Bit_scan_reverse(const uint64_t _Value) noexcept {
  unsigned long _Index; // Intentionally uninitialized for better codegen

#ifdef _WIN64
  if (_BitScanReverse64(&_Index, _Value)) {
    return _Index + 1;
  }
#else  // ^^^ 64-bit ^^^ / vvv 32-bit vvv
  uint32_t _Ui32 = static_cast<uint32_t>(_Value >> 32);

  if (_BitScanReverse(&_Index, _Ui32)) {
    return _Index + 1 + 32;
  }

  _Ui32 = static_cast<uint32_t>(_Value);

  if (_BitScanReverse(&_Index, _Ui32)) {
    return _Index + 1;
  }
#endif // ^^^ 32-bit ^^^

  return 0;
}

[[nodiscard]] inline uint32_t
_Bit_scan_reverse(const _Big_integer_flt &_Xval) noexcept {
  if (_Xval._Myused == 0) {
    return 0;
  }

  const uint32_t _Bx = _Xval._Myused - 1;

  assert(_Xval._Mydata[_Bx] != 0); // _Big_integer_flt should always be trimmed

  unsigned long _Index; // Intentionally uninitialized for better codegen

  _BitScanReverse(&_Index,
                  _Xval._Mydata[_Bx]); // assumes _Xval._Mydata[_Bx] != 0

  return _Index + 1 + _Bx * _Big_integer_flt::_Element_bits;
}

// Shifts the high-precision integer _Xval by _Nx bits to the left. Returns true
// if the left shift was successful; false if it overflowed. When overflow
// occurs, the high-precision integer is reset to zero.
[[nodiscard]] inline bool _Shift_left(_Big_integer_flt &_Xval,
                                      const uint32_t _Nx) noexcept {
  if (_Xval._Myused == 0) {
    return true;
  }

  const uint32_t _Unit_shift = _Nx / _Big_integer_flt::_Element_bits;
  const uint32_t _Bit_shift = _Nx % _Big_integer_flt::_Element_bits;

  const bool _Unit_shift_will_overflow =
      _Xval._Myused + _Unit_shift > _Big_integer_flt::_Element_count;

  if (_Unit_shift_will_overflow) {
    _Xval._Myused = 0;
    return false;
  }

  if (_Bit_shift == 0) {
    memmove(_Xval._Mydata + _Unit_shift, _Xval._Mydata,
            _Xval._Myused * sizeof(uint32_t));
    _Xval._Myused += _Unit_shift;
  } else {
    const bool _Bit_shifts_into_next_unit =
        _Bit_shift > (_Big_integer_flt::_Element_bits -
                      _Bit_scan_reverse(_Xval._Mydata[_Xval._Myused - 1]));

    const bool _Bit_shift_will_overflow =
        _Xval._Myused + _Unit_shift == _Big_integer_flt::_Element_count &&
        _Bit_shifts_into_next_unit;

    if (_Bit_shift_will_overflow) {
      _Xval._Myused = 0;
      return false;
    }

    const uint32_t _Msb_bits = _Bit_shift;
    const uint32_t _Lsb_bits = _Big_integer_flt::_Element_bits - _Msb_bits;

    const uint32_t _Lsb_mask = (1UL << _Lsb_bits) - 1UL;
    const uint32_t _Msb_mask = ~_Lsb_mask;

    const uint32_t _Max_destination_index = (std::min)(
        _Xval._Myused + _Unit_shift, _Big_integer_flt::_Element_count - 1);

    for (uint32_t _Destination_index = _Max_destination_index;
         _Destination_index != static_cast<uint32_t>(-1) &&
         _Destination_index >= _Unit_shift;
         --_Destination_index) { // performance note: PSLLDQ and PALIGNR
                                 // instructions could be more efficient here
      const uint32_t _Upper_source_index = _Destination_index - _Unit_shift;
      const uint32_t _Lower_source_index = _Destination_index - _Unit_shift - 1;

      const uint32_t _Upper_source = _Upper_source_index < _Xval._Myused
                                         ? _Xval._Mydata[_Upper_source_index]
                                         : 0;
      const uint32_t _Lower_source = _Lower_source_index < _Xval._Myused
                                         ? _Xval._Mydata[_Lower_source_index]
                                         : 0;

      const uint32_t _Shifted_upper_source = (_Upper_source & _Lsb_mask)
                                             << _Msb_bits;
      const uint32_t _Shifted_lower_source =
          (_Lower_source & _Msb_mask) >> _Lsb_bits;

      const uint32_t _Combined_shifted_source =
          _Shifted_upper_source | _Shifted_lower_source;

      _Xval._Mydata[_Destination_index] = _Combined_shifted_source;
    }

    _Xval._Myused = _Bit_shifts_into_next_unit ? _Max_destination_index + 1
                                               : _Max_destination_index;
  }

  memset(_Xval._Mydata, 0, _Unit_shift * sizeof(uint32_t));

  return true;
}

// Adds a 32-bit _Value to the high-precision integer _Xval. Returns true if the
// addition was successful; false if it overflowed. When overflow occurs, the
// high-precision integer is reset to zero.
[[nodiscard]] inline bool _Add(_Big_integer_flt &_Xval,
                               const uint32_t _Value) noexcept {
  if (_Value == 0) {
    return true;
  }

  uint32_t _Carry = _Value;
  for (uint32_t _Ix = 0; _Ix != _Xval._Myused; ++_Ix) {
    const uint64_t _Result = static_cast<uint64_t>(_Xval._Mydata[_Ix]) + _Carry;
    _Xval._Mydata[_Ix] = static_cast<uint32_t>(_Result);
    _Carry = static_cast<uint32_t>(_Result >> 32);
  }

  if (_Carry != 0) {
    if (_Xval._Myused < _Big_integer_flt::_Element_count) {
      _Xval._Mydata[_Xval._Myused] = _Carry;
      ++_Xval._Myused;
    } else {
      _Xval._Myused = 0;
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline uint32_t _Add_carry(uint32_t &_U1, const uint32_t _U2,
                                         const uint32_t _U_carry) noexcept {
  const uint64_t _Uu = static_cast<uint64_t>(_U1) + _U2 + _U_carry;
  _U1 = static_cast<uint32_t>(_Uu);
  return static_cast<uint32_t>(_Uu >> 32);
}

[[nodiscard]] inline uint32_t
_Add_multiply_carry(uint32_t &_U_add, const uint32_t _U_mul_1,
                    const uint32_t _U_mul_2, const uint32_t _U_carry) noexcept {
  const uint64_t _Uu_res =
      static_cast<uint64_t>(_U_mul_1) * _U_mul_2 + _U_add + _U_carry;
  _U_add = static_cast<uint32_t>(_Uu_res);
  return static_cast<uint32_t>(_Uu_res >> 32);
}

[[nodiscard]] inline uint32_t
_Multiply_core(uint32_t *const _Multiplicand,
               const uint32_t _Multiplicand_count,
               const uint32_t _Multiplier) noexcept {
  uint32_t _Carry = 0;
  for (uint32_t _Ix = 0; _Ix != _Multiplicand_count; ++_Ix) {
    const uint64_t _Result =
        static_cast<uint64_t>(_Multiplicand[_Ix]) * _Multiplier + _Carry;
    _Multiplicand[_Ix] = static_cast<uint32_t>(_Result);
    _Carry = static_cast<uint32_t>(_Result >> 32);
  }

  return _Carry;
}

// Multiplies the high-precision _Multiplicand by a 32-bit _Multiplier. Returns
// true if the multiplication was successful; false if it overflowed. When
// overflow occurs, the _Multiplicand is reset to zero.
[[nodiscard]] inline bool _Multiply(_Big_integer_flt &_Multiplicand,
                                    const uint32_t _Multiplier) noexcept {
  if (_Multiplier == 0) {
    _Multiplicand._Myused = 0;
    return true;
  }

  if (_Multiplier == 1) {
    return true;
  }

  if (_Multiplicand._Myused == 0) {
    return true;
  }

  const uint32_t _Carry =
      _Multiply_core(_Multiplicand._Mydata, _Multiplicand._Myused, _Multiplier);
  if (_Carry != 0) {
    if (_Multiplicand._Myused < _Big_integer_flt::_Element_count) {
      _Multiplicand._Mydata[_Multiplicand._Myused] = _Carry;
      ++_Multiplicand._Myused;
    } else {
      _Multiplicand._Myused = 0;
      return false;
    }
  }

  return true;
}

// This high-precision integer multiplication implementation was translated from
// the implementation of System.Numerics.BigIntegerBuilder.Mul in the .NET
// Framework sources. It multiplies the _Multiplicand by the _Multiplier and
// returns true if the multiplication was successful; false if it overflowed.
// When overflow occurs, the _Multiplicand is reset to zero.
[[nodiscard]] inline bool
_Multiply(_Big_integer_flt &_Multiplicand,
          const _Big_integer_flt &_Multiplier) noexcept {
  if (_Multiplicand._Myused == 0) {
    return true;
  }

  if (_Multiplier._Myused == 0) {
    _Multiplicand._Myused = 0;
    return true;
  }

  if (_Multiplier._Myused == 1) {
    return _Multiply(
        _Multiplicand,
        _Multiplier._Mydata[0]); // when overflow occurs, resets to zero
  }

  if (_Multiplicand._Myused == 1) {
    const uint32_t _Small_multiplier = _Multiplicand._Mydata[0];
    _Multiplicand = _Multiplier;
    return _Multiply(_Multiplicand,
                     _Small_multiplier); // when overflow occurs, resets to zero
  }

  // We prefer more iterations on the inner loop and fewer on the outer:
  const bool _Multiplier_is_shorter =
      _Multiplier._Myused < _Multiplicand._Myused;
  const uint32_t *const _Rgu1 =
      _Multiplier_is_shorter ? _Multiplier._Mydata : _Multiplicand._Mydata;
  const uint32_t *const _Rgu2 =
      _Multiplier_is_shorter ? _Multiplicand._Mydata : _Multiplier._Mydata;

  const uint32_t _Cu1 =
      _Multiplier_is_shorter ? _Multiplier._Myused : _Multiplicand._Myused;
  const uint32_t _Cu2 =
      _Multiplier_is_shorter ? _Multiplicand._Myused : _Multiplier._Myused;

  _Big_integer_flt _Result{};
  for (uint32_t _Iu1 = 0; _Iu1 != _Cu1; ++_Iu1) {
    const uint32_t _U_cur = _Rgu1[_Iu1];
    if (_U_cur == 0) {
      if (_Iu1 == _Result._Myused) {
        _Result._Mydata[_Iu1] = 0;
        _Result._Myused = _Iu1 + 1;
      }

      continue;
    }

    uint32_t _U_carry = 0;
    uint32_t _Iu_res = _Iu1;
    for (uint32_t _Iu2 = 0;
         _Iu2 != _Cu2 && _Iu_res != _Big_integer_flt::_Element_count;
         ++_Iu2, ++_Iu_res) {
      if (_Iu_res == _Result._Myused) {
        _Result._Mydata[_Iu_res] = 0;
        _Result._Myused = _Iu_res + 1;
      }

      _U_carry = _Add_multiply_carry(_Result._Mydata[_Iu_res], _U_cur,
                                     _Rgu2[_Iu2], _U_carry);
    }

    while (_U_carry != 0 && _Iu_res != _Big_integer_flt::_Element_count) {
      if (_Iu_res == _Result._Myused) {
        _Result._Mydata[_Iu_res] = 0;
        _Result._Myused = _Iu_res + 1;
      }

      _U_carry = _Add_carry(_Result._Mydata[_Iu_res++], 0, _U_carry);
    }

    if (_Iu_res == _Big_integer_flt::_Element_count) {
      _Multiplicand._Myused = 0;
      return false;
    }
  }

  // Store the _Result in the _Multiplicand and compute the actual number of
  // elements used:
  _Multiplicand = _Result;
  return true;
}

// Multiplies the high-precision integer _Xval by 10^_Power. Returns true if the
// multiplication was successful; false if it overflowed. When overflow occurs,
// the high-precision integer is reset to zero.
[[nodiscard]] inline bool
_Multiply_by_power_of_ten(_Big_integer_flt &_Xval,
                          const uint32_t _Power) noexcept {
  // To improve performance, we use a table of precomputed powers of ten, from
  // 10^10 through 10^380, in increments of ten. In its unpacked form, as an
  // array of _Big_integer_flt objects, this table consists mostly of zero
  // elements. Thus, we store the table in a packed form, trimming leading and
  // trailing zero elements. We provide an index that is used to unpack powers
  // from the table, using the function that appears after this function in this
  // file.

  // The minimum value representable with double-precision is 5E-324.
  // With this table we can thus compute most multiplications with a single
  // multiply.

  static constexpr uint32_t _Large_power_data[] = {
      0x540be400, 0x00000002, 0x63100000, 0x6bc75e2d, 0x00000005, 0x40000000,
      0x4674edea, 0x9f2c9cd0, 0x0000000c, 0xb9f56100, 0x5ca4bfab, 0x6329f1c3,
      0x0000001d, 0xb5640000, 0xc40534fd, 0x926687d2, 0x6c3b15f9, 0x00000044,
      0x10000000, 0x946590d9, 0xd762422c, 0x9a224501, 0x4f272617, 0x0000009f,
      0x07950240, 0x245689c1, 0xc5faa71c, 0x73c86d67, 0xebad6ddc, 0x00000172,
      0xcec10000, 0x63a22764, 0xefa418ca, 0xcdd17b25, 0x6bdfef70, 0x9dea3e1f,
      0x0000035f, 0xe4000000, 0xcdc3fe6e, 0x66bc0c6a, 0x2e391f32, 0x5a450203,
      0x71d2f825, 0xc3c24a56, 0x000007da, 0xa82e8f10, 0xaab24308, 0x8e211a7c,
      0xf38ace40, 0x84c4ce0b, 0x7ceb0b27, 0xad2594c3, 0x00001249, 0xdd1a4000,
      0xcc9f54da, 0xdc5961bf, 0xc75cabab, 0xf505440c, 0xd1bc1667, 0xfbb7af52,
      0x608f8d29, 0x00002a94, 0x21000000, 0x17bb8a0c, 0x56af8ea4, 0x06479fa9,
      0x5d4bb236, 0x80dc5fe0, 0xf0feaa0a, 0xa88ed940, 0x6b1a80d0, 0x00006323,
      0x324c3864, 0x8357c796, 0xe44a42d5, 0xd9a92261, 0xbd3c103d, 0x91e5f372,
      0xc0591574, 0xec1da60d, 0x102ad96c, 0x0000e6d3, 0x1e851000, 0x6e4f615b,
      0x187b2a69, 0x0450e21c, 0x2fdd342b, 0x635027ee, 0xa6c97199, 0x8e4ae916,
      0x17082e28, 0x1a496e6f, 0x0002196e, 0x32400000, 0x04ad4026, 0xf91e7250,
      0x2994d1d5, 0x665bcdbb, 0xa23b2e96, 0x65fa7ddb, 0x77de53ac, 0xb020a29b,
      0xc6bff953, 0x4b9425ab, 0x0004e34d, 0xfbc32d81, 0x5222d0f4, 0xb70f2850,
      0x5713f2f3, 0xdc421413, 0xd6395d7d, 0xf8591999, 0x0092381c, 0x86b314d6,
      0x7aa577b9, 0x12b7fe61, 0x000b616a, 0x1d11e400, 0x56c3678d, 0x3a941f20,
      0x9b09368b, 0xbd706908, 0x207665be, 0x9b26c4eb, 0x1567e89d, 0x9d15096e,
      0x7132f22b, 0xbe485113, 0x45e5a2ce, 0x001a7f52, 0xbb100000, 0x02f79478,
      0x8c1b74c0, 0xb0f05d00, 0xa9dbc675, 0xe2d9b914, 0x650f72df, 0x77284b4c,
      0x6df6e016, 0x514391c2, 0x2795c9cf, 0xd6e2ab55, 0x9ca8e627, 0x003db1a6,
      0x40000000, 0xf4ecd04a, 0x7f2388f0, 0x580a6dc5, 0x43bf046f, 0xf82d5dc3,
      0xee110848, 0xfaa0591c, 0xcdf4f028, 0x192ea53f, 0xbcd671a0, 0x7d694487,
      0x10f96e01, 0x791a569d, 0x008fa475, 0xb9b2e100, 0x8288753c, 0xcd3f1693,
      0x89b43a6b, 0x089e87de, 0x684d4546, 0xfddba60c, 0xdf249391, 0x3068ec13,
      0x99b44427, 0xb68141ee, 0x5802cac3, 0xd96851f1, 0x7d7625a2, 0x014e718d,
      0xfb640000, 0xf25a83e6, 0x9457ad0f, 0x0080b511, 0x2029b566, 0xd7c5d2cf,
      0xa53f6d7d, 0xcdb74d1c, 0xda9d70de, 0xb716413d, 0x71d0ca4e, 0xd7e41398,
      0x4f403a90, 0xf9ab3fe2, 0x264d776f, 0x030aafe6, 0x10000000, 0x09ab5531,
      0xa60c58d2, 0x566126cb, 0x6a1c8387, 0x7587f4c1, 0x2c44e876, 0x41a047cf,
      0xc908059e, 0xa0ba063e, 0xe7cfc8e8, 0xe1fac055, 0xef0144b2, 0x24207eb0,
      0xd1722573, 0xe4b8f981, 0x071505ae, 0x7a3b6240, 0xcea45d4f, 0x4fe24133,
      0x210f6d6d, 0xe55633f2, 0x25c11356, 0x28ebd797, 0xd396eb84, 0x1e493b77,
      0x471f2dae, 0x96ad3820, 0x8afaced1, 0x4edecddb, 0x5568c086, 0xb2695da1,
      0x24123c89, 0x107d4571, 0x1c410000, 0x6e174a27, 0xec62ae57, 0xef2289aa,
      0xb6a2fbdd, 0x17e1efe4, 0x3366bdf2, 0x37b48880, 0xbfb82c3e, 0x19acde91,
      0xd4f46408, 0x35ff6a4e, 0x67566a0e, 0x40dbb914, 0x782a3bca, 0x6b329b68,
      0xf5afc5d9, 0x266469bc, 0xe4000000, 0xfb805ff4, 0xed55d1af, 0x9b4a20a8,
      0xab9757f8, 0x01aefe0a, 0x4a2ca67b, 0x1ebf9569, 0xc7c41c29, 0xd8d5d2aa,
      0xd136c776, 0x93da550c, 0x9ac79d90, 0x254bcba8, 0x0df07618, 0xf7a88809,
      0x3a1f1074, 0xe54811fc, 0x59638ead, 0x97cbe710, 0x26d769e8, 0xb4e4723e,
      0x5b90aa86, 0x9c333922, 0x4b7a0775, 0x2d47e991, 0x9a6ef977, 0x160b40e7,
      0x0c92f8c4, 0xf25ff010, 0x25c36c11, 0xc9f98b42, 0x730b919d, 0x05ff7caf,
      0xb0432d85, 0x2d2b7569, 0xa657842c, 0xd01fef10, 0xc77a4000, 0xe8b862e5,
      0x10d8886a, 0xc8cd98e5, 0x108955c5, 0xd059b655, 0x58fbbed4, 0x03b88231,
      0x034c4519, 0x194dc939, 0x1fc500ac, 0x794cc0e2, 0x3bc980a1, 0xe9b12dd1,
      0x5e6d22f8, 0x7b38899a, 0xce7919d8, 0x78c67672, 0x79e5b99f, 0xe494034e,
      0x00000001, 0xa1000000, 0x6c5cd4e9, 0x9be47d6f, 0xf93bd9e7, 0x77626fa1,
      0xc68b3451, 0xde2b59e8, 0xcf3cde58, 0x2246ff58, 0xa8577c15, 0x26e77559,
      0x17776753, 0xebe6b763, 0xe3fd0a5f, 0x33e83969, 0xa805a035, 0xf631b987,
      0x211f0f43, 0xd85a43db, 0xab1bf596, 0x683f19a2, 0x00000004, 0xbe7dfe64,
      0x4bc9042f, 0xe1f5edb0, 0x8fa14eda, 0xe409db73, 0x674fee9c, 0xa9159f0d,
      0xf6b5b5d6, 0x7338960e, 0xeb49c291, 0x5f2b97cc, 0x0f383f95, 0x2091b3f6,
      0xd1783714, 0xc1d142df, 0x153e22de, 0x8aafdf57, 0x77f5e55f, 0xa3e7ca8b,
      0x032f525b, 0x42e74f3d, 0x0000000a, 0xf4dd1000, 0x5d450952, 0xaeb442e1,
      0xa3b3342e, 0x3fcda36f, 0xb4287a6e, 0x4bc177f7, 0x67d2c8d0, 0xaea8f8e0,
      0xadc93b67, 0x6cc856b3, 0x959d9d0b, 0x5b48c100, 0x4abe8a3d, 0x52d936f4,
      0x71dbe84d, 0xf91c21c5, 0x4a458109, 0xd7aad86a, 0x08e14c7c, 0x759ba59c,
      0xe43c8800, 0x00000017, 0x92400000, 0x04f110d4, 0x186472be, 0x8736c10c,
      0x1478abfb, 0xfc51af29, 0x25eb9739, 0x4c2b3015, 0xa1030e0b, 0x28fe3c3b,
      0x7788fcba, 0xb89e4358, 0x733de4a4, 0x7c46f2c2, 0x8f746298, 0xdb19210f,
      0x2ea3b6ae, 0xaa5014b2, 0xea39ab8d, 0x97963442, 0x01dfdfa9, 0xd2f3d3fe,
      0xa0790280, 0x00000037, 0x509c9b01, 0xc7dcadf1, 0x383dad2c, 0x73c64d37,
      0xea6d67d0, 0x519ba806, 0xc403f2f8, 0xa052e1a2, 0xd710233a, 0x448573a9,
      0xcf12d9ba, 0x70871803, 0x52dc3a9b, 0xe5b252e8, 0x0717fb4e, 0xbe4da62f,
      0x0aabd7e1, 0x8c62ed4f, 0xceb9ec7b, 0xd4664021, 0xa1158300, 0xcce375e6,
      0x842f29f2, 0x00000081, 0x7717e400, 0xd3f5fb64, 0xa0763d71, 0x7d142fe9,
      0x33f44c66, 0xf3b8f12e, 0x130f0d8e, 0x734c9469, 0x60260fa8, 0x3c011340,
      0xcc71880a, 0x37a52d21, 0x8adac9ef, 0x42bb31b4, 0xd6f94c41, 0xc88b056c,
      0xe20501b8, 0x5297ed7c, 0x62c361c4, 0x87dad8aa, 0xb833eade, 0x94f06861,
      0x13cc9abd, 0x8dc1d56a, 0x0000012d, 0x13100000, 0xc67a36e8, 0xf416299e,
      0xf3493f0a, 0x77a5a6cf, 0xa4be23a3, 0xcca25b82, 0x3510722f, 0xbe9d447f,
      0xa8c213b8, 0xc94c324e, 0xbc9e33ad, 0x76acfeba, 0x2e4c2132, 0x3e13cd32,
      0x70fe91b4, 0xbb5cd936, 0x42149785, 0x46cc1afd, 0xe638ddf8, 0x690787d2,
      0x1a02d117, 0x3eb5f1fe, 0xc3b9abae, 0x1c08ee6f, 0x000002be, 0x40000000,
      0x8140c2aa, 0x2cf877d9, 0x71e1d73d, 0xd5e72f98, 0x72516309, 0xafa819dd,
      0xd62a5a46, 0x2a02dcce, 0xce46ddfe, 0x2713248d, 0xb723d2ad, 0xc404bb19,
      0xb706cc2b, 0x47b1ebca, 0x9d094bdc, 0xc5dc02ca, 0x31e6518e, 0x8ec35680,
      0x342f58a8, 0x8b041e42, 0xfebfe514, 0x05fffc13, 0x6763790f, 0x66d536fd,
      0xb9e15076, 0x00000662, 0x67b06100, 0xd2010a1a, 0xd005e1c0, 0xdb12733b,
      0xa39f2e3f, 0x61b29de2, 0x2a63dce2, 0x942604bc, 0x6170d59b, 0xc2e32596,
      0x140b75b9, 0x1f1d2c21, 0xb8136a60, 0x89d23ba2, 0x60f17d73, 0xc6cad7df,
      0x0669df2b, 0x24b88737, 0x669306ed, 0x19496eeb, 0x938ddb6f, 0x5e748275,
      0xc56e9a36, 0x3690b731, 0xc82842c5, 0x24ae798e, 0x00000ede, 0x41640000,
      0xd5889ac1, 0xd9432c99, 0xa280e71a, 0x6bf63d2e, 0x8249793d, 0x79e7a943,
      0x22fde64a, 0xe0d6709a, 0x05cacfef, 0xbd8da4d7, 0xe364006c, 0xa54edcb3,
      0xa1a8086e, 0x748f459e, 0xfc8e54c8, 0xcc74c657, 0x42b8c3d4, 0x57d9636e,
      0x35b55bcc, 0x6c13fee9, 0x1ac45161, 0xb595badb, 0xa1f14e9d, 0xdcf9e750,
      0x07637f71, 0xde2f9f2b, 0x0000229d, 0x10000000, 0x3c5ebd89, 0xe3773756,
      0x3dcba338, 0x81d29e4f, 0xa4f79e2c, 0xc3f9c774, 0x6a1ce797, 0xac5fe438,
      0x07f38b9c, 0xd588ecfa, 0x3e5ac1ac, 0x85afccce, 0x9d1f3f70, 0xe82d6dd3,
      0x177d180c, 0x5e69946f, 0x648e2ce1, 0x95a13948, 0x340fe011, 0xb4173c58,
      0x2748f694, 0x7c2657bd, 0x758bda2e, 0x3b8090a0, 0x2ddbb613, 0x6dcf4890,
      0x24e4047e, 0x00005099};

  struct _Unpack_index {
    uint16_t _Offset; // The offset of this power's initial element in the array
    uint8_t _Zeroes;  // The number of omitted leading zero elements
    uint8_t _Size;    // The number of elements present for this power
  };

  static constexpr _Unpack_index _Large_power_indices[] = {
      {0, 0, 2},     {2, 0, 3},     {5, 0, 4},     {9, 1, 4},     {13, 1, 5},
      {18, 1, 6},    {24, 2, 6},    {30, 2, 7},    {37, 2, 8},    {45, 3, 8},
      {53, 3, 9},    {62, 3, 10},   {72, 4, 10},   {82, 4, 11},   {93, 4, 12},
      {105, 5, 12},  {117, 5, 13},  {130, 5, 14},  {144, 5, 15},  {159, 6, 15},
      {174, 6, 16},  {190, 6, 17},  {207, 7, 17},  {224, 7, 18},  {242, 7, 19},
      {261, 8, 19},  {280, 8, 21},  {301, 8, 22},  {323, 9, 22},  {345, 9, 23},
      {368, 9, 24},  {392, 10, 24}, {416, 10, 25}, {441, 10, 26}, {467, 10, 27},
      {494, 11, 27}, {521, 11, 28}, {549, 11, 29}};

  for (uint32_t _Large_power = _Power / 10; _Large_power != 0;) {
    const uint32_t _Current_power = (std::min)(
        _Large_power, static_cast<uint32_t>(std::size(_Large_power_indices)));

    const _Unpack_index &_Index = _Large_power_indices[_Current_power - 1];
    _Big_integer_flt _Multiplier{};
    _Multiplier._Myused = static_cast<uint32_t>(_Index._Size + _Index._Zeroes);

    const uint32_t *const _Source = _Large_power_data + _Index._Offset;

    memset(_Multiplier._Mydata, 0, _Index._Zeroes * sizeof(uint32_t));
    memcpy(_Multiplier._Mydata + _Index._Zeroes, _Source,
           _Index._Size * sizeof(uint32_t));

    if (!_Multiply(_Xval,
                   _Multiplier)) { // when overflow occurs, resets to zero
      return false;
    }

    _Large_power -= _Current_power;
  }

  static constexpr uint32_t _Small_powers_of_ten[9] = {
      10,        100,        1'000,       10'000,       100'000,
      1'000'000, 10'000'000, 100'000'000, 1'000'000'000};

  const uint32_t _Small_power = _Power % 10;

  if (_Small_power == 0) {
    return true;
  }

  return _Multiply(
      _Xval, _Small_powers_of_ten[_Small_power -
                                  1]); // when overflow occurs, resets to zero
}

// The following non-compiled code is the generator for the big powers of ten
// table found in _Multiply_by_power_of_ten(). This code is provided for future
// use if the table needs to be amended. Do not remove this code.
/*
#include <algorithm>
#include <charconv>
#include <stdint.h>
#include <stdio.h>
#include <tuple>
#include <vector>
using namespace std;

int main() {
    vector<uint32_t> elements;
    vector<tuple<uint32_t, uint32_t, uint32_t>> indices;

    for (uint32_t power = 10; power != 390; power += 10) {
        _Big_integer_flt big = _Make_big_integer_flt_one();

        for (uint32_t i = 0; i != power; ++i) {
            (void) _Multiply(big, 10); // assumes no overflow
        }

        const uint32_t* const first = big._Mydata;
        const uint32_t* const last = first + big._Myused;
        const uint32_t* const mid = find_if(first, last, [](const uint32_t elem)
{ return elem != 0; });

        indices.emplace_back(static_cast<uint32_t>(elements.size()),
static_cast<uint32_t>(mid - first), static_cast<uint32_t>(last - mid));

        elements.insert(elements.end(), mid, last);
    }

    printf("static constexpr uint32_t _Large_power_data[] =\n{");
    for (uint32_t i = 0; i != elements.size(); ++i) {
        printf("%s0x%08x, ", i % 8 == 0 ? "\n\t" : "", elements[i]);
    }
    printf("\n};\n");

    printf("static constexpr _Unpack_index _Large_power_indices[] =\n{");
    for (uint32_t i = 0; i != indices.size(); ++i) {
        printf(
            "%s{ %u, %u, %u }, ", i % 6 == 0 ? "\n\t" : "", get<0>(indices[i]),
get<1>(indices[i]), get<2>(indices[i]));
    }
    printf("\n};\n");
}
*/

// Computes the number of zeroes higher than the most significant set bit in _Ux
[[nodiscard]] inline uint32_t
_Count_sequential_high_zeroes(const uint32_t _Ux) noexcept {
  unsigned long _Index; // Intentionally uninitialized for better codegen
  return _BitScanReverse(&_Index, _Ux) ? 31 - _Index : 32;
}

// This high-precision integer division implementation was translated from the
// implementation of System.Numerics.BigIntegerBuilder.ModDivCore in the .NET
// Framework sources. It computes both quotient and remainder: the remainder is
// stored in the _Numerator argument, and the least significant 64 bits of the
// quotient are returned from the function.
[[nodiscard]] inline uint64_t
_Divide(_Big_integer_flt &_Numerator,
        const _Big_integer_flt &_Denominator) noexcept {
  // If the _Numerator is zero, then both the quotient and remainder are zero:
  if (_Numerator._Myused == 0) {
    return 0;
  }

  // If the _Denominator is zero, then uh oh. We can't divide by zero:
  assert(_Denominator._Myused != 0); // Division by zero

  uint32_t _Max_numerator_element_index = _Numerator._Myused - 1;
  const uint32_t _Max_denominator_element_index = _Denominator._Myused - 1;

  // The _Numerator and _Denominator are both nonzero.
  // If the _Denominator is only one element wide, we can take the fast route:
  if (_Max_denominator_element_index == 0) {
    const uint32_t _Small_denominator = _Denominator._Mydata[0];

    if (_Max_numerator_element_index == 0) {
      const uint32_t _Small_numerator = _Numerator._Mydata[0];

      if (_Small_denominator == 1) {
        _Numerator._Myused = 0;
        return _Small_numerator;
      }

      _Numerator._Mydata[0] = _Small_numerator % _Small_denominator;
      _Numerator._Myused = _Numerator._Mydata[0] > 0 ? 1u : 0u;
      return _Small_numerator / _Small_denominator;
    }

    if (_Small_denominator == 1) {
      uint64_t _Quotient = _Numerator._Mydata[1];
      _Quotient <<= 32;
      _Quotient |= _Numerator._Mydata[0];
      _Numerator._Myused = 0;
      return _Quotient;
    }

    // We count down in the next loop, so the last assignment to _Quotient will
    // be the correct one.
    uint64_t _Quotient = 0;

    uint64_t _Uu = 0;
    for (uint32_t _Iv = _Max_numerator_element_index;
         _Iv != static_cast<uint32_t>(-1); --_Iv) {
      _Uu = (_Uu << 32) | _Numerator._Mydata[_Iv];
      _Quotient =
          (_Quotient << 32) + static_cast<uint32_t>(_Uu / _Small_denominator);
      _Uu %= _Small_denominator;
    }

    _Numerator._Mydata[1] = static_cast<uint32_t>(_Uu >> 32);
    _Numerator._Mydata[0] = static_cast<uint32_t>(_Uu);
    _Numerator._Myused = _Numerator._Mydata[1] > 0 ? 2u : 1u;
    return _Quotient;
  }

  if (_Max_denominator_element_index > _Max_numerator_element_index) {
    return 0;
  }

  const uint32_t _Cu_den = _Max_denominator_element_index + 1;
  const int32_t _Cu_diff = static_cast<int32_t>(_Max_numerator_element_index -
                                                _Max_denominator_element_index);

  // Determine whether the result will have _Cu_diff or _Cu_diff + 1 digits:
  int32_t _Cu_quo = _Cu_diff;
  for (int32_t _Iu = static_cast<int32_t>(_Max_numerator_element_index);;
       --_Iu) {
    if (_Iu < _Cu_diff) {
      ++_Cu_quo;
      break;
    }

    if (_Denominator._Mydata[_Iu - _Cu_diff] != _Numerator._Mydata[_Iu]) {
      if (_Denominator._Mydata[_Iu - _Cu_diff] < _Numerator._Mydata[_Iu]) {
        ++_Cu_quo;
      }

      break;
    }
  }

  if (_Cu_quo == 0) {
    return 0;
  }

  // Get the uint to use for the trial divisions. We normalize so the high bit
  // is set:
  uint32_t _U_den = _Denominator._Mydata[_Cu_den - 1];
  uint32_t _U_den_next = _Denominator._Mydata[_Cu_den - 2];

  const uint32_t _Cbit_shift_left = _Count_sequential_high_zeroes(_U_den);
  const uint32_t _Cbit_shift_right = 32 - _Cbit_shift_left;
  if (_Cbit_shift_left > 0) {
    _U_den = (_U_den << _Cbit_shift_left) | (_U_den_next >> _Cbit_shift_right);
    _U_den_next <<= _Cbit_shift_left;

    if (_Cu_den > 2) {
      _U_den_next |= _Denominator._Mydata[_Cu_den - 3] >> _Cbit_shift_right;
    }
  }

  uint64_t _Quotient = 0;
  for (int32_t _Iu = _Cu_quo; --_Iu >= 0;) {
    // Get the high (normalized) bits of the _Numerator:
    const uint32_t _U_num_hi = (_Iu + _Cu_den <= _Max_numerator_element_index)
                                   ? _Numerator._Mydata[_Iu + _Cu_den]
                                   : 0;

    uint64_t _Uu_num =
        (static_cast<uint64_t>(_U_num_hi) << 32) |
        static_cast<uint64_t>(_Numerator._Mydata[_Iu + _Cu_den - 1]);

    uint32_t _U_num_next = _Numerator._Mydata[_Iu + _Cu_den - 2];
    if (_Cbit_shift_left > 0) {
      _Uu_num =
          (_Uu_num << _Cbit_shift_left) | (_U_num_next >> _Cbit_shift_right);
      _U_num_next <<= _Cbit_shift_left;

      if (_Iu + _Cu_den >= 3) {
        _U_num_next |=
            _Numerator._Mydata[_Iu + _Cu_den - 3] >> _Cbit_shift_right;
      }
    }

    // Divide to get the quotient digit:
    uint64_t _Uu_quo = _Uu_num / _U_den;
    uint64_t _Uu_rem = static_cast<uint32_t>(_Uu_num % _U_den);

    if (_Uu_quo > UINT32_MAX) {
      _Uu_rem += _U_den * (_Uu_quo - UINT32_MAX);
      _Uu_quo = UINT32_MAX;
    }

    while (_Uu_rem <= UINT32_MAX &&
           _Uu_quo * _U_den_next > ((_Uu_rem << 32) | _U_num_next)) {
      --_Uu_quo;
      _Uu_rem += _U_den;
    }

    // Multiply and subtract. Note that _Uu_quo may be one too large.
    // If we have a borrow at the end, we'll add the _Denominator back on and
    // decrement _Uu_quo.
    if (_Uu_quo > 0) {
      uint64_t _Uu_borrow = 0;

      for (uint32_t _Iu2 = 0; _Iu2 < _Cu_den; ++_Iu2) {
        _Uu_borrow += _Uu_quo * _Denominator._Mydata[_Iu2];

        const uint32_t _U_sub = static_cast<uint32_t>(_Uu_borrow);
        _Uu_borrow >>= 32;
        if (_Numerator._Mydata[_Iu + _Iu2] < _U_sub) {
          ++_Uu_borrow;
        }

        _Numerator._Mydata[_Iu + _Iu2] -= _U_sub;
      }

      if (_U_num_hi < _Uu_borrow) {
        // Add, tracking carry:
        uint32_t _U_carry = 0;
        for (uint32_t _Iu2 = 0; _Iu2 < _Cu_den; ++_Iu2) {
          const uint64_t _Sum =
              static_cast<uint64_t>(_Numerator._Mydata[_Iu + _Iu2]) +
              static_cast<uint64_t>(_Denominator._Mydata[_Iu2]) + _U_carry;

          _Numerator._Mydata[_Iu + _Iu2] = static_cast<uint32_t>(_Sum);
          _U_carry = static_cast<uint32_t>(_Sum >> 32);
        }

        --_Uu_quo;
      }

      _Max_numerator_element_index = _Iu + _Cu_den - 1;
    }

    _Quotient = (_Quotient << 32) + static_cast<uint32_t>(_Uu_quo);
  }

  // Trim the remainder:
  for (uint32_t _Ix = _Max_numerator_element_index + 1;
       _Ix < _Numerator._Myused; ++_Ix) {
    _Numerator._Mydata[_Ix] = 0;
  }

  uint32_t _Used = _Max_numerator_element_index + 1;

  while (_Used != 0 && _Numerator._Mydata[_Used - 1] == 0) {
    --_Used;
  }

  _Numerator._Myused = _Used;

  return _Quotient;
}

// ^^^^^^^^^^ DERIVED FROM corecrt_internal_big_integer.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM corecrt_internal_fltintrn.h vvvvvvvvvv

template <class _FloatingType> struct _Floating_type_traits;

template <> struct _Floating_type_traits<float> {
  static constexpr int32_t _Mantissa_bits = FLT_MANT_DIG;
  static constexpr int32_t _Exponent_bits =
      sizeof(float) * CHAR_BIT - FLT_MANT_DIG;

  static constexpr int32_t _Maximum_binary_exponent = FLT_MAX_EXP - 1;
  static constexpr int32_t _Minimum_binary_exponent = FLT_MIN_EXP - 1;

  static constexpr int32_t _Exponent_bias = 127;

  static constexpr int32_t _Sign_shift = _Exponent_bits + _Mantissa_bits - 1;
  static constexpr int32_t _Exponent_shift = _Mantissa_bits - 1;

  using _Uint_type = uint32_t;

  static constexpr uint32_t _Exponent_mask = (1u << _Exponent_bits) - 1;
  static constexpr uint32_t _Normal_mantissa_mask = (1u << _Mantissa_bits) - 1;
  static constexpr uint32_t _Denormal_mantissa_mask =
      (1u << (_Mantissa_bits - 1)) - 1;
  static constexpr uint32_t _Special_nan_mantissa_mask =
      1u << (_Mantissa_bits - 2);
  static constexpr uint32_t _Shifted_sign_mask = 1u << _Sign_shift;
  static constexpr uint32_t _Shifted_exponent_mask = _Exponent_mask
                                                     << _Exponent_shift;
};

template <> struct _Floating_type_traits<double> {
  static constexpr int32_t _Mantissa_bits = DBL_MANT_DIG;
  static constexpr int32_t _Exponent_bits =
      sizeof(double) * CHAR_BIT - DBL_MANT_DIG;

  static constexpr int32_t _Maximum_binary_exponent = DBL_MAX_EXP - 1;
  static constexpr int32_t _Minimum_binary_exponent = DBL_MIN_EXP - 1;

  static constexpr int32_t _Exponent_bias = 1023;

  static constexpr int32_t _Sign_shift = _Exponent_bits + _Mantissa_bits - 1;
  static constexpr int32_t _Exponent_shift = _Mantissa_bits - 1;

  using _Uint_type = uint64_t;

  static constexpr uint64_t _Exponent_mask = (1ULL << _Exponent_bits) - 1;
  static constexpr uint64_t _Normal_mantissa_mask =
      (1ULL << _Mantissa_bits) - 1;
  static constexpr uint64_t _Denormal_mantissa_mask =
      (1ULL << (_Mantissa_bits - 1)) - 1;
  static constexpr uint64_t _Special_nan_mantissa_mask =
      1ULL << (_Mantissa_bits - 2);
  static constexpr uint64_t _Shifted_sign_mask = 1ULL << _Sign_shift;
  static constexpr uint64_t _Shifted_exponent_mask = _Exponent_mask
                                                     << _Exponent_shift;
};

// ^^^^^^^^^^ DERIVED FROM corecrt_internal_fltintrn.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h vvvvvvvvvv

// This type is used to hold a partially-parsed string representation of a
// floating-point number. The number is stored in the following form:

// [sign] 0._Mymantissa * B^_Myexponent

// The _Mymantissa buffer stores the mantissa digits in big-endian, binary-coded
// decimal form. The _Mymantissa_count stores the number of digits present in
// the _Mymantissa buffer. The base B is not stored; it must be tracked
// separately. Note that the base of the mantissa digits may not be the same as
// B (e.g., for hexadecimal floating-point, the mantissa digits are in base 16
// but the exponent is a base 2 exponent).

// We consider up to 768 decimal digits during conversion. In most cases, we
// require nowhere near this many bits of precision to compute the correctly
// rounded binary floating-point value for the input string. 768 bits gives us
// room to handle the exact decimal representation of the smallest denormal
// value, 2^-1074 (752 decimal digits after trimming zeroes) with a bit of slack
// space.

// NOTE: The mantissa buffer count here must be kept in sync with the precision
// of the _Big_integer_flt type.
struct _Floating_point_string {
  bool _Myis_negative;
  int32_t _Myexponent;
  uint32_t _Mymantissa_count;
  uint8_t _Mymantissa[768];
};

// Stores a positive or negative zero into the _Result object
template <class _FloatingType>
void _Assemble_floating_point_zero(const bool _Is_negative,
                                   _FloatingType &_Result) noexcept {
  using _Floating_traits = _Floating_type_traits<_FloatingType>;
  using _Uint_type = typename _Floating_traits::_Uint_type;

  _Uint_type _Sign_component = _Is_negative;
  _Sign_component <<= _Floating_traits::_Sign_shift;

  _Result = _Bit_cast<_FloatingType>(_Sign_component);
}

// Stores a positive or negative infinity into the _Result object
template <class _FloatingType>
void _Assemble_floating_point_infinity(const bool _Is_negative,
                                       _FloatingType &_Result) noexcept {
  using _Floating_traits = _Floating_type_traits<_FloatingType>;
  using _Uint_type = typename _Floating_traits::_Uint_type;

  _Uint_type _Sign_component = _Is_negative;
  _Sign_component <<= _Floating_traits::_Sign_shift;

  const _Uint_type _Exponent_component =
      _Floating_traits::_Shifted_exponent_mask;

  _Result = _Bit_cast<_FloatingType>(_Sign_component | _Exponent_component);
}

// Determines whether a mantissa should be rounded up according to
// round_to_nearest given [1] the value of the least significant bit of the
// mantissa, [2] the value of the next bit after the least significant bit (the
// "round" bit) and [3] whether any trailing bits after the round bit are set.

// The mantissa is treated as an unsigned integer magnitude.

// For this function, "round up" is defined as "increase the magnitude" of the
// mantissa. (Note that this means that if we need to round a negative value to
// the next largest representable value, we return false, because the next
// largest representable value has a smaller magnitude.)
[[nodiscard]] inline bool _Should_round_up(const bool _Lsb_bit,
                                           const bool _Round_bit,
                                           const bool _Has_tail_bits) noexcept {
  // If there are no insignificant set bits, the value is exactly-representable
  // and should not be rounded. We could detect this with: const bool
  // _Is_exactly_representable = !_Round_bit && !_Has_tail_bits; if
  // (_Is_exactly_representable) { return false; } However, this is unnecessary
  // given the logic below.

  // If there are insignificant set bits, we need to round according to
  // round_to_nearest. We need to handle two cases: we round up if either [1]
  // the value is slightly greater than the midpoint between two
  // exactly-representable values or [2] the value is exactly the midpoint
  // between two exactly-representable values and the greater of the two is even
  // (this is "round-to-even").
  return _Round_bit && (_Has_tail_bits || _Lsb_bit);
}

// Computes _Value / 2^_Shift, then rounds the result according to
// round_to_nearest. By the time we call this function, we will already have
// discarded most digits. The caller must pass true for _Has_zero_tail if all
// discarded bits were zeroes.
[[nodiscard]] inline uint64_t
_Right_shift_with_rounding(const uint64_t _Value, const uint32_t _Shift,
                           const bool _Has_zero_tail) noexcept {
  // If we'd need to shift further than it is possible to shift, the answer is
  // always zero:
  if (_Shift >= 64) {
    return 0;
  }

  const uint64_t _Extra_bits_mask = (1ULL << (_Shift - 1)) - 1;
  const uint64_t _Round_bit_mask = (1ULL << (_Shift - 1));
  const uint64_t _Lsb_bit_mask = 1ULL << _Shift;

  const bool _Lsb_bit = (_Value & _Lsb_bit_mask) != 0;
  const bool _Round_bit = (_Value & _Round_bit_mask) != 0;
  const bool _Tail_bits = !_Has_zero_tail || (_Value & _Extra_bits_mask) != 0;

  return (_Value >> _Shift) +
         _Should_round_up(_Lsb_bit, _Round_bit, _Tail_bits);
}

// This is the result type of an attempt to parse a floating-point value from a
// string. It's currently ignored, but retained in case the from_chars()
// specification is changed to handle underflow and overflow in a special
// manner.
enum class _Floating_point_parse_status { _Ok, _Underflow, _Overflow };

// Converts the floating-point value [sign] 0.mantissa * 2^exponent into the
// correct form for _FloatingType and stores the result into the _Result object.
// The caller must ensure that the mantissa and exponent are correctly computed
// such that either [1] the most significant bit of the mantissa is in the
// correct position for the _FloatingType, or [2] the exponent has been
// correctly adjusted to account for the shift of the mantissa that will be
// required.

// This function correctly handles range errors and stores a zero or infinity in
// the _Result object on underflow and overflow errors, respectively. This
// function correctly forms denormal numbers when required.

// If the provided mantissa has more bits of precision than can be stored in the
// _Result object, the mantissa is rounded to the available precision. Thus, if
// possible, the caller should provide a mantissa with at least one more bit of
// precision than is required, to ensure that the mantissa is correctly rounded.
// (The caller should not round the mantissa before calling this function.)
template <class _FloatingType>
[[nodiscard]] _Floating_point_parse_status _Assemble_floating_point_value_t(
    const bool _Is_negative, const int32_t _Exponent,
    const typename _Floating_type_traits<_FloatingType>::_Uint_type _Mantissa,
    _FloatingType &_Result) noexcept {
  using _Floating_traits = _Floating_type_traits<_FloatingType>;
  using _Uint_type = typename _Floating_traits::_Uint_type;

  _Uint_type _Sign_component = _Is_negative;
  _Sign_component <<= _Floating_traits::_Sign_shift;

  _Uint_type _Exponent_component =
      static_cast<uint32_t>(_Exponent + _Floating_traits::_Exponent_bias);
  _Exponent_component <<= _Floating_traits::_Exponent_shift;

  _Result = _Bit_cast<_FloatingType>(_Sign_component | _Exponent_component |
                                     _Mantissa);

  return _Floating_point_parse_status::_Ok;
}

template <class _FloatingType>
[[nodiscard]] _Floating_point_parse_status _Assemble_floating_point_value(
    const uint64_t _Initial_mantissa, const int32_t _Initial_exponent,
    const bool _Is_negative, const bool _Has_zero_tail,
    _FloatingType &_Result) noexcept {
  using _Traits = _Floating_type_traits<_FloatingType>;

  // Assume that the number is representable as a normal value.
  // Compute the number of bits by which we must adjust the mantissa to shift it
  // into the correct position, and compute the resulting base two exponent for
  // the normalized mantissa:
  const uint32_t _Initial_mantissa_bits = _Bit_scan_reverse(_Initial_mantissa);
  const int32_t _Normal_mantissa_shift =
      static_cast<int32_t>(_Traits::_Mantissa_bits - _Initial_mantissa_bits);
  const int32_t _Normal_exponent = _Initial_exponent - _Normal_mantissa_shift;

  uint64_t _Mantissa = _Initial_mantissa;
  int32_t _Exponent = _Normal_exponent;

  if (_Normal_exponent > _Traits::_Maximum_binary_exponent) {
    // The exponent is too large to be represented by the floating-point type;
    // report the overflow condition:
    _Assemble_floating_point_infinity(_Is_negative, _Result);
    return _Floating_point_parse_status::_Overflow;
  }

  if (_Normal_exponent < _Traits::_Minimum_binary_exponent) {
    // The exponent is too small to be represented by the floating-point type as
    // a normal value, but it may be representable as a denormal value. Compute
    // the number of bits by which we need to shift the mantissa in order to
    // form a denormal number. (The subtraction of an extra 1 is to account for
    // the hidden bit of the mantissa that is not available for use when
    // representing a denormal.)
    const int32_t _Denormal_mantissa_shift =
        _Normal_mantissa_shift + _Normal_exponent + _Traits::_Exponent_bias - 1;

    // Denormal values have an exponent of zero, so the debiased exponent is the
    // negation of the exponent bias:
    _Exponent = -_Traits::_Exponent_bias;

    if (_Denormal_mantissa_shift < 0) {
      // Use two steps for right shifts: for a shift of N bits, we first shift
      // by N-1 bits, then shift the last bit and use its value to round the
      // mantissa.
      _Mantissa = _Right_shift_with_rounding(
          _Mantissa, static_cast<uint32_t>(-_Denormal_mantissa_shift),
          _Has_zero_tail);

      // If the mantissa is now zero, we have underflowed:
      if (_Mantissa == 0) {
        _Assemble_floating_point_zero(_Is_negative, _Result);
        return _Floating_point_parse_status::_Underflow;
      }

      // When we round the mantissa, the result may be so large that the number
      // becomes a normal value. For example, consider the single-precision case
      // where the mantissa is 0x01ffffff and a right shift of 2 is required to
      // shift the value into position. We perform the shift in two steps: we
      // shift by one bit, then we shift again and round using the dropped bit.
      // The initial shift yields 0x00ffffff. The rounding shift then yields
      // 0x007fffff and because the least significant bit was 1, we add 1 to
      // this number to round it. The final result is 0x00800000.

      // 0x00800000 is 24 bits, which is more than the 23 bits available in the
      // mantissa. Thus, we have rounded our denormal number into a normal
      // number.

      // We detect this case here and re-adjust the mantissa and exponent
      // appropriately, to form a normal number:
      if (_Mantissa > _Traits::_Denormal_mantissa_mask) {
        // We add one to the _Denormal_mantissa_shift to account for the hidden
        // mantissa bit (we subtracted one to account for this bit when we
        // computed the _Denormal_mantissa_shift above).
        _Exponent = _Initial_exponent - (_Denormal_mantissa_shift + 1) -
                    _Normal_mantissa_shift;
      }
    } else {
      _Mantissa <<= _Denormal_mantissa_shift;
    }
  } else {
    if (_Normal_mantissa_shift < 0) {
      // Use two steps for right shifts: for a shift of N bits, we first shift
      // by N-1 bits, then shift the last bit and use its value to round the
      // mantissa.
      _Mantissa = _Right_shift_with_rounding(
          _Mantissa, static_cast<uint32_t>(-_Normal_mantissa_shift),
          _Has_zero_tail);

      // When we round the mantissa, it may produce a result that is too large.
      // In this case, we divide the mantissa by two and increment the exponent
      // (this does not change the value).
      if (_Mantissa > _Traits::_Normal_mantissa_mask) {
        _Mantissa >>= 1;
        ++_Exponent;

        // The increment of the exponent may have generated a value too large to
        // be represented. In this case, report the overflow:
        if (_Exponent > _Traits::_Maximum_binary_exponent) {
          _Assemble_floating_point_infinity(_Is_negative, _Result);
          return _Floating_point_parse_status::_Overflow;
        }
      }
    } else if (_Normal_mantissa_shift > 0) {
      _Mantissa <<= _Normal_mantissa_shift;
    }
  }

  // Unset the hidden bit in the mantissa and assemble the floating-point value
  // from the computed components:
  _Mantissa &= _Traits::_Denormal_mantissa_mask;

  using _Uint_type = typename _Traits::_Uint_type;

  return _Assemble_floating_point_value_t(
      _Is_negative, _Exponent, static_cast<_Uint_type>(_Mantissa), _Result);
}

// This function is part of the fast track for integer floating-point strings.
// It takes an integer and a sign and converts the value into its _FloatingType
// representation, storing the result in the _Result object. If the value is not
// representable, +/-infinity is stored and overflow is reported (since this
// function deals with only integers, underflow is impossible).
template <class _FloatingType>
[[nodiscard]] _Floating_point_parse_status
_Assemble_floating_point_value_from_big_integer_flt(
    const _Big_integer_flt &_Integer_value,
    const uint32_t _Integer_bits_of_precision, const bool _Is_negative,
    const bool _Has_nonzero_fractional_part, _FloatingType &_Result) noexcept {
  using _Traits = _Floating_type_traits<_FloatingType>;

  const int32_t _Base_exponent = _Traits::_Mantissa_bits - 1;

  // Very fast case: If we have 64 bits of precision or fewer,
  // we can just take the two low order elements from the _Big_integer_flt:
  if (_Integer_bits_of_precision <= 64) {
    const int32_t _Exponent = _Base_exponent;

    const uint32_t _Mantissa_low =
        _Integer_value._Myused > 0 ? _Integer_value._Mydata[0] : 0;
    const uint32_t _Mantissa_high =
        _Integer_value._Myused > 1 ? _Integer_value._Mydata[1] : 0;
    const uint64_t _Mantissa =
        _Mantissa_low + (static_cast<uint64_t>(_Mantissa_high) << 32);

    return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative,
                                          !_Has_nonzero_fractional_part,
                                          _Result);
  }

  const uint32_t _Top_element_bits = _Integer_bits_of_precision % 32;
  const uint32_t _Top_element_index = _Integer_bits_of_precision / 32;

  const uint32_t _Middle_element_index = _Top_element_index - 1;
  const uint32_t _Bottom_element_index = _Top_element_index - 2;

  // Pretty fast case: If the top 64 bits occupy only two elements, we can just
  // combine those two elements:
  if (_Top_element_bits == 0) {
    const int32_t _Exponent =
        static_cast<int32_t>(_Base_exponent + _Bottom_element_index * 32);

    const uint64_t _Mantissa =
        _Integer_value._Mydata[_Bottom_element_index] +
        (static_cast<uint64_t>(_Integer_value._Mydata[_Middle_element_index])
         << 32);

    bool _Has_zero_tail = !_Has_nonzero_fractional_part;
    for (uint32_t _Ix = 0; _Has_zero_tail && _Ix != _Bottom_element_index;
         ++_Ix) {
      _Has_zero_tail = _Integer_value._Mydata[_Ix] == 0;
    }

    return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative,
                                          _Has_zero_tail, _Result);
  }

  // Not quite so fast case: The top 64 bits span three elements in the
  // _Big_integer_flt. Assemble the three pieces:
  const uint32_t _Top_element_mask = (1u << _Top_element_bits) - 1;
  const uint32_t _Top_element_shift = 64 - _Top_element_bits; // Left

  const uint32_t _Middle_element_shift = _Top_element_shift - 32; // Left

  const uint32_t _Bottom_element_bits = 32 - _Top_element_bits;
  const uint32_t _Bottom_element_mask = ~_Top_element_mask;
  const uint32_t _Bottom_element_shift = 32 - _Bottom_element_bits; // Right

  const int32_t _Exponent = static_cast<int32_t>(
      _Base_exponent + _Bottom_element_index * 32 + _Top_element_bits);

  const uint64_t _Mantissa =
      (static_cast<uint64_t>(_Integer_value._Mydata[_Top_element_index] &
                             _Top_element_mask)
       << _Top_element_shift) +
      (static_cast<uint64_t>(_Integer_value._Mydata[_Middle_element_index])
       << _Middle_element_shift) +
      (static_cast<uint64_t>(_Integer_value._Mydata[_Bottom_element_index] &
                             _Bottom_element_mask) >>
       _Bottom_element_shift);

  bool _Has_zero_tail =
      !_Has_nonzero_fractional_part &&
      (_Integer_value._Mydata[_Bottom_element_index] & _Top_element_mask) == 0;

  for (uint32_t _Ix = 0; _Has_zero_tail && _Ix != _Bottom_element_index;
       ++_Ix) {
    _Has_zero_tail = _Integer_value._Mydata[_Ix] == 0;
  }

  return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative,
                                        _Has_zero_tail, _Result);
}

// Accumulates the decimal digits in [_First_digit, _Last_digit) into the
// _Result high-precision integer. This function assumes that no overflow will
// occur.
inline void _Accumulate_decimal_digits_into_big_integer_flt(
    const uint8_t *const _First_digit, const uint8_t *const _Last_digit,
    _Big_integer_flt &_Result) noexcept {
  // We accumulate nine digit chunks, transforming the base ten string into base
  // one billion on the fly, allowing us to reduce the number of high-precision
  // multiplication and addition operations by 8/9.
  uint32_t _Accumulator = 0;
  uint32_t _Accumulator_count = 0;
  for (const uint8_t *_It = _First_digit; _It != _Last_digit; ++_It) {
    if (_Accumulator_count == 9) {
      (void)_Multiply(_Result, 1'000'000'000); // assumes no overflow
      (void)_Add(_Result, _Accumulator);       // assumes no overflow

      _Accumulator = 0;
      _Accumulator_count = 0;
    }

    _Accumulator *= 10;
    _Accumulator += *_It;
    ++_Accumulator_count;
  }

  if (_Accumulator_count != 0) {
    (void)_Multiply_by_power_of_ten(_Result,
                                    _Accumulator_count); // assumes no overflow
    (void)_Add(_Result, _Accumulator);                   // assumes no overflow
  }
}

// The core floating-point string parser for decimal strings. After a subject
// string is parsed and converted into a _Floating_point_string object, if the
// subject string was determined to be a decimal string, the object is passed to
// this function. This function converts the decimal real value to
// floating-point.
template <class _FloatingType>
[[nodiscard]] _Floating_point_parse_status
_Convert_decimal_string_to_floating_type(const _Floating_point_string &_Data,
                                         _FloatingType &_Result) noexcept {
  using _Traits = _Floating_type_traits<_FloatingType>;

  // To generate an N bit mantissa we require N + 1 bits of precision. The extra
  // bit is used to correctly round the mantissa (if there are fewer bits than
  // this available, then that's totally okay; in that case we use what we have
  // and we don't need to round).
  const uint32_t _Required_bits_of_precision =
      static_cast<uint32_t>(_Traits::_Mantissa_bits + 1);

  // The input is of the form 0.mantissa * 10^exponent, where 'mantissa' are the
  // decimal digits of the mantissa and 'exponent' is the decimal exponent. We
  // decompose the mantissa into two parts: an integer part and a fractional
  // part. If the exponent is positive, then the integer part consists of the
  // first 'exponent' digits, or all present digits if there are fewer digits.
  // If the exponent is zero or negative, then the integer part is empty. In
  // either case, the remaining digits form the fractional part of the mantissa.
  const uint32_t _Positive_exponent =
      static_cast<uint32_t>((std::max)(0, _Data._Myexponent));
  const uint32_t _Integer_digits_present =
      (std::min)(_Positive_exponent, _Data._Mymantissa_count);
  const uint32_t _Integer_digits_missing =
      _Positive_exponent - _Integer_digits_present;
  const uint8_t *const _Integer_first = _Data._Mymantissa;
  const uint8_t *const _Integer_last =
      _Data._Mymantissa + _Integer_digits_present;

  const uint8_t *const _Fractional_first = _Integer_last;
  const uint8_t *const _Fractional_last =
      _Data._Mymantissa + _Data._Mymantissa_count;
  const uint32_t _Fractional_digits_present =
      static_cast<uint32_t>(_Fractional_last - _Fractional_first);

  // First, we accumulate the integer part of the mantissa into a
  // _Big_integer_flt:
  _Big_integer_flt _Integer_value{};
  _Accumulate_decimal_digits_into_big_integer_flt(_Integer_first, _Integer_last,
                                                  _Integer_value);

  if (_Integer_digits_missing > 0) {
    if (!_Multiply_by_power_of_ten(_Integer_value, _Integer_digits_missing)) {
      _Assemble_floating_point_infinity(_Data._Myis_negative, _Result);
      return _Floating_point_parse_status::_Overflow;
    }
  }

  // At this point, the _Integer_value contains the value of the integer part of
  // the mantissa. If either [1] this number has more than the required number
  // of bits of precision or [2] the mantissa has no fractional part, then we
  // can assemble the result immediately:
  const uint32_t _Integer_bits_of_precision = _Bit_scan_reverse(_Integer_value);
  if (_Integer_bits_of_precision >= _Required_bits_of_precision ||
      _Fractional_digits_present == 0) {
    return _Assemble_floating_point_value_from_big_integer_flt(
        _Integer_value, _Integer_bits_of_precision, _Data._Myis_negative,
        _Fractional_digits_present != 0, _Result);
  }

  // Otherwise, we did not get enough bits of precision from the integer part,
  // and the mantissa has a fractional part. We parse the fractional part of the
  // mantissa to obtain more bits of precision. To do this, we convert the
  // fractional part into an actual fraction N/M, where the numerator N is
  // computed from the digits of the fractional part, and the denominator M is
  // computed as the power of 10 such that N/M is equal to the value of the
  // fractional part of the mantissa.
  _Big_integer_flt _Fractional_numerator{};
  _Accumulate_decimal_digits_into_big_integer_flt(
      _Fractional_first, _Fractional_last, _Fractional_numerator);

  const uint32_t _Fractional_denominator_exponent =
      _Data._Myexponent < 0 ? _Fractional_digits_present +
                                  static_cast<uint32_t>(-_Data._Myexponent)
                            : _Fractional_digits_present;

  _Big_integer_flt _Fractional_denominator = _Make_big_integer_flt_one();
  if (!_Multiply_by_power_of_ten(_Fractional_denominator,
                                 _Fractional_denominator_exponent)) {
    // If there were any digits in the integer part, it is impossible to
    // underflow (because the exponent cannot possibly be small enough), so if
    // we underflow here it is a true underflow and we return zero.
    _Assemble_floating_point_zero(_Data._Myis_negative, _Result);
    return _Floating_point_parse_status::_Underflow;
  }

  // Because we are using only the fractional part of the mantissa here, the
  // numerator is guaranteed to be smaller than the denominator. We normalize
  // the fraction such that the most significant bit of the numerator is in the
  // same position as the most significant bit in the denominator. This ensures
  // that when we later shift the numerator N bits to the left, we will produce
  // N bits of precision.
  const uint32_t _Fractional_numerator_bits =
      _Bit_scan_reverse(_Fractional_numerator);
  const uint32_t _Fractional_denominator_bits =
      _Bit_scan_reverse(_Fractional_denominator);

  const uint32_t _Fractional_shift =
      _Fractional_denominator_bits > _Fractional_numerator_bits
          ? _Fractional_denominator_bits - _Fractional_numerator_bits
          : 0;

  if (_Fractional_shift > 0) {
    (void)_Shift_left(_Fractional_numerator,
                      _Fractional_shift); // assumes no overflow
  }

  const uint32_t _Required_fractional_bits_of_precision =
      _Required_bits_of_precision - _Integer_bits_of_precision;

  uint32_t _Remaining_bits_of_precision_required =
      _Required_fractional_bits_of_precision;
  if (_Integer_bits_of_precision > 0) {
    // If the fractional part of the mantissa provides no bits of precision and
    // cannot affect rounding, we can just take whatever bits we got from the
    // integer part of the mantissa. This is the case for numbers
    // like 5.0000000000000000000001, where the significant digits of the
    // fractional part start so far to the right that they do not affect the
    // floating-point representation.

    // If the fractional shift is exactly equal to the number of bits of
    // precision that we require, then no fractional bits will be part of the
    // result, but the result may affect rounding. This is e.g. the case for
    // large, odd integers with a fractional part greater than or equal to .5.
    // Thus, we need to do the division to correctly round the result.
    if (_Fractional_shift > _Remaining_bits_of_precision_required) {
      return _Assemble_floating_point_value_from_big_integer_flt(
          _Integer_value, _Integer_bits_of_precision, _Data._Myis_negative,
          _Fractional_digits_present != 0, _Result);
    }

    _Remaining_bits_of_precision_required -= _Fractional_shift;
  }

  // If there was no integer part of the mantissa, we will need to compute the
  // exponent from the fractional part. The fractional exponent is the power of
  // two by which we must multiply the fractional part to move it into the range
  // [1.0, 2.0). This will either be the same as the shift we computed earlier,
  // or one greater than that shift:
  const uint32_t _Fractional_exponent =
      _Fractional_numerator < _Fractional_denominator ? _Fractional_shift + 1
                                                      : _Fractional_shift;

  (void)_Shift_left(
      _Fractional_numerator,
      _Remaining_bits_of_precision_required); // assumes no overflow
  uint64_t _Fractional_mantissa =
      _Divide(_Fractional_numerator, _Fractional_denominator);

  bool _Has_zero_tail = _Fractional_numerator._Myused == 0;

  // We may have produced more bits of precision than were required. Check, and
  // remove any "extra" bits:
  const uint32_t _Fractional_mantissa_bits =
      _Bit_scan_reverse(_Fractional_mantissa);
  if (_Fractional_mantissa_bits > _Required_fractional_bits_of_precision) {
    const uint32_t _Shift =
        _Fractional_mantissa_bits - _Required_fractional_bits_of_precision;
    _Has_zero_tail =
        _Has_zero_tail && (_Fractional_mantissa & ((1ULL << _Shift) - 1)) == 0;
    _Fractional_mantissa >>= _Shift;
  }

  // Compose the mantissa from the integer and fractional parts:
  const uint32_t _Integer_mantissa_low =
      _Integer_value._Myused > 0 ? _Integer_value._Mydata[0] : 0;
  const uint32_t _Integer_mantissa_high =
      _Integer_value._Myused > 1 ? _Integer_value._Mydata[1] : 0;
  const uint64_t _Integer_mantissa =
      _Integer_mantissa_low +
      (static_cast<uint64_t>(_Integer_mantissa_high) << 32);

  const uint64_t _Complete_mantissa =
      (_Integer_mantissa << _Required_fractional_bits_of_precision) +
      _Fractional_mantissa;

  // Compute the final exponent:
  // * If the mantissa had an integer part, then the exponent is one less than
  // the number of bits we obtained from the integer part. (It's one less
  // because we are converting to the form 1.11111, with one 1 to the left of
  // the decimal point.)
  // * If the mantissa had no integer part, then the exponent is the fractional
  // exponent that we computed. Then, in both cases, we subtract an additional
  // one from the exponent, to account for the fact that we've generated an
  // extra bit of precision, for use in rounding.
  const int32_t _Final_exponent =
      _Integer_bits_of_precision > 0
          ? static_cast<int32_t>(_Integer_bits_of_precision - 2)
          : -static_cast<int32_t>(_Fractional_exponent) - 1;

  return _Assemble_floating_point_value(_Complete_mantissa, _Final_exponent,
                                        _Data._Myis_negative, _Has_zero_tail,
                                        _Result);
}

template <class _FloatingType>
[[nodiscard]] _Floating_point_parse_status
_Convert_hexadecimal_string_to_floating_type(
    const _Floating_point_string &_Data, _FloatingType &_Result) noexcept {
  using _Traits = _Floating_type_traits<_FloatingType>;

  uint64_t _Mantissa = 0;
  int32_t _Exponent = _Data._Myexponent + _Traits::_Mantissa_bits - 1;

  // Accumulate bits into the mantissa buffer
  const uint8_t *const _Mantissa_last =
      _Data._Mymantissa + _Data._Mymantissa_count;
  const uint8_t *_Mantissa_it = _Data._Mymantissa;
  while (_Mantissa_it != _Mantissa_last &&
         _Mantissa <= _Traits::_Normal_mantissa_mask) {
    _Mantissa *= 16;
    _Mantissa += *_Mantissa_it++;
    _Exponent -= 4; // The exponent is in binary; log2(16) == 4
  }

  bool _Has_zero_tail = true;
  while (_Has_zero_tail && _Mantissa_it != _Mantissa_last) {
    _Has_zero_tail = *_Mantissa_it++ == 0;
  }

  return _Assemble_floating_point_value(
      _Mantissa, _Exponent, _Data._Myis_negative, _Has_zero_tail, _Result);
}

// ^^^^^^^^^^ DERIVED FROM corecrt_internal_strtox.h ^^^^^^^^^^

// FUNCTION from_chars (STRING TO FLOATING-POINT)

// C11 6.4.2.1 "General"
// digit: one of
//     0 1 2 3 4 5 6 7 8 9

// C11 6.4.4.1 "Integer constants"
// hexadecimal-digit: one of
//     0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F

// C11 6.4.4.2 "Floating constants" (without floating-suffix,
// hexadecimal-prefix) amended by C11 7.22.1.3 "The strtod, strtof, and strtold
// functions" making exponents optional LWG 3080: "the sign '+' may only appear
// in the exponent part"

// digit-sequence:
//     digit
//     digit-sequence digit

// hexadecimal-digit-sequence:
//     hexadecimal-digit
//     hexadecimal-digit-sequence hexadecimal-digit

// sign: one of
//     + -

// decimal-floating-constant:
//     fractional-constant exponent-part[opt]
//     digit-sequence exponent-part[opt]

// fractional-constant:
//     digit-sequence[opt] . digit-sequence
//     digit-sequence .

// exponent-part:
//     e sign[opt] digit-sequence
//     E sign[opt] digit-sequence

// hexadecimal-floating-constant:
//     hexadecimal-fractional-constant binary-exponent-part[opt]
//     hexadecimal-digit-sequence binary-exponent-part[opt]

// hexadecimal-fractional-constant:
//     hexadecimal-digit-sequence[opt] . hexadecimal-digit-sequence
//     hexadecimal-digit-sequence .

// binary-exponent-part:
//     p sign[opt] digit-sequence
//     P sign[opt] digit-sequence

template <class _Floating>
[[nodiscard]] inline from_chars_result
_Ordinary_floating_from_chars(const wchar_t *const _First,
                              const wchar_t *const _Last, _Floating &_Value,
                              const chars_format _Fmt, const bool _Minus_sign,
                              const wchar_t *_Next) noexcept { // strengthened
  // vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT
  // MODIFICATIONS vvvvvvvvvv

  const bool _Is_hexadecimal = _Fmt == chars_format::hex;
  const int _Base{_Is_hexadecimal ? 16 : 10};

  // PERFORMANCE NOTE: _Fp_string is intentionally left uninitialized.
  // Zero-initialization is quite expensive and is unnecessary. The benefit of
  // not zero-initializing is greatest for short inputs.
  _Floating_point_string _Fp_string;

  // Record the optional minus sign:
  _Fp_string._Myis_negative = _Minus_sign;

  uint8_t *const _Mantissa_first = _Fp_string._Mymantissa;
  uint8_t *const _Mantissa_last = std::end(_Fp_string._Mymantissa);
  uint8_t *_Mantissa_it = _Mantissa_first;

  // [_Whole_begin, _Whole_end) will contain 0 or more digits/hexits
  const wchar_t *const _Whole_begin = _Next;

  // Skip past any leading zeroes in the mantissa:
  for (; _Next != _Last && *_Next == '0'; ++_Next) {
  }
  const wchar_t *const _Leading_zero_end = _Next;

  // Scan the integer part of the mantissa:
  for (; _Next != _Last; ++_Next) {
    const unsigned char _Digit_value =
        _Digit_from_char(static_cast<unsigned char>(*_Next));

    if (_Digit_value >= _Base) {
      break;
    }

    if (_Mantissa_it != _Mantissa_last) {
      *_Mantissa_it++ = _Digit_value;
    }
  }
  const wchar_t *const _Whole_end = _Next;

  // Defend against _Exponent_adjustment integer overflow. (These values don't
  // need to be strict.)
  constexpr ptrdiff_t _Maximum_adjustment = 1'000'000;
  constexpr ptrdiff_t _Minimum_adjustment = -1'000'000;

  // The exponent adjustment holds the number of digits in the mantissa buffer
  // that appeared before the radix point. It can be negative, and leading
  // zeroes in the integer part are ignored. Examples: For "03333.111", it is 4.
  // For "00000.111", it is 0.
  // For "00000.001", it is -2.
  int _Exponent_adjustment = static_cast<int>(
      (std::min)(_Whole_end - _Leading_zero_end, _Maximum_adjustment));

  // [_Whole_end, _Dot_end) will contain 0 or 1 '.' characters
  if (_Next != _Last && *_Next == '.') {
    ++_Next;
  }
  const wchar_t *const _Dot_end = _Next;

  // [_Dot_end, _Frac_end) will contain 0 or more digits/hexits

  // If we haven't yet scanned any nonzero digits, continue skipping over
  // zeroes, updating the exponent adjustment to account for the zeroes we are
  // skipping:
  if (_Exponent_adjustment == 0) {
    for (; _Next != _Last && *_Next == '0'; ++_Next) {
    }

    _Exponent_adjustment =
        static_cast<int>((std::min)(_Dot_end - _Next, _Minimum_adjustment));
  }

  // Scan the fractional part of the mantissa:
  for (; _Next != _Last; ++_Next) {
    const unsigned char _Digit_value =
        _Digit_from_char(static_cast<unsigned char>(*_Next));

    if (_Digit_value >= _Base) {
      break;
    }

    if (_Mantissa_it != _Mantissa_last) {
      *_Mantissa_it++ = _Digit_value;
    }
  }
  const wchar_t *const _Frac_end = _Next;

  // We must have at least 1 digit/hexit
  if (_Whole_begin == _Whole_end && _Dot_end == _Frac_end) {
    return {_First, std::errc::invalid_argument};
  }

  const wchar_t _Exponent_prefix{_Is_hexadecimal ? L'p' : L'e'};

  bool _Exponent_is_negative = false;
  int _Exponent = 0;

  constexpr int _Maximum_temporary_decimal_exponent = 5200;
  constexpr int _Minimum_temporary_decimal_exponent = -5200;

  if (_Fmt != chars_format::fixed // N4713 23.20.3 [charconv.from.chars]/7.3
                                  // "if fmt has chars_format::fixed set but not
                                  // chars_format::scientific, the optional
                                  // exponent part shall not appear"
      && _Next != _Last &&
      (static_cast<unsigned char>(*_Next) | 0x20) ==
          _Exponent_prefix) { // found exponent prefix
    const wchar_t *_Unread = _Next + 1;

    if (_Unread != _Last &&
        (*_Unread == '+' || *_Unread == '-')) { // found optional sign
      _Exponent_is_negative = *_Unread == '-';
      ++_Unread;
    }

    while (_Unread != _Last) {
      const unsigned char _Digit_value =
          _Digit_from_char(static_cast<unsigned char>(*_Unread));

      if (_Digit_value >= 10) {
        break;
      }

      // found decimal digit

      if (_Exponent <= _Maximum_temporary_decimal_exponent) {
        _Exponent = _Exponent * 10 + _Digit_value;
      }

      ++_Unread;
      _Next = _Unread; // consume exponent-part/binary-exponent-part
    }

    if (_Exponent_is_negative) {
      _Exponent = -_Exponent;
    }
  }

  // [_Frac_end, _Exponent_end) will either be empty or contain "[EPep]
  // sign[opt] digit-sequence"
  const wchar_t *const _Exponent_end = _Next;

  if (_Fmt == chars_format::scientific &&
      _Frac_end ==
          _Exponent_end) { // N4713 23.20.3 [charconv.from.chars]/7.2
                           // "if fmt has chars_format::scientific set but not
                           // chars_format::fixed, the otherwise optional
                           // exponent part shall appear"
    return {_First, std::errc::invalid_argument};
  }

  // Remove trailing zeroes from mantissa:
  while (_Mantissa_it != _Mantissa_first && *(_Mantissa_it - 1) == 0) {
    --_Mantissa_it;
  }

  // If the mantissa buffer is empty, the mantissa was composed of all zeroes
  // (so the mantissa is 0). All such strings have the value zero, regardless of
  // what the exponent is (because 0 * b^n == 0 for all b and n). We can return
  // now. Note that we defer this check until after we scan the exponent, so
  // that we can correctly update _Next to point past the end of the exponent.
  if (_Mantissa_it == _Mantissa_first) {
    _Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
    return {_Next, std::errc{}}; // _Floating_point_parse_status::_Ok
  }

  // Before we adjust the exponent, handle the case where we detected a wildly
  // out of range exponent during parsing and clamped the value:
  if (_Exponent > _Maximum_temporary_decimal_exponent) {
    _Assemble_floating_point_infinity(_Fp_string._Myis_negative, _Value);
    return {_Next, std::errc{}}; // _Floating_point_parse_status::_Overflow
  }

  if (_Exponent < _Minimum_temporary_decimal_exponent) {
    _Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
    return {_Next, std::errc{}}; // _Floating_point_parse_status::_Underflow
  }

  // In hexadecimal floating constants, the exponent is a base 2 exponent. The
  // exponent adjustment computed during parsing has the same base as the
  // mantissa (so, 16 for hexadecimal floating constants). We therefore need to
  // scale the base 16 multiplier to base 2 by multiplying by log2(16):
  const int _Exponent_adjustment_multiplier{_Is_hexadecimal ? 4 : 1};

  _Exponent += _Exponent_adjustment * _Exponent_adjustment_multiplier;

  // Verify that after adjustment the exponent isn't wildly out of range (if it
  // is, it isn't representable in any supported floating-point format).
  if (_Exponent > _Maximum_temporary_decimal_exponent) {
    _Assemble_floating_point_infinity(_Fp_string._Myis_negative, _Value);
    return {_Next, std::errc{}}; // _Floating_point_parse_status::_Overflow
  }

  if (_Exponent < _Minimum_temporary_decimal_exponent) {
    _Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
    return {_Next, std::errc{}}; // _Floating_point_parse_status::_Underflow
  }

  _Fp_string._Myexponent = _Exponent;
  _Fp_string._Mymantissa_count =
      static_cast<uint32_t>(_Mantissa_it - _Mantissa_first);

  if (_Is_hexadecimal) {
    (void)_Convert_hexadecimal_string_to_floating_type(
        _Fp_string, _Value); // discard status
  } else {
    (void)_Convert_decimal_string_to_floating_type(_Fp_string,
                                                   _Value); // discard status
  }

  return {_Next, std::errc{}};

  // ^^^^^^^^^^ DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT
  // MODIFICATIONS ^^^^^^^^^^
}

[[nodiscard]] inline bool _Starts_with_case_insensitive(
    const wchar_t *_First, const wchar_t *const _Last,
    const wchar_t *_Lowercase) noexcept { // strengthened
  // pre: _Lowercase contains only ['a', 'z'] and is null-terminated
  for (; _First != _Last && *_Lowercase != '\0'; ++_First, ++_Lowercase) {
    if ((static_cast<unsigned char>(*_First) | 0x20) != *_Lowercase) {
      return false;
    }
  }

  return *_Lowercase == '\0';
}

template <class _Floating>
[[nodiscard]] inline from_chars_result
_Infinity_from_chars(const wchar_t *const _First, const wchar_t *const _Last,
                     _Floating &_Value, const bool _Minus_sign,
                     const wchar_t *_Next) noexcept { // strengthened
  // pre: _Next points at 'i' (case-insensitively)
  if (!_Starts_with_case_insensitive(_Next + 1, _Last,
                                     L"nf")) { // definitely invalid
    return {_First, std::errc::invalid_argument};
  }

  // definitely inf
  _Next += 3;

  if (_Starts_with_case_insensitive(_Next, _Last,
                                    L"inity")) { // definitely infinity
    _Next += 5;
  }

  _Assemble_floating_point_infinity(_Minus_sign, _Value);

  return {_Next, std::errc{}};
}

template <class _Floating>
[[nodiscard]] inline from_chars_result
_Nan_from_chars(const wchar_t *const _First, const wchar_t *const _Last,
                _Floating &_Value, const bool _Minus_sign,
                const wchar_t *_Next) noexcept { // strengthened
  // pre: _Next points at 'n' (case-insensitively)
  if (!_Starts_with_case_insensitive(_Next + 1, _Last,
                                     L"an")) { // definitely invalid
    return {_First, std::errc::invalid_argument};
  }

  // definitely nan
  _Next += 3;

  if constexpr (std::is_same_v<_Floating, float>) {
    _Value = __builtin_nanf("0"); // quiet NaN
  } else {
    _Value = __builtin_nan("0"); // quiet NaN
  }

  if (_Next != _Last && *_Next == '(') { // possibly nan(n-char-sequence[opt])
    const wchar_t *const _Seq_begin = _Next + 1;

    for (const wchar_t *_Temp = _Seq_begin; _Temp != _Last; ++_Temp) {
      if (*_Temp == ')') { // definitely nan(n-char-sequence[opt])
        _Next = _Temp + 1;

        if (_Temp - _Seq_begin == 3 &&
            _Starts_with_case_insensitive(_Seq_begin, _Temp,
                                          L"ind")) { // definitely nan(ind)
          _Value = -_Value; // indeterminate NaN is negative quiet NaN
        } else if (_Temp - _Seq_begin == 4 &&
                   _Starts_with_case_insensitive(
                       _Seq_begin, _Temp, L"snan")) { // definitely nan(snan)
          if constexpr (std::is_same_v<_Floating, float>) {
            _Value = __builtin_nansf("1"); // signaling NaN
          } else {
            _Value = __builtin_nans("1"); // signaling NaN
          }
        }

        break;
      } else if (*_Temp == '_' || ('0' <= *_Temp && *_Temp <= '9') ||
                 ('A' <= *_Temp && *_Temp <= 'Z') ||
                 ('a' <= *_Temp &&
                  *_Temp <=
                      'z')) { // possibly nan(n-char-sequence[opt]), keep going
      } else {                // definitely nan, not nan(n-char-sequence[opt])
        break;
      }
    }
  }

  if (_Minus_sign) {
    _Value = -_Value;
  }

  return {_Next, std::errc{}};
}

template <class _Floating>
[[nodiscard]] inline from_chars_result
_Floating_from_chars(const wchar_t *const _First, const wchar_t *const _Last,
                     _Floating &_Value,
                     const chars_format _Fmt) noexcept { // strengthened
  //_Adl_verify_range(_First, _Last);

  bool _Minus_sign = false;

  const wchar_t *_Next = _First;

  if (_Next == _Last) {
    return {_First, std::errc::invalid_argument};
  }

  if (*_Next == '-') {
    _Minus_sign = true;
    ++_Next;

    if (_Next == _Last) {
      return {_First, std::errc::invalid_argument};
    }
  }

  // Distinguish ordinary numbers versus inf/nan with a single test.
  // ordinary numbers start with ['.'] ['0', '9'] ['A', 'F'] ['a', 'f']
  // inf/nan start with ['I'] ['N'] ['i'] ['n']
  // All other starting characters are invalid.
  // Setting the 0x20 bit folds these ranges in a useful manner.
  // ordinary (and some invalid) starting characters are folded to ['.'] ['0',
  // '9'] ['a', 'f'] inf/nan starting characters are folded to ['i'] ['n'] These
  // are ordered: ['.'] ['0', '9'] ['a', 'f'] < ['i'] ['n'] Note that invalid
  // starting characters end up on both sides of this test.
  const unsigned char _Folded_start =
      static_cast<unsigned char>(static_cast<unsigned char>(*_Next) | 0x20);

  if (_Folded_start <= 'f') { // possibly an ordinary number
    return _Ordinary_floating_from_chars(_First, _Last, _Value, _Fmt,
                                         _Minus_sign, _Next);
  } else if (_Folded_start == 'i') { // possibly inf
    return _Infinity_from_chars(_First, _Last, _Value, _Minus_sign, _Next);
  } else if (_Folded_start == 'n') { // possibly nan
    return _Nan_from_chars(_First, _Last, _Value, _Minus_sign, _Next);
  } else { // definitely invalid
    return {_First, std::errc::invalid_argument};
  }
}

inline from_chars_result from_chars(
    const wchar_t *const _First, const wchar_t *const _Last, float &_Value,
    const chars_format _Fmt = chars_format::general) noexcept { // strengthened
  return _Floating_from_chars(_First, _Last, _Value, _Fmt);
}
inline from_chars_result from_chars(
    const wchar_t *const _First, const wchar_t *const _Last, double &_Value,
    const chars_format _Fmt = chars_format::general) noexcept { // strengthened
  return _Floating_from_chars(_First, _Last, _Value, _Fmt);
}
inline from_chars_result from_chars(
    const wchar_t *const _First, const wchar_t *const _Last,
    long double &_Value,
    const chars_format _Fmt = chars_format::general) noexcept { // strengthened
  double _Dbl; // intentionally default-init
  const auto _Result = _Floating_from_chars(_First, _Last, _Dbl, _Fmt);

  if (_Result.ec == std::errc{}) {
    _Value = _Dbl;
  }

  return _Result;
}

// FUNCTION to_chars (FLOATING-POINT TO STRING)

template <class _Floating>
[[nodiscard]] inline to_chars_result
_Floating_to_chars_hex_precision(wchar_t *_First, wchar_t *const _Last,
                                 const _Floating _Value,
                                 int _Precision) noexcept { // strengthened

  // * Determine the effective _Precision.
  // * Later, we'll decrement _Precision when printing each hexit after the
  // decimal point.

  // The hexits after the decimal point correspond to the explicitly stored
  // fraction bits. float explicitly stores 23 fraction bits. 23 / 4 == 5.75,
  // which is 6 hexits. double explicitly stores 52 fraction bits. 52 / 4 == 13,
  // which is 13 hexits.
  constexpr int _Full_precision = std::is_same_v<_Floating, float> ? 6 : 13;
  constexpr int _Adjusted_explicit_bits = _Full_precision * 4;

  if (_Precision < 0) {
    // C11 7.21.6.1 "The fprintf function"/5: "A negative precision argument is
    // taken as if the precision were omitted." /8: "if the precision is missing
    // and FLT_RADIX is a power of 2, then the precision is sufficient for an
    // exact representation of the value"
    _Precision = _Full_precision;
  }

  // * Extract the _Ieee_mantissa and _Ieee_exponent.
  using _Traits = _Floating_type_traits<_Floating>;
  using _Uint_type = typename _Traits::_Uint_type;

  const _Uint_type _Uint_value = _Bit_cast<_Uint_type>(_Value);
  const _Uint_type _Ieee_mantissa =
      _Uint_value & _Traits::_Denormal_mantissa_mask;
  const int32_t _Ieee_exponent =
      static_cast<int32_t>(_Uint_value >> _Traits::_Exponent_shift);

  // * Prepare the _Adjusted_mantissa. This is aligned to hexit boundaries,
  // * with the implicit bit restored (0 for zero values and subnormal values, 1
  // for normal values).
  // * Also calculate the _Unbiased_exponent. This unifies the processing of
  // zero, subnormal, and normal values.
  _Uint_type _Adjusted_mantissa;

  if constexpr (std::is_same_v<_Floating, float>) {
    _Adjusted_mantissa =
        _Ieee_mantissa
        << 1; // align to hexit boundary (23 isn't divisible by 4)
  } else {
    _Adjusted_mantissa =
        _Ieee_mantissa; // already aligned (52 is divisible by 4)
  }

  int32_t _Unbiased_exponent;

  if (_Ieee_exponent == 0) { // zero or subnormal
    // implicit bit is 0

    if (_Ieee_mantissa == 0) { // zero
      // C11 7.21.6.1 "The fprintf function"/8: "If the value is zero, the
      // exponent is zero."
      _Unbiased_exponent = 0;
    } else { // subnormal
      _Unbiased_exponent = 1 - _Traits::_Exponent_bias;
    }
  } else { // normal
    _Adjusted_mantissa |= _Uint_type{1}
                          << _Adjusted_explicit_bits; // implicit bit is 1
    _Unbiased_exponent = _Ieee_exponent - _Traits::_Exponent_bias;
  }

  // _Unbiased_exponent is within [-126, 127] for float, [-1022, 1023] for
  // double.

  // * Decompose _Unbiased_exponent into _Sign_character and _Absolute_exponent.
  char _Sign_character;
  uint32_t _Absolute_exponent;

  if (_Unbiased_exponent < 0) {
    _Sign_character = '-';
    _Absolute_exponent = static_cast<uint32_t>(-_Unbiased_exponent);
  } else {
    _Sign_character = '+';
    _Absolute_exponent = static_cast<uint32_t>(_Unbiased_exponent);
  }

  // _Absolute_exponent is within [0, 127] for float, [0, 1023] for double.

  // * Perform a single bounds check.
  {
    int32_t _Exponent_length;

    if (_Absolute_exponent < 10) {
      _Exponent_length = 1;
    } else if (_Absolute_exponent < 100) {
      _Exponent_length = 2;
#ifdef __EDG__ // TRANSITION, VSO#731759
    } else if (std::is_same_v<_Floating, float>) {
#else  // ^^^ workaround ^^^ / vvv no workaround vvv
    } else if constexpr (std::is_same_v<_Floating, float>) {
#endif // ^^^ no workaround ^^^
      _Exponent_length = 3;
    } else if (_Absolute_exponent < 1000) {
      _Exponent_length = 3;
    } else {
      _Exponent_length = 4;
    }

    // _Precision might be enormous; avoid integer overflow by testing it
    // separately.
    ptrdiff_t _Buffer_size = _Last - _First;

    if (_Buffer_size < _Precision) {
      return {_Last, std::errc::value_too_large};
    }

    _Buffer_size -= _Precision;

    const int32_t _Length_excluding_precision =
        1                                      // leading hexit
        + static_cast<int32_t>(_Precision > 0) // possible decimal point
        // excluding `+ _Precision`, hexits after decimal point
        + 2                 // "p+" or "p-"
        + _Exponent_length; // exponent

    if (_Buffer_size < _Length_excluding_precision) {
      return {_Last, std::errc::value_too_large};
    }
  }

  // * Perform rounding when we've been asked to omit hexits.
  if (_Precision < _Full_precision) {
    // _Precision is within [0, 5] for float, [0, 12] for double.

    // _Dropped_bits is within [4, 24] for float, [4, 52] for double.
    const int _Dropped_bits = (_Full_precision - _Precision) * 4;

    // Perform rounding by adding an appropriately-shifted bit.

    // This can propagate carries all the way into the leading hexit. Examples:
    // "0.ff9" rounded to a precision of 2 is "1.00".
    // "1.ff9" rounded to a precision of 2 is "2.00".

    // Note that the leading hexit participates in the rounding decision.
    // Examples: "0.8" rounded to a precision of 0 is "0". "1.8" rounded to a
    // precision of 0 is "2".

    // Reference implementation with suboptimal codegen:
    // const bool _Lsb_bit       = (_Adjusted_mantissa & (_Uint_type{1} <<
    // _Dropped_bits)) != 0; const bool _Round_bit     = (_Adjusted_mantissa &
    // (_Uint_type{1} << (_Dropped_bits - 1))) != 0; const bool _Has_tail_bits =
    // (_Adjusted_mantissa & ((_Uint_type{1} << (_Dropped_bits - 1)) - 1)) != 0;
    // const bool _Should_round = _Should_round_up(_Lsb_bit, _Round_bit,
    // _Has_tail_bits); _Adjusted_mantissa += _Uint_type{_Should_round} <<
    // _Dropped_bits;

    // Example for optimized implementation: Let _Dropped_bits be 8.
    //          Bit index: ...[8]76543210
    // _Adjusted_mantissa: ...[L]RTTTTTTT (not depicting known details, like
    // hexit alignment) By focusing on the bit at index _Dropped_bits, we can
    // avoid unnecessary branching and shifting.

    // Bit index: ...[8]76543210
    //  _Lsb_bit: ...[L]RTTTTTTT
    const _Uint_type _Lsb_bit = _Adjusted_mantissa;

    //  Bit index: ...9[8]76543210
    // _Round_bit: ...L[R]TTTTTTT0
    const _Uint_type _Round_bit = _Adjusted_mantissa << 1;

    // We can detect (without branching) whether any of the trailing bits are
    // set. Due to _Should_round below, this computation will be used if and
    // only if R is 1, so we can assume that here.
    //      Bit index: ...9[8]76543210
    //     _Round_bit: ...L[1]TTTTTTT0
    // _Has_tail_bits: ....[H]........

    // If all of the trailing bits T are 0, then `_Round_bit - 1` will produce 0
    // for H (due to R being 1). If any of the trailing bits T are 1, then
    // `_Round_bit - 1` will produce 1 for H (due to R being 1).
    const _Uint_type _Has_tail_bits = _Round_bit - 1;

    // Finally, we can use _Should_round_up() logic with bitwise-AND and
    // bitwise-OR, selecting just the bit at index _Dropped_bits. This is the
    // appropriately-shifted bit that we want.
    const _Uint_type _Should_round = _Round_bit & (_Has_tail_bits | _Lsb_bit) &
                                     (_Uint_type{1} << _Dropped_bits);

    _Adjusted_mantissa += _Should_round;
  }

  // * Print the leading hexit, then mask it away.
  {
    const uint32_t _Nibble =
        static_cast<uint32_t>(_Adjusted_mantissa >> _Adjusted_explicit_bits);
    assert(_Nibble < 3);
    const wchar_t _Leading_hexit = static_cast<wchar_t>('0' + _Nibble);

    *_First++ = _Leading_hexit;

    constexpr _Uint_type _Mask = (_Uint_type{1} << _Adjusted_explicit_bits) - 1;
    _Adjusted_mantissa &= _Mask;
  }

  // * Print the decimal point and trailing hexits.

  // C11 7.21.6.1 "The fprintf function"/8:
  // "if the precision is zero and the # flag is not specified, no decimal-point
  // character appears."
  if (_Precision > 0) {
    *_First++ = '.';

    int32_t _Number_of_bits_remaining =
        _Adjusted_explicit_bits; // 24 for float, 52 for double

    for (;;) {
      assert(_Number_of_bits_remaining >= 4);
      assert(_Number_of_bits_remaining % 4 == 0);
      _Number_of_bits_remaining -= 4;

      const uint32_t _Nibble = static_cast<uint32_t>(_Adjusted_mantissa >>
                                                     _Number_of_bits_remaining);
      assert(_Nibble < 16);
      const wchar_t _Hexit = _Charconv_digits[_Nibble];

      *_First++ = _Hexit;

      // _Precision is the number of hexits that still need to be printed.
      --_Precision;
      if (_Precision == 0) {
        break; // We're completely done with this phase.
      }
      // Otherwise, we need to keep printing hexits.

      if (_Number_of_bits_remaining == 0) {
        // We've finished printing _Adjusted_mantissa, so all remaining hexits
        // are '0'.
        wmemset(_First, '0', static_cast<size_t>(_Precision));
        _First += _Precision;
        break;
      }

      // Mask away the hexit that we just printed, then keep looping.
      // (We skip this when breaking out of the loop above, because
      // _Adjusted_mantissa isn't used later.)
      const _Uint_type _Mask = (_Uint_type{1} << _Number_of_bits_remaining) - 1;
      _Adjusted_mantissa &= _Mask;
    }
  }

  // * Print the exponent.

  // C11 7.21.6.1 "The fprintf function"/8: "The exponent always contains at
  // least one digit, and only as many more digits as necessary to represent the
  // decimal exponent of 2."

  // Performance note: We should take advantage of the known ranges of possible
  // exponents.

  *_First++ = 'p';
  *_First++ = _Sign_character;

  // We've already printed '-' if necessary, so uint32_t _Absolute_exponent
  // avoids testing that again.
  return to_chars(_First, _Last, _Absolute_exponent);
}

template <class _Floating>
[[nodiscard]] inline to_chars_result _Floating_to_chars_hex_shortest(
    wchar_t *_First, wchar_t *const _Last,
    const _Floating _Value) noexcept { // strengthened

  // This prints "1.728p+0" instead of "2.e5p-1".
  // This prints "0.000002p-126" instead of "1p-149" for float.
  // This prints "0.0000000000001p-1022" instead of "1p-1074" for double.
  // This prioritizes being consistent with printf's de facto behavior (and
  // hex-precision's behavior) over minimizing the number of characters printed.

  using _Traits = _Floating_type_traits<_Floating>;
  using _Uint_type = typename _Traits::_Uint_type;

  const _Uint_type _Uint_value = _Bit_cast<_Uint_type>(_Value);

  if (_Uint_value == 0) { // zero detected; write "0p+0" and return
    // C11 7.21.6.1 "The fprintf function"/8: "If the value is zero, the
    // exponent is zero." Special-casing zero is necessary because of the
    // exponent.
    const wchar_t *const _Str = L"0p+0";
    const size_t _Len = 4;

    if (_Last - _First < static_cast<ptrdiff_t>(_Len)) {
      return {_Last, std::errc::value_too_large};
    }

    ArrayCopy(_First, _Str, _Len);

    return {_First + _Len, std::errc{}};
  }

  const _Uint_type _Ieee_mantissa =
      _Uint_value & _Traits::_Denormal_mantissa_mask;
  const int32_t _Ieee_exponent =
      static_cast<int32_t>(_Uint_value >> _Traits::_Exponent_shift);

  char _Leading_hexit; // implicit bit
  int32_t _Unbiased_exponent;

  if (_Ieee_exponent == 0) { // subnormal
    _Leading_hexit = '0';
    _Unbiased_exponent = 1 - _Traits::_Exponent_bias;
  } else { // normal
    _Leading_hexit = '1';
    _Unbiased_exponent = _Ieee_exponent - _Traits::_Exponent_bias;
  }

  // Performance note: Consider avoiding per-character bounds checking when
  // there's plenty of space.

  if (_First == _Last) {
    return {_Last, std::errc::value_too_large};
  }

  *_First++ = _Leading_hexit;

  if (_Ieee_mantissa == 0) {
    // The fraction bits are all 0. Trim them away, including the decimal point.
  } else {
    if (_First == _Last) {
      return {_Last, std::errc::value_too_large};
    }

    *_First++ = '.';

    // The hexits after the decimal point correspond to the explicitly stored
    // fraction bits. float explicitly stores 23 fraction bits. 23 / 4 == 5.75,
    // so we'll print at most 6 hexits. double explicitly stores 52 fraction
    // bits. 52 / 4 == 13, so we'll print at most 13 hexits.
    _Uint_type _Adjusted_mantissa;
    int32_t _Number_of_bits_remaining;

    if constexpr (std::is_same_v<_Floating, float>) {
      _Adjusted_mantissa =
          _Ieee_mantissa
          << 1; // align to hexit boundary (23 isn't divisible by 4)
      _Number_of_bits_remaining = 24; // 23 fraction bits + 1 alignment bit
    } else {
      _Adjusted_mantissa =
          _Ieee_mantissa;             // already aligned (52 is divisible by 4)
      _Number_of_bits_remaining = 52; // 52 fraction bits
    }

    // do-while: The condition _Adjusted_mantissa != 0 is initially true - we
    // have nonzero fraction bits and we've printed the decimal point. Each
    // iteration, we print a hexit, mask it away, and keep looping if we still
    // have nonzero fraction bits. If there would be trailing '0' hexits, this
    // trims them. If there wouldn't be trailing '0' hexits, the same condition
    // works (as we print the final hexit and mask it away); we don't need to
    // test _Number_of_bits_remaining.
    do {
      assert(_Number_of_bits_remaining >= 4);
      assert(_Number_of_bits_remaining % 4 == 0);
      _Number_of_bits_remaining -= 4;

      const uint32_t _Nibble = static_cast<uint32_t>(_Adjusted_mantissa >>
                                                     _Number_of_bits_remaining);
      assert(_Nibble < 16);
      const wchar_t _Hexit = _Charconv_digits[_Nibble];

      if (_First == _Last) {
        return {_Last, std::errc::value_too_large};
      }

      *_First++ = _Hexit;

      const _Uint_type _Mask = (_Uint_type{1} << _Number_of_bits_remaining) - 1;
      _Adjusted_mantissa &= _Mask;

    } while (_Adjusted_mantissa != 0);
  }

  // C11 7.21.6.1 "The fprintf function"/8: "The exponent always contains at
  // least one digit, and only as many more digits as necessary to represent the
  // decimal exponent of 2."

  // Performance note: We should take advantage of the known ranges of possible
  // exponents.

  // float: _Unbiased_exponent is within [-126, 127].
  // double: _Unbiased_exponent is within [-1022, 1023].

  if (_Last - _First < 2) {
    return {_Last, std::errc::value_too_large};
  }

  *_First++ = 'p';

  if (_Unbiased_exponent < 0) {
    *_First++ = '-';
    _Unbiased_exponent = -_Unbiased_exponent;
  } else {
    *_First++ = '+';
  }

  // We've already printed '-' if necessary, so static_cast<uint32_t> avoids
  // testing that again.
  return to_chars(_First, _Last, static_cast<uint32_t>(_Unbiased_exponent));
}

enum class _Floating_to_chars_overload {
  _Plain,
  _Format_only,
  _Format_precision
};

template <_Floating_to_chars_overload _Overload, class _Floating>
[[nodiscard]] inline to_chars_result
_Floating_to_chars(wchar_t *_First, wchar_t *const _Last, _Floating _Value,
                   const chars_format _Fmt,
                   const int _Precision) noexcept { // strengthened

  using _Traits = _Floating_type_traits<_Floating>;
  using _Uint_type = typename _Traits::_Uint_type;

  _Uint_type _Uint_value = _Bit_cast<_Uint_type>(_Value);

  if ((_Uint_value & _Traits::_Shifted_sign_mask) !=
      0) { // sign bit detected; write minus sign and clear sign bit
    if (_First == _Last) {
      return {_Last, std::errc::value_too_large};
    }

    *_First++ = '-';

    _Uint_value &= ~_Traits::_Shifted_sign_mask;
    _Value = _Bit_cast<_Floating>(_Uint_value);
  }

  if ((_Uint_value & _Traits::_Shifted_exponent_mask) ==
      _Traits::_Shifted_exponent_mask) { // inf/nan detected; write appropriate
                                         // string and return
    const wchar_t *_Str;
    size_t _Len;

    if ((_Uint_value & _Traits::_Denormal_mantissa_mask) == 0) {
      _Str = L"inf";
      _Len = 3;
    } else if ((_Uint_value & _Traits::_Special_nan_mantissa_mask) != 0) {
      _Str = L"nan";
      _Len = 3;
    } else {
      _Str = L"nan(snan)";
      _Len = 9;
    }

    if (_Last - _First < static_cast<ptrdiff_t>(_Len)) {
      return {_Last, std::errc::value_too_large};
    }

    ArrayCopy(_First, _Str, _Len);

    return {_First + _Len, std::errc{}};
  }

  if constexpr (_Overload == _Floating_to_chars_overload::_Plain) {
    (void)_Fmt;
    (void)_Precision;

    return _Floating_to_chars_ryu(_First, _Last, _Value, chars_format{});
  } else if constexpr (_Overload == _Floating_to_chars_overload::_Format_only) {
    (void)_Precision;

    if (_Fmt == chars_format::hex) {
      return _Floating_to_chars_hex_shortest(_First, _Last, _Value);
    }

    return _Floating_to_chars_ryu(_First, _Last, _Value, _Fmt);
  } else if constexpr (_Overload ==
                       _Floating_to_chars_overload::_Format_precision) {
    switch (_Fmt) {
#if _HAS_COMPLETE_CHARCONV
    case chars_format::scientific:
      return _Floating_to_chars_scientific_precision(_First, _Last, _Value,
                                                     _Precision);
    case chars_format::fixed:
      return _Floating_to_chars_fixed_precision(_First, _Last, _Value,
                                                _Precision);
    case chars_format::general:
      return _Floating_to_chars_general_precision(_First, _Last, _Value,
                                                  _Precision);
#else  // _HAS_COMPLETE_CHARCONV
    case chars_format::scientific:
    case chars_format::fixed:
    case chars_format::general:
      return {_Last, std::errc::value_too_large}; // NOT YET IMPLEMENTED
#endif // _HAS_COMPLETE_CHARCONV
    case chars_format::hex:
    default: // avoid warning C4715: not all control paths return a value
      return _Floating_to_chars_hex_precision(_First, _Last, _Value,
                                              _Precision);
    }
  }
}

inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const float _Value) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(
      _First, _Last, _Value, chars_format{}, 0);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const double _Value) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(
      _First, _Last, _Value, chars_format{}, 0);
}
inline to_chars_result
to_chars(wchar_t *const _First, wchar_t *const _Last,
         const long double _Value) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(
      _First, _Last, static_cast<double>(_Value), chars_format{}, 0);
}
inline to_chars_result
to_chars(wchar_t *const _First, wchar_t *const _Last, const float _Value,
         const chars_format _Fmt) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(
      _First, _Last, _Value, _Fmt, 0);
}
inline to_chars_result
to_chars(wchar_t *const _First, wchar_t *const _Last, const double _Value,
         const chars_format _Fmt) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(
      _First, _Last, _Value, _Fmt, 0);
}
inline to_chars_result
to_chars(wchar_t *const _First, wchar_t *const _Last, const long double _Value,
         const chars_format _Fmt) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(
      _First, _Last, static_cast<double>(_Value), _Fmt, 0);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const float _Value, const chars_format _Fmt,
                                const int _Precision) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      _First, _Last, _Value, _Fmt, _Precision);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const double _Value, const chars_format _Fmt,
                                const int _Precision) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      _First, _Last, _Value, _Fmt, _Precision);
}
inline to_chars_result to_chars(wchar_t *const _First, wchar_t *const _Last,
                                const long double _Value,
                                const chars_format _Fmt,
                                const int _Precision) noexcept { // strengthened
  return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(
      _First, _Last, static_cast<double>(_Value), _Fmt, _Precision);
}

} // namespace bela

#endif
