//
#include <atomic>
#include "server.hpp"

namespace wsudo {

std::atomic_uint32_t ProcessPipeId{0};
bool Server::Initialize(bela::error_code &ec) {
  auto PipeId = static_cast<uint32_t>(ProcessPipeId++);
  name = bela::StringCat(LR"(\.\pipe\wsudo-delegation-)", GetCurrentProcessId(), L"-", PipeId);

  if (hPipe = CreateNamedPipeW(name.data(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 65536, 65536,
                               0, nullptr);
      hPipe == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code(L"Server::Initialize<CreateNamedPipeW> ");
    return false;
  }
  return true;
}

bool Server::Waiting(std::wstring_view path, const std::vector<std::wstring> &argv, int timeout, bela::error_code &ec) {

}

} // namespace wsudo