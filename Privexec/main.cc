///
#include "app.hpp"
#include <objbase.h>

class DotComInitialize {
public:
  DotComInitialize() {
    if (FAILED(CoInitialize(nullptr))) {
      MessageBoxW(nullptr, L"CoInitialize()", L"CoInitialize() failed",
                  MB_OK | MB_ICONERROR);
    }
  }
  ~DotComInitialize() { CoUninitialize(); }
};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  priv::App app;
  DotComInitialize dotcom;
  return app.run(hInstance);
}
