///////////////
#ifndef PRIVEXEC_PE_HPP
#define PRIVEXEC_PE_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <string_view>
#include <vector>
#include "readlink.hpp"

namespace priv {

class Memview {
public:
  Memview() = default;
  Memview(const Memview &) = delete;
  Memview &operator=(const Memview &) = delete;
  ~Memview() {
    if (ismaped) {
      UnmapViewOfFile(hMap);
    }
    if (hMap != INVALID_HANDLE_VALUE) {
      CloseHandle(hMap);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
      CloseHandle(hFile);
    }
  }
  bool Fileview(const std::wstring_view &path) {
    hFile = CreateFileW(path.data(), GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      return false;
    }
    LARGE_INTEGER li;
    if (GetFileSizeEx(hFile, &li)) {
      filesize = li.QuadPart;
    }
    hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (hMap == INVALID_HANDLE_VALUE) {
      return false;
    }
    baseAddress = ::MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (baseAddress == nullptr) {
      return false;
    }
    ismaped = true;
    return true;
  }
  int64_t FileSize() const { return filesize; }
  char *BaseAddress() { return reinterpret_cast<char *>(baseAddress); }

private:
  HANDLE hFile{INVALID_HANDLE_VALUE};
  HANDLE hMap{INVALID_HANDLE_VALUE};
  LPVOID baseAddress{nullptr};
  int64_t filesize{0};
  bool ismaped{false};
};

template <typename CharT>
bool EndsWith(const std::basic_string<CharT> &str, const CharT *cstr,
              size_t len) {
  if (str.size() < len) {
    return false;
  }
  return str.compare(str.size() - len, len, cstr) == 0;
}
template <typename CharT>
bool EndsWith(const std::basic_string_view<CharT> &str, const CharT *cstr,
              size_t len) {
  if (str.size() < len) {
    return false;
  }
  return str.compare(str.size() - len, len, cstr) == 0;
}
inline std::wstring GetFileTarget(std::wstring file) {
  std::wstring target;
  if (readlink(file, target)) {
    return target;
  }
  return file;
}

inline HMODULE KrModule() {
  static HMODULE hModule = GetModuleHandleW(L"kernel32.dll");
  if (hModule == nullptr) {
    OutputDebugStringW(L"GetModuleHandleA failed");
  }
  return hModule;
}

#ifndef _M_X64
class FsRedirection {
public:
  typedef BOOL WINAPI fntype_Wow64DisableWow64FsRedirection(PVOID *OldValue);
  typedef BOOL WINAPI fntype_Wow64RevertWow64FsRedirection(PVOID *OldValue);
  FsRedirection() {
    auto hModule = KrModule();
    auto pfnWow64DisableWow64FsRedirection =
        (fntype_Wow64DisableWow64FsRedirection *)GetProcAddress(
            hModule, "Wow64DisableWow64FsRedirection");
    if (pfnWow64DisableWow64FsRedirection) {
      pfnWow64DisableWow64FsRedirection(&OldValue);
    }
  }
  ~FsRedirection() {
    auto hModule = KrModule();
    auto pfnWow64RevertWow64FsRedirection =
        (fntype_Wow64RevertWow64FsRedirection *)GetProcAddress(
            hModule, "Wow64RevertWow64FsRedirection");
    if (pfnWow64RevertWow64FsRedirection) {
      pfnWow64RevertWow64FsRedirection(&OldValue);
    }
  }

private:
  PVOID OldValue = nullptr;
};
#endif

inline bool PESubsystemIsConsole(const std::wstring &file) {
#ifndef _M_X64
  FsRedirection fsr;
#endif
  auto target = GetFileTarget(file);
  constexpr const size_t minsize =
      sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS);
  Memview mv;
  if (!mv.Fileview(target)) {
    return false;
  }
  if (mv.FileSize() < minsize) {
    return false;
  }
  auto dh = reinterpret_cast<IMAGE_DOS_HEADER *>(mv.BaseAddress());
  if (minsize + dh->e_lfanew >= (uint64_t)mv.FileSize()) {
    return false;
  }
  auto nh =
      reinterpret_cast<IMAGE_NT_HEADERS *>(mv.BaseAddress() + dh->e_lfanew);
  if (nh->FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64)) {
    auto oh =
        reinterpret_cast<IMAGE_OPTIONAL_HEADER64 *>(&(nh->OptionalHeader));
    return (oh->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
  }
  auto oh = reinterpret_cast<IMAGE_OPTIONAL_HEADER32 *>(&(nh->OptionalHeader));
  return (oh->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
}

inline std::vector<std::wstring_view> StringViewSplit(std::wstring_view sv,
                                                      wchar_t delim) {
  std::vector<std::wstring_view> output;
  size_t first = 0;
  while (first < sv.size()) {
    const auto second = sv.find_first_of(delim, first);
    if (first != second) {
      output.emplace_back(sv.substr(first, second - first));
    }
    if (second == std::wstring_view::npos) {
      break;
    }
    first = second + 1;
  }
  return output;
}

inline bool PathFileIsExists(const std::wstring &file) {
  auto at = GetFileAttributesW(file.c_str());
  if (INVALID_FILE_ATTRIBUTES == at) {
    return false;
  }
  if ((at & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return false;
  }
  return true;
}

inline bool SearchSinglePath(const std::wstring &cmd, std::wstring_view p,
                             std::wstring &exe) {
  const wchar_t *suffix[] = {L"", L".exe", L".com", L".bat"};
  for (auto s : suffix) {
    std::wstring exe_(p.data(), p.size());
    if (exe_.empty() && exe_.back() != '\\') {
      exe_.push_back('\\');
    }
    exe_.append(cmd).append(s);
    if (PathFileIsExists(exe_)) {
      exe.assign(std::move(exe_));
      return true;
    }
  }
  return false;
}

inline bool FindExecutableImageEx(const std::wstring &cmd, std::wstring &exe) {
  auto pos = cmd.find(L'/');
  if (pos != std::wstring::npos) {
    exe = cmd;
    return PathFileIsExists(cmd);
  }
  pos = cmd.find(L'\\');
  if (pos != std::wstring::npos) {
    exe = cmd;
    return PathFileIsExists(cmd);
  }
  std::wstring Path(PATHCCH_MAX_CCH, L'\0');
  GetEnvironmentVariableW(L"PATH", &Path[0], PATHCCH_MAX_CCH);
  auto pv = StringViewSplit(Path, L';');
  for (const auto &p : pv) {
    if (SearchSinglePath(cmd, p, exe)) {
      return true;
    }
  }
  return false;
}

} // namespace priv

#endif