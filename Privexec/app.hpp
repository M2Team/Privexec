////

#ifndef PRIVEXEC_APP_HPP
#define PRIVEXEC_APP_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <vector>
#include <unordered_map>

namespace priv {

using vcombox = std::vector<applevel_t>;
using alias_t = std::unordered_map<std::wstring, std::wstring>;
using wid_t = WELL_KNOWN_SID_TYPE;
using capchecks_t = std::unordered_map<HWND, wid_t>;

struct applevel_t {
  int level{0};
};

struct Element {
  HWND hInput{nullptr};
  HWND hButton{nullptr};
  void Visible(BOOL v) {
    EnableWindow(hInput, v);
    EnableWindow(hButton, v);
  }
  std::wstring Content() {
    auto l = GetWindowTextLengthW(hInput);
    if (l == 0 || l > PATHCCH_MAX_CCH * 2) {
      return L"";
    }
    std::wstring s(l + 1, L'\0');
    GetWindowTextW(hInput, &s[0], l + 1); //// Null T
    s.resize(l);
    return s;
  }
  void Update(std::wstring text) {
    // update hInput (commbox or edit text)
    ::SetWindowTextW(hInput, text.data());
  }
};

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
  bool IsMatchedApplevel(int level) {
    auto index = SendMessageW(hBox, CB_GETCURSEL, 0, 0);
    if (index >= box.size() || index < 0) {
      return false;
    }
    if (box[index].level == level) {
      return true;
    }
    return false;
  }
  bool Initialize(HWND window);
  bool AliasInitialize(); // initialize alias
  bool InitializeCapabilities();
  bool UpdateCapabilities(const std::wstring &file);
  HINSTANCE hInst{nullptr};
  HWND hWnd{nullptr};
  HWND hBox{nullptr};
  Element cmd;
  Element cwd;
  Element appx;
  // HWND hExecute{nullptr};
  // HWND hExit{nullptr};
  vcombox box;
  capchecks_t checks;
  alias_t alias;
};
} // namespace priv

#endif
