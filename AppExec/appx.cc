//////// Appmanifest import
// working in progress
#include "app.hpp"
// https://www.appveyor.com/docs/windows-images-software/#visual-studio-2017
// Appveyor Visual Studio Community 2017 version 15.9.4
#include <filesystem>

namespace priv {
bool App::ParseAppx(std::wstring_view file) {
  std::filesystem::path p(file);
  title.Update(p.filename().c_str());

  return false;
}
} // namespace priv