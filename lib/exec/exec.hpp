// exec library new command feature
#ifndef WSUDO_EXEC_HPP
#define WSUDO_EXEC_HPP
#include <bela/env.hpp>

namespace wsudo::exec {
constexpr const wchar_t *string_nullable(std::wstring_view str) { return str.empty() ? nullptr : str.data(); }
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

enum class privilege_t : uint8_t {
  none,
  appcontainer,
  mic,
  standard,
  elevated, // such as administrator
  system,
  trustedinstaller,
};

enum class visible_t : uint8_t {
  none,
  newconsole,
  hide,
};

// command define
struct command {
  std::wstring path;
  std::wstring cwd;
  std::wstring env;
  std::vector<std::wstring> argv;
  uint32_t pid{0};
  privilege_t priv{privilege_t::standard};
  visible_t visible;
  bool execute(bela::error_code &ec);
};

struct appcommand {
  std::wstring path;
  std::wstring cwd;
  std::wstring appid;
  std::wstring appmanifest;
  std::wstring sid;
  std::wstring folder;
  std::wstring env;
  std::vector<std::wstring> argv;
  std::vector<std::wstring> allowdirs;
  std::vector<std::wstring> regdirs;
  std::vector<std::wstring> caps;
  visible_t visible;
  uint32_t pid{0};
  bool islpac{false};
  bool initialize(bela::error_code &ec);
  bool execute(bela::error_code &ec);
};

inline bool IsUserAdministratorsGroup() {
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                &AdministratorsGroup)) {
    return false;
  }
  BOOL b = FALSE;
  if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
    b = FALSE;
  }
  FreeSid(AdministratorsGroup);
  return b == TRUE;
}

bool SplitArgv(std::wstring_view cmd, std::wstring &path, std::vector<std::wstring> &argv, bela::error_code &ec);
} // namespace wsudo::exec

#endif