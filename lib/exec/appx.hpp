///
#ifndef WSUDO_EXEC_APPX_HPP
#define WSUDO_EXEC_APPX_HPP
#include <bela/base.hpp>

namespace wsudo::exec {
bool LoadAppx(std::wstring_view file, std::vector<std::wstring> &caps, bela::error_code &ec);
}

#endif