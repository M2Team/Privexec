///
#ifndef WSUDO_SERVER_HPP
#define WSUDO_SERVER_HPP
#include <bela/base.hpp>
#include <exec.hpp>

namespace wsudo {
class Server {
public:
  Server() = default;
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  ~Server() {
    if (hPipe != INVALID_HANDLE_VALUE) {
      CloseHandle(hPipe);
    }
  }
  bool Initialize(bela::error_code &ec);
  bool Waiting(std::wstring_view path, const std::vector<std::wstring> &argv, int timeout, bela::error_code &ec);
  std::wstring_view Name() const { return name; }

private:
  HANDLE hPipe{INVALID_HANDLE_VALUE};
  std::wstring name;
};
} // namespace wsudo

#endif