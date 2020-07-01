#include <string>
#include <string_view>
#include <capabilities.hpp>
#include "app.hpp"
#include "resource.h"

namespace priv {

bool IsKnownCapabilityNames(std::wstring_view name) {
  for (auto c : KnownCapabilityNames) {
    if (name == c) {
      return true;
    }
  }
  return false;
}

bool App::InitializeCapabilities() {
  ListView_SetExtendedListViewStyleEx(appx.hlview, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
  LVCOLUMNW lvw;
  memset(&lvw, 0, sizeof(lvw));
  lvw.cx = 200;
  ListView_InsertColumn(appx.hlview, 0, &lvw);
  constexpr std::size_t wnlen = sizeof(wncas) / sizeof(CapabilityName);
  for (std::size_t n = wnlen; n > 0; n--) {
    const auto &w = wncas[n - 1];
    LVITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.pszText = const_cast<LPWSTR>(w.name);
    item.lParam = (LPARAM)w.value;
    ListView_InsertItem(appx.hlview, &item);
  }
  ListView_SetColumnWidth(appx.hlview, 0, LVSCW_AUTOSIZE_USEHEADER);
  return true;
}
} // namespace priv