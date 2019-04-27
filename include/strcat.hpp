///////
#ifndef CLANGBUILDER_STRCAT_HPP
#define CLANGBUILDER_STRCAT_HPP
#include "charconv.hpp"

namespace base {

namespace strings_internal {
// AlphaNumBuffer allows a way to pass a string to StrCat without having to do
// memory allocation.  It is simply a pair of a fixed-size character array, and
// a size.  Please don't use outside of absl, yet.
template <size_t max_size> struct AlphaNumBuffer {
  std::array<char, max_size> data;
  size_t size;
};

} // namespace strings_internal

class AlphaNum {
public:
  AlphaNum(short x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(unsigned short x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(int x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(unsigned int x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(long x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }

  AlphaNum(unsigned long x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(long long x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(unsigned long long x) {
    auto ret = to_chars(digist_, digist_ + 32, x, 10);
    piece_ = std::wstring_view(digist_, ret.ptr - digist_);
  }
  AlphaNum(const wchar_t *cstr) : piece_(cstr) {}
  AlphaNum(std::wstring_view sv) : piece_(sv) {}
  AlphaNum(const std::wstring &str) : piece_(str) {}
  AlphaNum(char c) = delete;
  AlphaNum(const AlphaNum &) = delete;
  AlphaNum &operator=(const AlphaNum &) = delete;
  std::wstring_view::size_type size() const { return piece_.size(); }
  const wchar_t *data() const { return piece_.data(); }
  std::wstring_view Piece() const { return piece_; }

private:
  std::wstring_view piece_;
  wchar_t digist_[32];
};

namespace internal {
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

} // namespace internal
inline std::wstring StringCat() { return std::wstring(); }

inline std::wstring StringCat(const AlphaNum &a) {
  return std::wstring(a.Piece());
}

inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b) {
  std::wstring result;
  result.resize(a.size() + b.size());
  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  out = internal::Append(out, a);
  out = internal::Append(out, b);
  return result;
}
inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b,
                           const AlphaNum &c) {
  std::wstring result;
  result.resize(a.size() + b.size() + c.size());
  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  out = internal::Append(out, a);
  out = internal::Append(out, b);
  out = internal::Append(out, c);
  return result;
}
inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b,
                           const AlphaNum &c, const AlphaNum &d) {
  std::wstring result;
  result.resize(a.size() + b.size() + c.size() + d.size());
  wchar_t *const begin = &*result.begin();
  wchar_t *out = begin;
  out = internal::Append(out, a);
  out = internal::Append(out, b);
  out = internal::Append(out, c);
  out = internal::Append(out, d);
  return result;
}

// Support 5 or more arguments
template <typename... AV>
inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b,
                           const AlphaNum &c, const AlphaNum &d,
                           const AlphaNum &e, const AV &... args) {
  return internal::CatPieces({a.Piece(), b.Piece(), c.Piece(), d.Piece(),
                              e.Piece(),
                              static_cast<const AlphaNum &>(args).Piece()...});
}

} // namespace base

#endif