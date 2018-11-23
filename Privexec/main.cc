///
#include "app.hpp"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  priv::App app;
  return app.run(hInstance);
}
