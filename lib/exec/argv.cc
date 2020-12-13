// argv feature
#include "exec.hpp"
#include <bela/tokenizecmdline.hpp>
#include <bela/simulator.hpp>

namespace wsudo::exec {
//
bool SplitArgv(std::wstring_view cmd, std::wstring &path, std::vector<std::wstring> &argv, bela::error_code &ec) {
  bela::Tokenizer tokenizer;
  if (!tokenizer.Tokenize(cmd)) {
    ec = bela::make_error_code(1, L"bad command '", cmd, L"'");
    return false;
  }
  const auto Argv = tokenizer.Argv();
  const auto Argc = tokenizer.Argc();
  for (size_t i = 0; i < Argc; i++) {
    argv.emplace_back(Argv[i]);
  }
  if (argv.empty()) {
    ec = bela::make_error_code(1, L"bad command '", cmd, L"'");
    return false;
  }
  std::wstring_view arg0 = path.empty() ? argv[0] : path;
  std::wstring p;
  if (!bela::env::LookPath(arg0, p, true)) {
    ec = bela::make_error_code(1, L"command not found '", arg0, L"'");
    return false;
  }
  path.assign(std::move(p));
  return true;
}

} // namespace wsudo::exec
