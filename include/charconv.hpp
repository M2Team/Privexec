//////
#ifndef CLANGBUILDER_CHARCONV_HPP
#define CLANGBUILDER_CHARCONV_HPP

#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <system_error>

namespace base {
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