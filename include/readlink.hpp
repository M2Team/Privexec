/////////
#ifndef PRIVEXEC_READLINK_HPP
#define PRIVEXEC_READLINK_HPP
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#include <Windows.h>
#endif
#include <Dbghelp.h>
#include <string>
#include <string_view>
#include <winnt.h>
#include <winioctl.h>
#include <ShlObj.h>
#include <PathCch.h>
#pragma comment(lib, "DbgHelp.lib")

/*
#define IO_REPARSE_TAG_MOUNT_POINT              (0xA0000003L)
#define IO_REPARSE_TAG_HSM                      (0xC0000004L)
#define IO_REPARSE_TAG_HSM2                     (0x80000006L)
#define IO_REPARSE_TAG_SIS                      (0x80000007L)
#define IO_REPARSE_TAG_WIM                      (0x80000008L)
#define IO_REPARSE_TAG_CSV                      (0x80000009L)
#define IO_REPARSE_TAG_DFS                      (0x8000000AL)
#define IO_REPARSE_TAG_SYMLINK                  (0xA000000CL)
#define IO_REPARSE_TAG_DFSR                     (0x80000012L)
#define IO_REPARSE_TAG_DEDUP                    (0x80000013L)
#define IO_REPARSE_TAG_NFS                      (0x80000014L)
#define IO_REPARSE_TAG_FILE_PLACEHOLDER         (0x80000015L)
#define IO_REPARSE_TAG_WOF                      (0x80000017L)
#define IO_REPARSE_TAG_WCI                      (0x80000018L)
#define IO_REPARSE_TAG_WCI_1                    (0x90001018L)
#define IO_REPARSE_TAG_GLOBAL_REPARSE           (0xA0000019L)
#define IO_REPARSE_TAG_CLOUD                    (0x9000001AL)
#define IO_REPARSE_TAG_CLOUD_1                  (0x9000101AL)
#define IO_REPARSE_TAG_CLOUD_2                  (0x9000201AL)
#define IO_REPARSE_TAG_CLOUD_3                  (0x9000301AL)
#define IO_REPARSE_TAG_CLOUD_4                  (0x9000401AL)
#define IO_REPARSE_TAG_CLOUD_5                  (0x9000501AL)
#define IO_REPARSE_TAG_CLOUD_6                  (0x9000601AL)
#define IO_REPARSE_TAG_CLOUD_7                  (0x9000701AL)
#define IO_REPARSE_TAG_CLOUD_8                  (0x9000801AL)
#define IO_REPARSE_TAG_CLOUD_9                  (0x9000901AL)
#define IO_REPARSE_TAG_CLOUD_A                  (0x9000A01AL)
#define IO_REPARSE_TAG_CLOUD_B                  (0x9000B01AL)
#define IO_REPARSE_TAG_CLOUD_C                  (0x9000C01AL)
#define IO_REPARSE_TAG_CLOUD_D                  (0x9000D01AL)
#define IO_REPARSE_TAG_CLOUD_E                  (0x9000E01AL)
#define IO_REPARSE_TAG_CLOUD_F                  (0x9000F01AL)
#define IO_REPARSE_TAG_CLOUD_MASK               (0x0000F000L)
#define IO_REPARSE_TAG_APPEXECLINK              (0x8000001BL)
#define IO_REPARSE_TAG_PROJFS                   (0x9000001CL)
#define IO_REPARSE_TAG_STORAGE_SYNC             (0x8000001EL)
#define IO_REPARSE_TAG_WCI_TOMBSTONE            (0xA000001FL)
#define IO_REPARSE_TAG_UNHANDLED                (0x80000020L)
#define IO_REPARSE_TAG_ONEDRIVE                 (0x80000021L)
#define IO_REPARSE_TAG_PROJFS_TOMBSTONE         (0xA0000022L)
#define IO_REPARSE_TAG_AF_UNIX                  (0x80000023L)

/// Windows Linux Subsystem
#define IO_REPARSE_TAG_LX_FIFO                  (0x80000024L)
#define IO_REPARSE_TAG_LX_CHR                   (0x80000025L)
#define IO_REPARSE_TAG_LX_BLK                   (0x80000026L)
*/

#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK (0x8000001BL)
#endif

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_reparse_data_buffer

typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;         // Reparse tag type
  USHORT ReparseDataLength; // Length of the reparse data
  USHORT Reserved;          // Used internally by NTFS to store remaining length

  union {
    // Structure for IO_REPARSE_TAG_SYMLINK
    // Handled by nt!IoCompleteRequest
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;

    // Structure for IO_REPARSE_TAG_MOUNT_POINT
    // Handled by nt!IoCompleteRequest
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;

    // Structure for IO_REPARSE_TAG_WIM
    // Handled by wimmount!FPOpenReparseTarget->wimserv.dll
    // (wimsrv!ImageExtract)
    struct {
      GUID ImageGuid;           // GUID of the mounted VIM image
      BYTE ImagePathHash[0x14]; // Hash of the path to the file within the image
    } WimImageReparseBuffer;

    // Structure for IO_REPARSE_TAG_WOF
    // Handled by FSCTL_GET_EXTERNAL_BACKING, FSCTL_SET_EXTERNAL_BACKING in NTFS
    // (Windows 10+)
    struct {
      //-- WOF_EXTERNAL_INFO --------------------
      ULONG Wof_Version;  // Should be 1 (WOF_CURRENT_VERSION)
      ULONG Wof_Provider; // Should be 2 (WOF_PROVIDER_FILE)

      //-- FILE_PROVIDER_EXTERNAL_INFO_V1 --------------------
      ULONG FileInfo_Version; // Should be 1 (FILE_PROVIDER_CURRENT_VERSION)
      ULONG
      FileInfo_Algorithm; // Usually 0 (FILE_PROVIDER_COMPRESSION_XPRESS4K)
    } WofReparseBuffer;

    // Structure for IO_REPARSE_TAG_APPEXECLINK
    struct {
      ULONG StringCount;   // Number of the strings in the StringList, separated
                           // by '\0'
      WCHAR StringList[1]; // Multistring (strings separated by '\0', terminated
                           // by '\0\0')
    } AppExecLinkReparseBuffer;

    // Dummy structure
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

namespace priv {

struct ReparseBuffer {
  ReparseBuffer() {
    data = reinterpret_cast<REPARSE_DATA_BUFFER *>(
        malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
  }
  ~ReparseBuffer() {
    if (data != nullptr) {
      free(data);
    }
  }
  REPARSE_DATA_BUFFER *data{nullptr};
};

class ErrorMessage {
public:
  ErrorMessage(DWORD errid) : lastError(errid) {
    if (FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, errid, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (LPWSTR)&buf, 0, nullptr) == 0) {
      buf = nullptr;
    }
  }
  ~ErrorMessage() {
    if (buf != nullptr) {
      LocalFree(buf);
    }
  }
  const wchar_t *message() const { return buf == nullptr ? L"unknwon" : buf; }
  DWORD LastError() const { return lastError; }

private:
  DWORD lastError;
  LPWSTR buf{nullptr};
};

inline DWORD GetShortcutTargetPathImpl(const wchar_t *szShortcutFile,
                                       wchar_t *szTargetPath, DWORD bufLen) {
  DWORD rc = 0;
  HANDLE hFile = nullptr;
  HANDLE hMapFile = nullptr;
  UCHAR *buf = nullptr;

  // zero target path
  memset(szTargetPath, 0, bufLen * sizeof(wchar_t));

  // open shortcut file
  hFile = CreateFileW(szShortcutFile, GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    rc = GetLastError();
    goto cleanup;
  }

  // get file size
  DWORD dwSize = GetFileSize(hFile, nullptr);
  // test for files with a length of 0 (zero) and reject those files,
  // otherwise CreateFileMapping will fail
  if (dwSize == INVALID_FILE_SIZE || dwSize == 0) {
    rc = INVALID_FILE_SIZE;
    goto cleanup;
  }

  // create file mapping object
  hMapFile = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
  if (hMapFile == nullptr) {
    rc = GetLastError();
    goto cleanup;
  }

  // map view of the file mapping into the address space of our process
  buf = (UCHAR *)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
  if (buf == nullptr) {
    rc = GetLastError();
    goto cleanup;
  }

  // check for LNK file header "4C 00 00 00"
  if (*(DWORD *)(buf + 0x00) != 0x0000004C) {
    // LNK file header invalid
    rc = 1;
    goto cleanup;
  }

  // check for shell link GUID "{00021401-0000-0000-00C0-000000000046}"
  if (*(DWORD *)(buf + 0x04) != 0x00021401 ||
      *(DWORD *)(buf + 0x08) != 0x00000000 ||
      *(DWORD *)(buf + 0x0C) != 0x000000C0 ||
      *(DWORD *)(buf + 0x10) != 0x46000000) {
    // shell link GUID invalid
    rc = 1;
    goto cleanup;
  }

  // check for presence of shell item ID list
  if ((*(BYTE *)(buf + 0x14) & 0x01) == 0) {
    // shell item ID list is not present
    rc = 1;
    goto cleanup;
  }

  // convert shell item identifier list to file system path
  // the shell item ID list starts always at offset 0x4E after the LNK
  // file header and the WORD for the length of the item ID list
  // see the reference "The Windows Shortcut File Format" by "Jesse Hager"
  auto item = (LPCITEMIDLIST)(buf + 0x4E);
  if (SHGetPathFromIDListEx(item, szTargetPath, bufLen, GPFIDL_UNCPRINTER) ==
      FALSE) {
    rc = 1;
    goto cleanup;
  }

cleanup:
  if (buf != nullptr) {
    UnmapViewOfFile(buf);
  }

  if (hMapFile != nullptr) {
    CloseHandle(hMapFile);
  }
  if (hFile != nullptr) {
    CloseHandle(hFile);
  }

  return rc;
}

inline bool readshelllink(const std::wstring &shlink, std::wstring &target) {
  wchar_t buffer[8192];
  if (GetShortcutTargetPathImpl(shlink.c_str(), buffer, 8192) == 0) {
    target.assign(buffer);
    auto n = ExpandEnvironmentStringsW(target.c_str(), buffer, 8192);
    if (n > 0) {
      target.assign(buffer, n);
    }
    return true;
  }
  return false;
}

inline bool readlink(const std::wstring &symfile, std::wstring &realfile) {
  auto hFile = CreateFileW(
      symfile.c_str(), 0,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  ReparseBuffer rbuf;
  DWORD dwBytes = 0;
  if (DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, nullptr, 0, rbuf.data,
                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytes,
                      nullptr) != TRUE) {
    CloseHandle(hFile);
    return false;
  }
  CloseHandle(hFile);
  switch (rbuf.data->ReparseTag) {
  case IO_REPARSE_TAG_SYMLINK: {
    auto wstr = rbuf.data->SymbolicLinkReparseBuffer.PathBuffer +
                (rbuf.data->SymbolicLinkReparseBuffer.SubstituteNameOffset /
                 sizeof(WCHAR));
    auto wlen = rbuf.data->SymbolicLinkReparseBuffer.SubstituteNameLength /
                sizeof(WCHAR);
    if (wlen >= 4 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
        wstr[3] == L'\\') {
      /* Starts with \??\ */
      if (wlen >= 6 &&
          ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
           (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
          wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\')) {
        /* \??\<drive>:\ */
        wstr += 4;
        wlen -= 4;

      } else if (wlen >= 8 && (wstr[4] == L'U' || wstr[4] == L'u') &&
                 (wstr[5] == L'N' || wstr[5] == L'n') &&
                 (wstr[6] == L'C' || wstr[6] == L'c') && wstr[7] == L'\\') {
        /* \??\UNC\<server>\<share>\ - make sure the final path looks like */
        /* \\<server>\<share>\ */
        wstr += 6;
        wstr[0] = L'\\';
        wlen -= 6;
      }
    }
    realfile.assign(wstr, wlen);
  } break;
  case IO_REPARSE_TAG_MOUNT_POINT: {
    auto wstr = rbuf.data->MountPointReparseBuffer.PathBuffer +
                (rbuf.data->MountPointReparseBuffer.SubstituteNameOffset /
                 sizeof(WCHAR));
    auto wlen =
        rbuf.data->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
    /* Only treat junctions that look like \??\<drive>:\ as symlink. */
    /* Junctions can also be used as mount points, like \??\Volume{<guid>}, */
    /* but that's confusing for programs since they wouldn't be able to */
    /* actually understand such a path when returned by uv_readlink(). */
    /* UNC paths are never valid for junctions so we don't care about them. */
    if (!(wlen >= 6 && wstr[0] == L'\\' && wstr[1] == L'?' && wstr[2] == L'?' &&
          wstr[3] == L'\\' &&
          ((wstr[4] >= L'A' && wstr[4] <= L'Z') ||
           (wstr[4] >= L'a' && wstr[4] <= L'z')) &&
          wstr[5] == L':' && (wlen == 6 || wstr[6] == L'\\'))) {
      SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
      return false;
    }

    /* Remove leading \??\ */
    wstr += 4;
    wlen -= 4;
    realfile.assign(wstr, wlen);
  } break;
  case IO_REPARSE_TAG_APPEXECLINK: {
    if (rbuf.data->AppExecLinkReparseBuffer.StringCount != 0) {
      LPWSTR szString = (LPWSTR)rbuf.data->AppExecLinkReparseBuffer.StringList;
      for (ULONG i = 0; i < rbuf.data->AppExecLinkReparseBuffer.StringCount;
           i++) {
        if (i == 2) {
          realfile = szString;
        }
        szString += wcslen(szString) + 1;
      }
    }
  } break;
  default:
    return false;
  }
  return true;
}
} // namespace priv

#endif