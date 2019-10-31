// Thanks Neargye/semver
// origin code see
// https://github.com/Neargye/semver/blob/master/include/semver.hpp
// support wchar_t
#ifndef BELA_SENVER_HPP
#define BELA_SENVER_HPP

#include <cstdint>
#include <cstddef>
#include <limits>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include "strcat.hpp"
#include "narrow/strcat.hpp"

// Allow to disable exceptions.
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) ||                     \
     defined(_CPPUNWIND)) &&                                                   \
    !defined(SEMVER_NOEXCEPTION)
#include <stdexcept>
#define SEMVER_THROW(exception) throw exception
#else
#include <cstdlib>
#define SEMVER_THROW(exception) std::abort()
#endif

namespace bela::semver {
enum struct prerelease : std::uint8_t {
  alpha = 0, // alpha
  beta = 1,
  rc = 2,
  none = 3
};
namespace detail {

template <typename T> class Literal;
template <> class Literal<char> {
public:
  static constexpr std::string_view Alpha{"-alpha"};
  static constexpr std::string_view Beta{"-beta"};
  static constexpr std::string_view RC{"-rc"};
};
template <> class Literal<wchar_t> {
public:
  static constexpr std::wstring_view Alpha{L"-alpha"};
  static constexpr std::wstring_view Beta{L"-beta"};
  static constexpr std::wstring_view RC{L"-rc"};
};
template <> class Literal<char16_t> {
public:
  static constexpr std::u16string_view Alpha{u"-alpha"};
  static constexpr std::u16string_view Beta{u"-beta"};
  static constexpr std::u16string_view RC{u"-rc"};
};

constexpr char char_to_lower(char c) noexcept {
  return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

template <typename T>
constexpr bool str_equals(std::basic_string_view<T> lhs, std::size_t pos,
                          std::basic_string_view<T> rhs) noexcept {
  for (std::size_t i1 = pos, i2 = 0; i1 < lhs.length() && i2 < rhs.length();
       ++i1, ++i2) {
    if (char_to_lower(static_cast<char>(lhs[i1])) !=
        char_to_lower(static_cast<char>(rhs[i2]))) {
      return false;
    }
  }
  return true;
}

constexpr bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }
constexpr std::uint8_t char_to_digit(char c) noexcept { return c - '0'; }

template <typename T, typename I>
constexpr bool read_uint(std::basic_string_view<T> str, std::size_t &i,
                         I &d) noexcept {
  if (std::uint32_t t = 0;
      i < str.length() && is_digit(static_cast<char>(str[i]))) {
    for (std::size_t p = i, s = 0; p < str.length() && s < 3; ++p, ++s) {
      auto c = static_cast<char>(str[p]);
      if (is_digit(c)) {
        t = t * 10 + char_to_digit(c);
        ++i;
      } else {
        break;
      }
    }
    if (t <= std::numeric_limits<I>::max()) {
      d = static_cast<I>(t);
      return true;
    }
  }

  return false;
}

template <typename T>
constexpr bool read_dot(std::basic_string_view<T> str,
                        std::size_t &i) noexcept {
  if (i < str.length() && str[i] == '.') {
    ++i;
    return true;
  }
  return false;
}

template <typename T>
constexpr bool read_prerelease(std::basic_string_view<T> str, std::size_t &i,
                               prerelease &p) noexcept {
  if (i >= str.length()) {
    p = prerelease::none;
    return false;
  } else if (str_equals(str, i, Literal<T>::Alpha)) {
    i += Literal<T>::Alpha.length();
    p = prerelease::alpha;
    return true;
  } else if (str_equals(str, i, Literal<T>::Beta)) {
    i += Literal<T>::Beta.length();
    p = prerelease::beta;
    return true;
  } else if (str_equals(str, i, Literal<T>::RC)) {
    i += Literal<T>::RC.length();
    p = prerelease::rc;
    return true;
  }
  return false;
}
} // namespace detail

struct alignas(1) version {
  std::uint8_t major = 0;
  std::uint8_t minor = 1;
  std::uint8_t patch = 0;
  prerelease prerelease_type = prerelease::none;
  std::uint8_t prerelease_number = 0;

  constexpr version(std::uint8_t major, std::uint8_t minor, std::uint8_t patch,
                    prerelease prerelease_type = prerelease::none,
                    std::uint8_t prerelease_number = 0) noexcept
      : major{major}, minor{minor}, patch{patch},
        prerelease_type{prerelease_type}, prerelease_number{
                                              prerelease_type ==
                                                      prerelease::none
                                                  ? static_cast<std::uint8_t>(0)
                                                  : prerelease_number} {}

  constexpr version(std::string_view str)
      : version(0, 0, 0, prerelease::none, 0) {
    from_string(str);
  }

  constexpr version(std::wstring_view str)
      : version(0, 0, 0, prerelease::none, 0) {
    from_string(str);
  }

  constexpr version() = default;
  // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase
  constexpr version(const version &) = default;
  constexpr version(version &&) = default;
  ~version() = default;
  constexpr version &operator=(const version &) = default;
  constexpr version &operator=(version &&) = default;

  std::wstring to_wstring() const {
    auto str =
        bela::StringCat(static_cast<int>(major), L".", static_cast<int>(minor),
                        L".", static_cast<int>(patch));
    switch (prerelease_type) {
    case prerelease::alpha:
      str.append(detail::Literal<wchar_t>::Alpha);
      break;
    case prerelease::beta:
      str.append(detail::Literal<wchar_t>::Beta);
      break;
    case prerelease::rc:
      str.append(detail::Literal<wchar_t>::RC);
      break;
    case prerelease::none:
      return str;
    default:
      break;
    }
    if (prerelease_number > 0) {
      bela::StrAppend(&str, L".", static_cast<int>(prerelease_number));
    }
    return str;
  }

  std::string to_string() const {
    auto str = bela::narrow::StringCat(static_cast<int>(major), ".",
                                       static_cast<int>(minor), ".",
                                       static_cast<int>(patch));
    switch (prerelease_type) {
    case prerelease::alpha:
      str.append(detail::Literal<char>::Alpha);
      break;
    case prerelease::beta:
      str.append(detail::Literal<char>::Beta);
      break;
    case prerelease::rc:
      str.append(detail::Literal<char>::RC);
      break;
    case prerelease::none:
      return str;
    default:
      SEMVER_THROW(std::invalid_argument{"invalid version"});
    }
    if (prerelease_number > 0) {
      bela::narrow::StrAppend(&str, ".", static_cast<int>(prerelease_number));
    }
    return str;
  }
  template <typename T>
  constexpr bool
  from_string_noexcept_t(std::basic_string_view<T> str) noexcept {
    if (std::size_t i = 0;
        detail::read_uint(str, i, major) && detail::read_dot(str, i) &&
        detail::read_uint(str, i, minor) && detail::read_dot(str, i) &&
        detail::read_uint(str, i, patch) && str.length() == i) {
      prerelease_type = prerelease::none;
      prerelease_number = 0;
      return true;
    } else if (detail::read_prerelease(str, i, prerelease_type) &&
               str.length() == i) {
      prerelease_number = 0;
      return true;
    } else if (detail::read_dot(str, i) &&
               detail::read_uint(str, i, prerelease_number) &&
               str.length() == i) {
      return true;
    }

    *this = version{0, 0, 0};
    return false;
  }

  constexpr bool from_string_noexcept(std::string_view str) noexcept {
    return from_string_noexcept_t(str);
  }
  
  constexpr bool from_string_noexcept(std::wstring_view str) noexcept {
    return from_string_noexcept_t(str);
  }

  constexpr void from_string(std::string_view str) {
    if (!from_string_noexcept_t(str)) {
      SEMVER_THROW(std::invalid_argument{"invalid version"});
    }
  }

  constexpr void from_string(std::wstring_view str) {
    if (!from_string_noexcept_t(str)) {
      SEMVER_THROW(std::invalid_argument{"invalid version"});
    }
  }

  constexpr int compare(const version &other) const noexcept {
    if (major != other.major) {
      return major - other.major;
    }
    if (minor != other.minor) {
      return minor - other.minor;
    }
    if (patch != other.patch) {
      return patch - other.patch;
    }
    if (prerelease_type != other.prerelease_type) {
      return static_cast<std::uint8_t>(prerelease_type) -
             static_cast<std::uint8_t>(other.prerelease_type);
    }
    if (prerelease_number != other.prerelease_number) {
      return prerelease_number - other.prerelease_number;
    }
    return 0;
  }
};

constexpr bool operator==(const version &lhs, const version &rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

constexpr bool operator!=(const version &lhs, const version &rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

constexpr bool operator>(const version &lhs, const version &rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

constexpr bool operator>=(const version &lhs, const version &rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

constexpr bool operator<(const version &lhs, const version &rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

constexpr bool operator<=(const version &lhs, const version &rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

inline std::ostream &operator<<(std::ostream &os, const version &v) {
  return os << v.to_string();
}

constexpr version operator""_version(const char *str, std::size_t size) {
  return version{std::string_view{str, size}};
}

inline std::string to_string(const version &v) { return v.to_string(); }
inline std::wstring to_wstring(const version &v) { return v.to_wstring(); }

constexpr std::optional<version>
from_string_noexcept(std::string_view str) noexcept {
  if (version v{0, 0, 0}; v.from_string_noexcept_t(str)) {
    return v;
  }
  return std::nullopt;
}
constexpr std::optional<version>
from_string_noexcept(std::wstring_view str) noexcept {
  if (version v{0, 0, 0}; v.from_string_noexcept_t(str)) {
    return v;
  }
  return std::nullopt;
}
constexpr version from_string(std::string_view str) { return version{str}; }
constexpr version from_string(std::wstring_view str) { return version{str}; }

} // namespace bela::semver

#endif
