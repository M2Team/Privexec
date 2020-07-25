////////////
#ifndef PRIVEXEC_FILE_HPP
#define PRIVEXEC_FILE_HPP
#include <cstdlib>
#include <cstdio>
#include <wchar.h>

namespace priv {
struct FD {
  FD() = default;
  FD(FILE *fd_) : fd(fd_) {}
  FD(FD &&other) {
    if (fd != nullptr) {
      fclose(fd);
    }
    fd = other.fd;
    other.fd = nullptr;
  }
  FD &operator=(FD &&other) {
    if (fd != nullptr) {
      fclose(fd);
    }
    fd = other.fd;
    other.fd = nullptr;
    return *this;
  }
  FD &operator=(FILE *f) {
    if (fd != nullptr) {
      fclose(fd);
    }
    fd = f;
    return *this;
  }
  FD(const FD &) = delete;
  FD &operator=(const FD &) = delete;
  ~FD() {
    if (fd != nullptr) {
      fclose(fd);
    }
  }

  explicit operator bool() const noexcept { return fd != nullptr; }
  operator FILE *() const { return fd; }
  FILE *fd{nullptr};
};
} // namespace priv

#endif