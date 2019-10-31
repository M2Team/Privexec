//          _____                            _   _
//         / ____|                          | | (_)
//        | (___   ___ _ __ ___   __ _ _ __ | |_ _  ___
//         \___ \ / _ \ '_ ` _ \ / _` | '_ \| __| |/ __|
//         ____) |  __/ | | | | | (_| | | | | |_| | (__
//        |_____/ \___|_| |_| |_|\__,_|_| |_|\__|_|\___|
// __      __           _             _                _____
// \ \    / /          (_)           (_)              / ____|_     _
//  \ \  / /__ _ __ ___ _  ___  _ __  _ _ __   __ _  | |   _| |_ _| |_
//   \ \/ / _ \ '__/ __| |/ _ \| '_ \| | '_ \ / _` | | |  |_   _|_   _|
//    \  /  __/ |  \__ \ | (_) | | | | | | | | (_| | | |____|_|   |_|
//     \/ \___|_|  |___/_|\___/|_| |_|_|_| |_|\__, |  \_____|
// https://github.com/Neargye/semver           __/ |
// version 0.2.0                              |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2019 Daniil Goncharov <neargye@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef NEARGYE_SEMANTIC_VERSIONING_HPP
#define NEARGYE_SEMANTIC_VERSIONING_HPP

#include <cstdint>
#include <cstddef>
#include <limits>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

// Allow to disable exceptions.
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && !defined(SEMVER_NOEXCEPTION)
#  include <stdexcept>
#  define SEMVER_THROW(exception) throw exception
#else
#  include <cstdlib>
#  define SEMVER_THROW(exception) std::abort()
#endif

namespace semver {

enum struct prerelease : std::uint8_t {
  alpha = 0,
  beta = 1,
  rc = 2,
  none = 3
};

namespace detail {

constexpr char char_to_lower(char c) noexcept {
  return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

constexpr bool str_equals(std::string_view lhs, std::size_t pos, std::string_view rhs) noexcept {
  for (std::size_t i1 = pos, i2 = 0; i1 < lhs.length() && i2 < rhs.length(); ++i1, ++i2) {
    if (char_to_lower(lhs[i1]) != char_to_lower(rhs[i2])) {
      return false;
    }
  }

  return true;
}

constexpr bool is_digit(char c) noexcept {
  return c >= '0' && c <= '9';
}

constexpr std::uint8_t char_to_digit(char c) noexcept {
  return c - '0';
}

constexpr bool read_uint(std::string_view str, std::size_t& i, std::uint8_t& d) noexcept {
  if (std::uint32_t t = 0; i < str.length() && is_digit(str[i])) {
    for (std::size_t p = i, s = 0; p < str.length() && s < 3; ++p, ++s) {
      if (is_digit(str[p])) {
        t = t * 10 + char_to_digit(str[p]);
        ++i;
      } else {
        break;
      }
    }
    if (t <= std::numeric_limits<std::uint8_t>::max()) {
      d = static_cast<std::uint8_t>(t);
      return true;
    }
  }

  return false;
}

constexpr bool read_dot(std::string_view str, std::size_t& i) noexcept {
  if (i < str.length() && str[i] == '.') {
    ++i;
    return true;
  }

  return false;
}

constexpr bool read_prerelease(std::string_view str, std::size_t& i, prerelease& p) noexcept {
  constexpr std::string_view alpha{"-alpha"};
  constexpr std::string_view beta{"-beta"};
  constexpr std::string_view rc{"-rc"};

  if (i >= str.length()) {
    p = prerelease::none;
    return false;
  } else if (str_equals(str, i, alpha)) {
    i += alpha.length();
    p = prerelease::alpha;
    return true;
  } else if (str_equals(str, i, beta)) {
    i += beta.length();
    p = prerelease::beta;
    return true;
  } else if (str_equals(str, i, rc)) {
    i += rc.length();
    p = prerelease::rc;
    return true;
  }

  return false;
}

} // namespace semver::detail

struct alignas(1) version {
  std::uint8_t major = 0;
  std::uint8_t minor = 1;
  std::uint8_t patch = 0;
  prerelease prerelease_type = prerelease::none;
  std::uint8_t prerelease_number = 0;

  constexpr version(std::uint8_t major,
                    std::uint8_t minor,
                    std::uint8_t patch,
                    prerelease prerelease_type = prerelease::none,
                    std::uint8_t prerelease_number = 0) noexcept
      : major{major},
        minor{minor},
        patch{patch},
        prerelease_type{prerelease_type},
        prerelease_number{prerelease_type == prerelease::none ? static_cast<std::uint8_t>(0) : prerelease_number} {
  }

  constexpr version(std::string_view str) : version(0, 0, 0, prerelease::none, 0) {
    from_string(str);
  }

  constexpr version() = default; // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase

  constexpr version(const version&) = default;

  constexpr version(version&&) = default;

  ~version() = default;

  constexpr version& operator=(const version&) = default;

  constexpr version& operator=(version&&) = default;

  std::string to_string() const {
    auto str = std::to_string(major)
                  .append(".")
                  .append(std::to_string(minor))
                  .append(".")
                  .append(std::to_string(patch));

    switch (prerelease_type) {
      case prerelease::alpha:
        str.append("-alpha");
        break;

      case prerelease::beta:
        str.append("-beta");
        break;

      case prerelease::rc:
        str.append("-rc");
        break;

      case prerelease::none:
        return str;

      default:
        SEMVER_THROW(std::invalid_argument{"invalid version"});
    }

    if (prerelease_number > 0) {
      str.append(".").append(std::to_string(prerelease_number));
    }

    return str;
  }

  constexpr bool from_string_noexcept(std::string_view str) noexcept {
    if (std::size_t i = 0;
        detail::read_uint(str, i, major) && detail::read_dot(str, i) &&
        detail::read_uint(str, i, minor) && detail::read_dot(str, i) &&
        detail::read_uint(str, i, patch) && str.length() == i) {
      prerelease_type = prerelease::none;
      prerelease_number = 0;
      return true;
    } else if (detail::read_prerelease(str, i, prerelease_type) && str.length() == i) {
      prerelease_number = 0;
      return true;
    } else if (detail::read_dot(str, i) && detail::read_uint(str, i, prerelease_number) && str.length() == i) {
      return true;
    }

    *this = version{0, 0, 0};
    return false;
  }

  constexpr void from_string(std::string_view str) {
    if (!from_string_noexcept(str)) {
      SEMVER_THROW(std::invalid_argument{"invalid version"});
    }
  }

  constexpr int compare(const version& other) const noexcept {
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
      return static_cast<std::uint8_t>(prerelease_type) - static_cast<std::uint8_t>(other.prerelease_type);
    }

    if (prerelease_number != other.prerelease_number) {
      return prerelease_number - other.prerelease_number;
    }

    return 0;
  }
};

constexpr bool operator==(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

constexpr bool operator!=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

constexpr bool operator>(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

constexpr bool operator>=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

constexpr bool operator<(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

constexpr bool operator<=(const version& lhs, const version& rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

inline std::ostream& operator<<(std::ostream& os, const version& v) {
  return os << v.to_string();
}

constexpr version operator""_version(const char* str, std::size_t size) {
  return version{std::string_view{str, size}};
}

inline std::string to_string(const version& v) {
  return v.to_string();
}

constexpr std::optional<version> from_string_noexcept(std::string_view str) noexcept {
  if (version v{0, 0, 0}; v.from_string_noexcept(str)) {
    return v;
  }

  return std::nullopt;
}

constexpr version from_string(std::string_view str) {
  return version{str};
}

} // namespace semver

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
