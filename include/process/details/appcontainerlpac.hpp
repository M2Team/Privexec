////////////// TODO impl LPAC
#ifndef APPCONTAINERLPAC_HPP
#define APPCONTAINERLPAC_HPP
#include <string>
#include <comdef.h>
#include <Sddl.h>
#include <Shlwapi.h>
#include "processfwd.hpp"
#include "systemhelper.hpp"

namespace priv {
//

template <typename T> T *Alloc(size_t n = 1) {
  return reinterpret_cast<T *>(HeapAlloc(GetProcessHeap(), 0, sizeof(T) * n));
}

inline bool enableLPAC(HANDLE hToken) {
  // https://github.com/Microsoft/Windows-classic-samples/blob/master/Samples/Security/ResourceAttributes/cpp/ResourceAttributesSample.cpp
  auto ca1 = Alloc<CLAIM_SECURITY_ATTRIBUTE_V1>();
  auto u64 = Alloc<UINT64>();
  auto deleter = finally([&] {
    HeapFree(GetProcessHeap(), 0, ca1);
    HeapFree(GetProcessHeap(), 0, u64);
  });
  ca1->Name = L"WIN://NOALLAPPPKG";
  ca1->ValueCount = 1;
  ca1->ValueType = CLAIM_SECURITY_ATTRIBUTE_TYPE_INT64;
  ca1->Values.pUint64 = u64;
  ca1->Reserved = 0;
  ca1->Flags = 0;
  CLAIM_SECURITY_ATTRIBUTES_INFORMATION cai = {0};
  cai.Version = CLAIM_SECURITY_ATTRIBUTES_INFORMATION_VERSION_V1;
  cai.AttributeCount = 1;
  cai.Attribute.pAttributeV1 = ca1;
  return SetTokenInformation(hToken, TokenUserClaimAttributes, &cai,
                             sizeof(cai)) == TRUE;
}

inline bool CreateProcessElevatedLPAC(
    LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
    DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
  /// Enable Privilege TODO
  SysImpersonator impersonator;
  if (!impersonator.PreImpersonation()) {
    return false;
  }
  PrivilegeView pv;
  pv.privis.push_back(SE_TCB_NAME);                ///
  pv.privis.push_back(SE_ASSIGNPRIMARYTOKEN_NAME); //
  pv.privis.push_back(SE_INCREASE_QUOTA_NAME);     //
  if (!impersonator.Impersonation(&pv)) {
    return false;
  }
  auto hToken = INVALID_HANDLE_VALUE;
  auto hTokenDup = INVALID_HANDLE_VALUE;
  auto deleter = finally([&] {
    CloseHandleEx(hToken);
    CloseHandleEx(hTokenDup);
  });
  // get user login token
  if (WTSQueryUserToken(impersonator.SessionId(), &hToken) != TRUE) {
    return false;
  }

  if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification,
                       TokenPrimary, &hTokenDup) != TRUE) {
    return false;
  }
  if (!enableLPAC(hToken)) {
    return false;
  }
  return CreateProcessAsUserW(hToken, lpApplicationName, lpCommandLine,
                              lpProcessAttributes, lpThreadAttributes,
                              bInheritHandles, dwCreationFlags, lpEnvironment,
                              lpCurrentDirectory, lpStartupInfo,
                              lpProcessInformation) == TRUE;
}

inline bool CreateProcessLPAC(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                              LPSECURITY_ATTRIBUTES lpProcessAttributes,
                              LPSECURITY_ATTRIBUTES lpThreadAttributes,
                              BOOL bInheritHandles, DWORD dwCreationFlags,
                              LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
                              LPSTARTUPINFOW lpStartupInfo,
                              LPPROCESS_INFORMATION lpProcessInformation) {
  if (IsUserAdministratorsGroup()) {
    return CreateProcessElevatedLPAC(
        lpApplicationName, lpCommandLine, lpProcessAttributes,
        lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
        lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
  }
  auto hToken = INVALID_HANDLE_VALUE;
  auto hTokenDup = INVALID_HANDLE_VALUE;
  auto deleter = finally([&] {
    CloseHandleEx(hToken);
    CloseHandleEx(hTokenDup);
  });

  if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken) != TRUE) {
    return false;
  }

  if (DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification,
                       TokenPrimary, &hTokenDup) != TRUE) {
    return false;
  }
  if (!enableLPAC(hToken)) {
    fprintf(stderr, "EnableLPAC\n");
    return false;
  }
  return CreateProcessAsUserW(hToken, lpApplicationName, lpCommandLine,
                              lpProcessAttributes, lpThreadAttributes,
                              bInheritHandles, dwCreationFlags, lpEnvironment,
                              lpCurrentDirectory, lpStartupInfo,
                              lpProcessInformation) == TRUE;
}

} // namespace priv

#endif