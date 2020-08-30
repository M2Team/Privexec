//////// Appmanifest import
// working in progress
#include "app.hpp"
// https://www.appveyor.com/docs/windows-images-software/#visual-studio-2017
// Appveyor Visual Studio Community 2017 version 15.9.4
#include <bela/path.hpp>
#include <appx.hpp>

namespace priv {
bool App::ParseAppx(std::wstring_view file) {
  auto basename = bela::BaseName(file);
  title.Update(basename);
  std::vector<std::wstring> caps;
  bela::error_code ec;
  if (!wsudo::exec::LoadAppx(file, caps, ec)) {
    return false;
  }
  auto N = ListView_GetItemCount(appx.hlview);
  for (int i = 0; i < N; i++) {
    auto p = appx.CheckedValue(i);
    if (std::find(caps.begin(), caps.end(), p) != caps.end()) {
      ListView_SetCheckState(appx.hlview, i, TRUE);
    } else {
      ListView_SetCheckState(appx.hlview, i, FALSE);
    }
  }
  return true;
}
} // namespace priv