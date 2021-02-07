///
#include <apphelp.hpp>
#include "app.hpp"
#include <objbase.h>

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                    _In_ int nCmdShow) {
  priv::dotcom_global_initializer di;
  priv::App app;
  // INITCOMMONCONTROLSEX iccex;
  // iccex.dwICC = ICC_WIN95_CLASSES;
  // iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  // InitCommonControlsEx(&iccex);
  return app.run(hInstance);
}
