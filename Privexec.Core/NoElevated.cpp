#include "stdafx.h"
#include <string>
#include <taskschd.h>
#include <comdef.h>
#include <Sddl.h>
#include <Shlwapi.h>
#include "Privexec.Core.hpp"

template <class T> class SmartCOMPtr {
public:
  SmartCOMPtr() { ptr = NULL; }
  SmartCOMPtr(T *p) {
    ptr = p;
    if (ptr != NULL)
      ptr->AddRef();
  }
  SmartCOMPtr(const SmartCOMPtr<T> &sptr) {
    ptr = sptr.ptr;
    if (ptr != NULL)
      ptr->AddRef();
  }
  T **operator&() { return &ptr; }
  T *operator->() { return ptr; }
  T *operator=(T *p) {
    if (*this != p) {
      ptr = p;
      if (ptr != NULL)
        ptr->AddRef();
    }
    return *this;
  }
  operator T *() const { return ptr; }
  template <class I> HRESULT QueryInterface(REFCLSID rclsid, I **pp) {
    if (pp != NULL) {
      return ptr->QueryInterface(rclsid, (void **)pp);
    } else {
      return E_FAIL;
    }
  }
  HRESULT CoCreateInstance(REFCLSID clsid, IUnknown *pUnknown,
                           REFIID interfaceId,
                           DWORD dwClsContext = CLSCTX_ALL) {
    HRESULT hr = ::CoCreateInstance(clsid, pUnknown, dwClsContext, interfaceId,
                                    (void **)&ptr);
    return hr;
  }
  ~SmartCOMPtr() {
    if (ptr != NULL)
      ptr->Release();
  }

private:
  T *ptr;
};

namespace priv {
bool CreateNoElevatedProcess(LPWSTR pszCmdline, DWORD &dwProcessId) {
  if (!IsUserAdministratorsGroup()) {
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    auto bResult = CreateProcessW(nullptr, pszCmdline, nullptr, nullptr, FALSE,
                                  0, nullptr, nullptr, &si, &pi);
    if (bResult) {
      dwProcessId = pi.dwProcessId;
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      return true;
    }
    return false;
  }
  /// --- UAC No Elevated
  SmartCOMPtr<ITaskService> iTaskService;
  SmartCOMPtr<ITaskFolder> iTaskFolder;
  SmartCOMPtr<ITaskDefinition> iTask;
  SmartCOMPtr<IRegistrationInfo> iRegInfo;
  SmartCOMPtr<IPrincipal> iPrin;
  SmartCOMPtr<ITaskSettings> iSettings;
  SmartCOMPtr<ITriggerCollection> iTriggerCollection;
  SmartCOMPtr<ITrigger> iTrigger;
  SmartCOMPtr<IRegistrationTrigger> iRegistrationTrigger;
  SmartCOMPtr<IActionCollection> iActionCollection;
  SmartCOMPtr<IAction> iAction;
  SmartCOMPtr<IExecAction> iExecAction;
  SmartCOMPtr<IRegisteredTask> iRegisteredTask;

  LPWSTR pszArgs = PathGetArgsW(pszCmdline);
  std::wstring file;
  if (pszArgs != nullptr) {
    file.assign(pszCmdline, pszArgs);
    auto iter = file.rbegin();
    for (; iter != file.rend(); iter++) {
      if (*iter != L' ' && *iter != L'\t')
        break;
    }
    file.resize(file.size() - (iter - file.rbegin()));
  } else {
    file.assign(pszCmdline);
  }

  LPCWSTR pszTaskName = L"Privexec.Core.UAC.NoElevated.Task";
  if (iTaskService.CoCreateInstance(CLSID_TaskScheduler, nullptr,
                                    IID_ITaskService,
                                    CLSCTX_INPROC_SERVER) != S_OK) {
    return false;
  }
  if (iTaskService->Connect(_variant_t(), _variant_t(), _variant_t(),
                            _variant_t()) != S_OK) {
    return false;
  }
  if (iTaskService->GetFolder(_bstr_t(L"\\"), &iTaskFolder) != S_OK) {
    return false;
  }
  iTaskFolder->DeleteTask(_bstr_t(pszTaskName), 0);
  if (iTaskService->NewTask(0, &iTask) != S_OK) {
    return false;
  }
  if (iTask->get_RegistrationInfo(&iRegInfo) != S_OK) {
    return false;
  }
  if (iRegInfo->put_Author(L"Privexec.Core.Team") != S_OK) {
    return false;
  }
  if (iTask->get_Principal(&iPrin) != S_OK) {
    return false;
  }
  if (iPrin->put_Id(_bstr_t(L"PrivexecCoreRunLimitUser_Principal")) != S_OK) {
    return false;
  }
  if (iPrin->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN) != S_OK) {
    return false;
  }
  if (iPrin->put_RunLevel(TASK_RUNLEVEL_LUA) != S_OK) {
    return false;
  }
  if (iTask->get_Settings(&iSettings) != S_OK) {
    return false;
  }
  if (iSettings->put_StartWhenAvailable(VARIANT_BOOL(true)) != S_OK) {
    return false;
  }
  if (iTask->get_Triggers(&iTriggerCollection) != S_OK) {
    return false;
  }
  if (iTriggerCollection->Create(TASK_TRIGGER_REGISTRATION, &iTrigger) !=
      S_OK) {
    return false;
  }
  if (iTrigger->QueryInterface(IID_IRegistrationTrigger,
                               (void **)&iRegistrationTrigger) != S_OK) {
    return false;
  }
  if (iRegistrationTrigger->put_Id(
          _bstr_t(L"PrivexecCoreRunLimitUser_Trigger")) != S_OK) {
    return false;
  }
  if (iRegistrationTrigger->put_Delay(L"PT0S") != S_OK) {
    return false;
  }
  if (iTask->get_Actions(&iActionCollection) != S_OK) {
    return false;
  }
  if (iActionCollection->Create(TASK_ACTION_EXEC, &iAction) != S_OK) {
    return false;
  }
  if (iAction->QueryInterface(IID_IExecAction, (void **)&iExecAction) != S_OK) {
    return false;
  }
  if (iExecAction->put_Path(_bstr_t(file.data())) != S_OK) {
    return false;
  }
  if (pszArgs) {
    iExecAction->put_Arguments(_bstr_t(pszArgs));
  }

  return (iTaskFolder->RegisterTaskDefinition(
              _bstr_t(pszTaskName), iTask, TASK_CREATE_OR_UPDATE, _variant_t(),
              _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""),
              &iRegisteredTask) == S_OK);
}
} // namespace priv