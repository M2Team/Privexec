/// uac no elevated
#ifndef PRIVEXEC_NOELEVATED_HPP
#define PRIVEXEC_NOELEVATED_HPP
#include <string>
#include <taskschd.h>
#include <comdef.h>
#include <Sddl.h>
#include <Shlwapi.h>
#include "processfwd.hpp"
#include "comutils.hpp"
#include "systemhelper.hpp"

namespace priv {
bool process::noelevatedexec2() {
  if (!IsUserAdministratorsGroup()) {
    return execute();
  }
  /// Enable Privilege TODO

  return true;
}
bool process::noelevatedexec() {
  if (!IsUserAdministratorsGroup()) {
    return execute();
  }
  comptr<ITaskService> iTaskService;
  comptr<ITaskFolder> iTaskFolder;
  comptr<ITaskDefinition> iTask;
  comptr<IRegistrationInfo> iRegInfo;
  comptr<IPrincipal> iPrin;
  comptr<ITaskSettings> iSettings;
  comptr<ITriggerCollection> iTriggerCollection;
  comptr<ITrigger> iTrigger;
  comptr<IRegistrationTrigger> iRegistrationTrigger;
  comptr<IActionCollection> iActionCollection;
  comptr<IAction> iAction;
  comptr<IExecAction> iExecAction;
  comptr<IRegisteredTask> iRegisteredTask;

  std::wstring file;
  auto pszcmd = cmd_.data();
  auto pszargs = PathGetArgsW(pszcmd);
  if (pszargs != nullptr) {
    file.assign(pszcmd, pszargs);
    auto iter = file.rbegin();
    for (; iter != file.rend(); iter++) {
      if (*iter != L' ' && *iter != L'\t')
        break;
    }
    // remove end space char
    file.resize(file.size() - (iter - file.rbegin()));
  } else {
    file.assign(cmd_);
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
  if (iExecAction->put_WorkingDirectory(bstr_t(Castwstr(cwd_))) != S_OK) {
    return false;
  }
  if (pszargs) {
    iExecAction->put_Arguments(_bstr_t(pszargs));
  }
  return (iTaskFolder->RegisterTaskDefinition(
              _bstr_t(pszTaskName), iTask, TASK_CREATE_OR_UPDATE, _variant_t(),
              _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""),
              &iRegisteredTask) == S_OK);
}
} // namespace priv

#endif