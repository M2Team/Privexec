#include <string>
#include <string_view>
#include "app.hpp"
#include "resource.h"
#include <capabilities.hpp>

/// SEE:
/// https://github.com/googleprojectzero/sandbox-attacksurface-analysis-tools/blob/master/NtApiDotNet/SecurityCapabilities.cs
// $source=Get-Content -Path .\SecurityCapabilities.cs -Raw
// Add-Type -TypeDefinition $source

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
  appcas.hlview = GetDlgItem(hWnd, IDL_APPCONTAINER_LISTVIEW);
  appcas.hlpacbox = GetDlgItem(hWnd, IDC_LPACMODE);
  ListView_SetExtendedListViewStyleEx(appcas.hlview, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
  LVCOLUMNW lvw;
  memset(&lvw, 0, sizeof(lvw));
  lvw.cx = 200;
  ListView_InsertColumn(appcas.hlview, 0, &lvw);
  constexpr std::size_t wnlen = sizeof(wncas) / sizeof(CapabilityName);
  for (std::size_t n = wnlen; n > 0; n--) {
    const auto &w = wncas[n - 1];
    LVITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.pszText = const_cast<LPWSTR>(w.name);
    item.lParam = (LPARAM)w.value;
    ListView_InsertItem(appcas.hlview, &item);
  }
  ListView_SetColumnWidth(appcas.hlview, 0, LVSCW_AUTOSIZE_USEHEADER);
  return true;
}
} // namespace priv