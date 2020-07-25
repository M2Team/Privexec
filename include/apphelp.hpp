////////////
#ifndef PRIV_APPHELP_HPP
#define PRIV_APPHELP_HPP
#include <objbase.h>
#include <bela/base.hpp>
#include <bela/terminal.hpp>

namespace priv {
class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitialize(NULL);
    if (FAILED(hr)) {
      bela::FPrintF(stderr, L"\x1b[31minitialize dotcom error: 0x%08x\x1b[0m\n", hr);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};

} // namespace priv

#endif