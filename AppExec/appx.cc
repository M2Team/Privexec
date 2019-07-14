//////// Appmanifest import
// working in progress
#include "app.hpp"
// https://www.appveyor.com/docs/windows-images-software/#visual-studio-2017
// Appveyor Visual Studio Community 2017 version 15.9.4
#include <filesystem>
#include <process.hpp>

namespace priv {
bool App::ParseAppx(std::wstring_view file) {
  std::filesystem::path p(file);
  title.Update(p.filename().c_str());
  std::vector<std::wstring> caps;
  if (!priv::MergeFromAppManifest(file, caps)) {
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