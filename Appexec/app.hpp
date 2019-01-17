////

#ifndef PRIVEXEC_APP_HPP
#define PRIVEXEC_APP_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <Windowsx.h>
#include <CommCtrl.h>
#include <PathCch.h>
#include <vector>
#include <unordered_map>

namespace priv {

using wid_t = WELL_KNOWN_SID_TYPE;
using alias_t = std::unordered_map<std::wstring, std::wstring>;
bool AppAliasInitialize(HWND hbox, priv::alias_t &alias);

inline  std::wstring windowcontent(HWND hWnd) {
    auto l = GetWindowTextLengthW(hWnd);
    if (l == 0 || l > PATHCCH_MAX_CCH * 2) {
      return L"";
    }
    std::wstring s(l + 1, L'\0');
    GetWindowTextW(hWnd, &s[0], l + 1); //// Null T
    s.resize(l);
    return s;
  }

struct Element {
  HWND hInput{nullptr};
  HWND hButton{nullptr};
  void Visible(BOOL v) {
    EnableWindow(hInput, v);
    EnableWindow(hButton, v);
  }
  bool IsVisible() const { return IsWindowVisible(hInput) == TRUE; }
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
  void Update(const std::wstring &text) {
    // update hInput (commbox or edit text)
    ::SetWindowTextW(hInput, text.data());
  }
};

struct Appx {
  HWND hName{nullptr};
  HWND hAcl{nullptr};
  HWND hlview{nullptr};
  HWND hlpacbox{nullptr};

  std::wstring AppContainerName(){
    return windowcontent(hName);
  }

  std::wstring AclText(){
    return windowcontent(hAcl);
  }

  bool IsChecked(int i) { return ListView_GetCheckState(hlview, i) != 0; }
  bool IsLowPrivilegeAppContainer() { return Button_GetCheck(hlpacbox) != 0; }
  const wchar_t *CheckedValue(int i) {
    LVITEM item;
    memset(&item, 0, sizeof(LVITEM));
    item.mask = LVIF_PARAM;
    item.iItem = i;

    if (!ListView_GetItem(hlview, &item)) {
      return nullptr;
    }
    return reinterpret_cast<const wchar_t *>(item.lParam);
  }
  void Visible(BOOL visible) {
    // enable
    EnableWindow(hlview, visible);
    EnableWindow(hlpacbox, visible);
  }

  std::vector<std::wstring> Capabilities() {
    std::vector<std::wstring> cas;
    auto count = (int)::SendMessageW(hlview, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
      if (IsChecked(i)) {
        auto cv = CheckedValue(i);
        if (cv != nullptr) {
          cas.push_back(cv);
        }
      }
    }
    return cas;
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
  bool DropFiles(WPARAM wParam, LPARAM lParam);

private:
  bool Initialize(HWND window);
  bool InitializeCapabilities();
  std::wstring ResolveCMD();
  std::wstring ResolveCWD();
  bool AppLookupExecute();
  bool AppLookupCWD();
  bool AppLookupAcl(std::vector<std::wstring> &fsdir,
                    std::vector<std::wstring> &registries);
  bool AppExecute();
  HINSTANCE hInst{nullptr};
  HWND hWnd{nullptr};
  HWND hINFO{nullptr};
  Element cmd;
  Element cwd;
  Appx appx;
  alias_t alias;
};
} // namespace priv

#endif
