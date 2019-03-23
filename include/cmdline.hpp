/////
#ifndef PRIVEXEC_CMDLINE_HPP
#define PRIVEXEC_CMDLINE_HPP

#include <string>
#include <string_view>

// https://github.com/golang/go/blob/50bd1c4d4eb4fac8ddeb5f063c099daccfb71b26/src/syscall/exec_windows.go#L26

inline std::wstring EscapeArg(std::wstring_view arg) {
  if (arg.empty()) {
    return L"\"\"";
  }
  bool hasspace = false;
  auto n = arg.size();
  for (auto c : arg) {
    switch (c) {
    case L'"':
    case L'\\':
      n++;
      break;
    case L' ':
    case L'\t':
      hasspace = true;
      break;
    default:
      break;
    }
  }
  if (hasspace) {
    n += 2;
  }
  if (n == arg.size()) {
    return std::wstring(arg);
  }
  std::wstring buf;
  size_t slashes = 0;
  if (hasspace) {
    buf.push_back(L'"');
  }
  for (auto c : arg) {
    switch (c) {
    case L'\\':
      slashes++;
      buf.push_back(L'\\');
      break;
    case L'"': {
      for (; slashes > 0; slashes--) {
        buf.push_back(L'\\');
      }
      buf.push_back(L'\\');
      buf.push_back(c);
    } break;
    default:
      slashes = 0;
      buf.push_back(c);
      break;
    }
  }
  if (hasspace) {
    for (; slashes > 0; slashes--) {
      buf.push_back(L'\\');
    }
    buf.push_back(L'"');
  }
  return buf;
}

template <typename... Args>
std::wstring MakeCmdline(std::wstring_view argv0, Args... args) {
  auto sv = std::initialize_list<std::wstring_view>{args...};
  auto cmdline = EscapeArg(argv0);
  for (auto s : sv) {
    cmdline.append(L" ").append(EscapeArg(s));
  }
  if (!cmdline.empty() && cmdline.back() == L' ') {
    cmdline.pop_back();
  }
  return cmdline;
}

#endif