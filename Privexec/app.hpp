////

#ifndef PRIVEXEC_APP_HPP
#define PRIVEXEC_APP_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <unordered_map>

namespace priv {
using vcombox = std::unordered_map<HWND, int>;
using alias_t = std::unordered_map<std::wstring, std::wstring>;

class App {
public:
  App() = default;
  App(const App &) = delete;
  App &operator=(const App &) = delete;
  static App *GetThisFromHandle(HWND const window) noexcept {
    return reinterpret_cast<App *>(GetWindowLongPtrW(window, GWLP_USERDATA));
  }
  int run(HINSTANCE hInstance);
  static INT_PTR WINAPI WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                                   LPARAM lParam);
  INT_PTR MessageHandler(UINT message, WPARAM wParam, LPARAM lParam);

private:
  bool initialize(HWND window);
  bool aliasinit(); // initialize alias
  HINSTANCE hInst{nullptr};
  HWND hWnd{nullptr};
  vcombox box;
  alias_t alias;
};
} // namespace priv

#endif