///////////
// bela expand env
#include <bela/base.hpp>
#include <bela/env.hpp>
#include <bela/strcat.hpp>

namespace bela {
namespace env_internal {
constexpr const size_t npos = static_cast<size_t>(-1);
size_t memsearch(const wchar_t *begin, const wchar_t *end, int ch) {
  for (auto it = begin; it != end; it++) {
    if (*it == ch) {
      return it - begin;
    }
  }
  return npos;
}

} // namespace env_internal

// https://docs.microsoft.com/en-us/windows/desktop/api/processenv/nf-processenv-expandenvironmentstringsw
std::wstring ExpandEnv(std::wstring_view sv) {
  auto pos = sv.find('%');
  if (pos == std::wstring_view::npos) {
    return std::wstring(sv);
  }
  auto pos2 = sv.find(L'%', pos + 1);
  if (pos2 == std::wstring_view::npos) {
    return std::wstring(sv);
  }
  std::wstring buf;
  buf.resize(sv.size() + 256);
  auto N = ExpandEnvironmentStringsW(sv.data(), buf.data(),
                                     static_cast<DWORD>(buf.size()));
  if (static_cast<size_t>(N) > buf.size()) {
    buf.resize(N);
    N = ExpandEnvironmentStringsW(sv.data(), buf.data(),
                                  static_cast<DWORD>(buf.size()));
  }
  if (N == 0 || static_cast<size_t>(N) > buf.size()) {
    return L"";
  }
  buf.resize(N - 1);
  return buf;
}

std::wstring PathUnExpand(std::wstring_view sv) {
  constexpr const std::wstring_view envvars[] = {
      //--
      L"ALLUSERSPROFILE", //
      L"APPDATA",         //
      L"COMPUTERNAME",    //
      L"ProgramFiles",    //
      L"SystemRoot",      //
      L"SystemDrive",     //
      L"USERPROFILE"
      //
  };
  std::wstring buf;
  buf.reserve(sv.size());
  auto it = sv.data();
  auto end = it + sv.size();
  while (it < end) {
    auto pos = env_internal::memsearch(it, end, '%');
    if (pos == env_internal::npos) {
      buf.append(it, end - it);
      break;
    }
    buf.append(it, pos);
    it += pos + 1;
    if (it >= end) {
      break;
    }
    size_t xlen = end - it;
    for (auto e : envvars) {
      if (xlen < e.size() + 1) {
        continue;
      }
      if (_wcsnicmp(it, e.data(), e.size()) == 0 && it[e.size()] == '%') {
        buf.append(GetEnv(e));
        it += e.size() + 1;
      }
    }
  }
  return buf;
}

namespace env {

inline bool is_shell_specia_var(wchar_t ch) {
  return (ch == '*' || ch == '#' || ch == '$' || ch == '@' || ch == '!' ||
          ch == '?' || ch == '-' || (ch >= '0' && ch <= '9'));
}

inline bool is_alphanum(wchar_t ch) {
  return (ch == '_' || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') ||
          (ch >= 'A' && ch <= 'Z'));
}

std::wstring_view resovle_shell_name(std::wstring_view s, size_t &off) {
  off = 0;
  if (s.front() == '{') {
    if (s.size() > 2 && is_shell_specia_var(s[1]) && s[2] == '}') {
      off = 3;
      return s.substr(1, 2);
    }
    for (size_t i = 1; i < s.size(); i++) {
      if (s[i] == '}') {
        if (i == 1) {
          off = 2;
          return L"";
        }
        off = i + 1;
        return s.substr(1, i - 1);
      }
    }
    off = 1;
    return L"";
  }
  if (is_shell_specia_var(s[0])) {
    off = 1;
    return s.substr(0, 1);
  }
  size_t i = 0;
  for (; i < s.size() && is_alphanum(s[i]); i++) {
    ;
  }
  off = i;
  return s.substr(0, i);
}

bool os_expand_env(const std::wstring &key, std::wstring &value) {
  auto len = GetEnvironmentVariableW(key.data(), nullptr, 0);
  if (len == 0) {
    return false;
  }
  auto oldsize = value.size();
  value.resize(oldsize + len);
  len = GetEnvironmentVariableW(key.data(), value.data() + oldsize, len);
  if (len == 0) {
    value.resize(oldsize);
    return false;
  }
  value.resize(oldsize + len);
  return true;
}

bool Derivator::AddBashCompatible(int argc, wchar_t *const *argv) {
  // $0~$N
  for (int i = 0; i < argc; i++) {
    envblock.emplace(bela::AlphaNum(i).Piece(), argv[i]);
  }
  envblock.emplace(
      L"$",
      bela::AlphaNum(GetCurrentProcessId()).Piece()); // current process PID
  envblock.emplace(L"@", GetCommandLineW());          // $@ -->cmdline
  std::wstring userprofile;
  if (os_expand_env(L"USERPROFILE", userprofile)) {
    envblock.emplace(L"HOME", userprofile);
  }
  return true;
}

bool Derivator::EraseEnv(std::wstring_view key) {
  return envblock.erase(key) != 0;
}

bool Derivator::SetEnv(std::wstring_view key, std::wstring_view value,
                        bool force) {
  if (force) {
    // envblock[key] = value;
    envblock.insert_or_assign(key, value);
    return true;
  }
  return envblock.emplace(key, value).second;
}

bool Derivator::PutEnv(std::wstring_view nv, bool force) {
  auto pos = nv.find(L'=');
  if (pos == std::wstring_view::npos) {
    return SetEnv(nv, L"", force);
  }
  return SetEnv(nv.substr(0, pos), nv.substr(pos + 1), force);
}

[[nodiscard]] std::wstring_view
Derivator::GetEnv(std::wstring_view key) const {
  auto it = envblock.find(key);
  if (it == envblock.end()) {
    return L"";
  }
  return it->second;
}

bool Derivator::AppendEnv(std::wstring_view key, std::wstring &s) const {
  auto it = envblock.find(key);
  if (it == envblock.end()) {
    return false;
  }
  s.append(it->second);
  return true;
}

// Expand Env string to normal string only support  Unix style'${KEY}'
bool Derivator::ExpandEnv(std::wstring_view raw, std::wstring &w,
                           bool disableos) const {
  w.reserve(raw.size() * 2);
  size_t i = 0;
  for (size_t j = 0; j < raw.size(); j++) {
    if (raw[j] == '$' && j + 1 < raw.size()) {
      w.append(raw.substr(i, j - i));
      size_t off = 0;
      auto name = resovle_shell_name(raw.substr(j + 1), off);
      if (name.empty()) {
        if (off == 0) {
          w.push_back(raw[j]);
        }
      } else {
        if (!AppendEnv(name, w)) {
          if (!disableos) {
            os_expand_env(std::wstring(name), w);
          }
        }
      }
      j += off;
      i = j + 1;
    }
  }
  w.append(raw.substr(i));
  return true;
}

// DerivatorMT support MultiThreading

bool DerivatorMT::AddBashCompatible(int argc, wchar_t *const *argv) {
  for (int i = 0; i < argc; i++) {
    envblock.emplace(bela::AlphaNum(i).Piece(), argv[i]);
  }
  envblock.emplace(
      L"$",
      bela::AlphaNum(GetCurrentProcessId()).Piece()); // $$ --> current PID
  envblock.emplace(L"@", GetCommandLineW());          // $@ -->cmdline
  std::wstring userprofile;
  if (os_expand_env(L"USERPROFILE", userprofile)) {
    envblock.emplace(L"HOME", userprofile);
  }
  return true;
}

bool DerivatorMT::EraseEnv(std::wstring_view key) {
  return envblock.erase(key) != 0; /// Internal is thread safe
}

bool DerivatorMT::SetEnv(std::wstring_view key, std::wstring_view value,
                          bool force) {
  if (force) {
    // envblock[key] = value;
    envblock.insert_or_assign(key, value);
    return true;
  }
  return envblock.emplace(key, value).second;
}

bool DerivatorMT::PutEnv(std::wstring_view nv, bool force) {
  auto pos = nv.find(L'=');
  if (pos == std::wstring_view::npos) {
    return SetEnv(nv, L"", force);
  }
  return SetEnv(nv.substr(0, pos), nv.substr(pos + 1), force);
}

[[nodiscard]] std::wstring DerivatorMT::GetEnv(std::wstring_view key) {
  auto it = envblock.find(key);
  if (it == envblock.end()) {
    return L"";
  }
  return it->second;
}

bool DerivatorMT::AppendEnv(std::wstring_view key, std::wstring &s) {
  auto it = envblock.find(key);
  if (it == envblock.end()) {
    return false;
  }
  s.append(it->second);
  return true;
}

bool DerivatorMT::ExpandEnv(std::wstring_view raw, std::wstring &w,
                             bool disableos) {
  w.reserve(raw.size() * 2);
  size_t i = 0;
  for (size_t j = 0; j < raw.size(); j++) {
    if (raw[j] == '$' && j + 1 < raw.size()) {
      w.append(raw.substr(i, j - i));
      size_t off = 0;
      auto name = resovle_shell_name(raw.substr(j + 1), off);
      if (name.empty()) {
        if (off == 0) {
          w.push_back(raw[j]);
        }
      } else {
        if (!AppendEnv(name, w)) {
          if (!disableos) {
            os_expand_env(std::wstring(name), w);
          }
        }
      }
      j += off;
      i = j + 1;
    }
  }
  w.append(raw.substr(i));
  return true;
}

} // namespace env
} // namespace bela
