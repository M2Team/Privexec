////////////
#ifndef PRIV_APPHELP_HPP
#define PRIV_APPHELP_HPP
#include <objbase.h>

namespace priv {
class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitialize(NULL);
    if (FAILED(hr)) {
      priv::Print(priv::fc::Red, L"initialize dotcom error: 0x%08x\n", hr);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};
} // namespace priv

#endif