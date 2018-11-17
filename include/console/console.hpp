///
#ifndef PRIVEXEC_CONSOLE_HPP
#define PRIVEXEC_CONSOLE_HPP
#include "adapter.hpp"
namespace priv {

template <typename... Args> int Print(const wchar_t *fmt, Args... args) {
  //
  return 0;
}
} // namespace priv
#include "adapter.ipp"
#endif