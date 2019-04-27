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
#include <string_view>
#include <vector>
#include <unordered_map>

namespace priv {

using wid_t = WELL_KNOWN_SID_TYPE;
using alias_t = std::unordered_map<std::wstring, std::wstring>;
bool AppAliasInitialize(HWND hbox, priv::alias_t &alias);
static constexpr const auto whitecolor = RGB(255, 255, 255);
static constexpr const auto blackcolor = RGB(0, 0, 0);

struct AppSettings {
  COLORREF bk{whitecolor};
  COLORREF textcolor{blackcolor};
};

bool AppInitializeSettings(AppSettings &as);
bool AppApplySettings(const AppSettings &as);

struct Appbox {
  Appbox &operator=(HWND hc) {
    hBox = hc;
    return *this;
  }
  int Index() const {
    // get current appbox index.
    return (int)SendMessage(hBox, CB_GETCURSEL, 0, 0);
  }
  size_t Append(int al, const wchar_t *text, bool sel = false) {
    auto rl = ::SendMessageW(hBox, CB_ADDSTRING, 1, (LPARAM)text);
    if (rl == CB_ERR || rl == CB_ERRSPACE) {
      return als.size();
    }
    if (sel) {
      ::SendMessageW(hBox, CB_SETCURSEL, (WPARAM)als.size(), 0);
    }
    als.push_back(al);
    return als.size();
  }
  bool IsMatch(int al) {
    auto index = Index();
    if (index >= als.size() || index < 0) {
      return false;
    }
    if (als[index] == al) {
      return true;
    }
    return false;
  }
  int AppIndex() const {
    auto index = Index();
    if (index >= als.size() || index < 0) {
      return -1;
    }
    return als[index];
  }
  HWND hBox{nullptr};
  std::vector<int> als;
};

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
	size_t buflen=static_cast<size_t>(l+1);
    std::wstring s(buflen, L'\0');
    GetWindowTextW(hInput, &s[0], l + 1); //// Null T
    s.resize(l);
    return s;
  }
  void Update(const std::wstring &text) {
    // update hInput (commbox or edit text)
    ::SetWindowTextW(hInput, text.data());
  }
};

struct AppCapabilities {
  HWND hlview{nullptr};
  HWND hlpacbox{nullptr};
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
  ~App() {
    if (hbrBkgnd != nullptr) {
      DeleteObject(hbrBkgnd);
      hbrBkgnd = nullptr;
    }
  }
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
  bool SelChanged();
  std::wstring ResolveCMD();
  std::wstring ResolveCWD();
  bool AppLookupExecute();
  bool AppLookupManifest();
  bool AppLookupCWD();
  bool AppExecute();
  HINSTANCE hInst{nullptr};
  HWND hWnd{nullptr};
  Appbox box;
  Element cmd;
  Element cwd;
  Element appx;
  AppCapabilities appcas;
  alias_t alias;
  HBRUSH hbrBkgnd{nullptr};
};
} // namespace priv

#endif
