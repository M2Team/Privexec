///
#include <process/process.hpp>
#include "app.hpp" // included windows.h
#include <Windowsx.h>
// C RunTime Header Files
#include <cstdlib>
#include <CommCtrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <Shlwapi.h>
#include <string>
#include "apputils.hpp"
#include "resource.h"

namespace priv {

// expand env
inline std::wstring ExpandEnv(const std::wstring &s) {
  auto len = ExpandEnvironmentStringsW(s.data(), nullptr, 0);
  if (len <= 0) {
    return s;
  }
  std::wstring s2(len + 1, L'\0');
  auto N = ExpandEnvironmentStringsW(s.data(), &s2[0], len + 1);
  s2.resize(N - 1);
  return s2;
}

int App::run(HINSTANCE hInstance) {
  hInst = hInstance;
  return (int)DialogBoxParamW(hInstance,
                              MAKEINTRESOURCE(IDD_APPLICATION_DIALOG), NULL,
                              App::WindowProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR WINAPI App::WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                               LPARAM lParam) {
  App *app{nullptr};
  if (message == WM_INITDIALOG) {
    auto app = reinterpret_cast<App *>(lParam);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    app->Initialize(hWnd);
  } else if ((app = GetThisFromHandle(hWnd)) != nullptr) {
    return app->MessageHandler(message, wParam, lParam);
  }
  return FALSE;
}

struct CapabilityName {
  LPCWSTR name;
  LPCWSTR value;
};

bool App::InitializeCapabilities() {
  appcas.hlview = GetDlgItem(hWnd, IDL_APPCONTAINER_LISTVIEW);
  appcas.hlpacbox = GetDlgItem(hWnd, IDC_LPACMODE);
  ListView_SetExtendedListViewStyleEx(appcas.hlview, LVS_EX_CHECKBOXES,
                                      LVS_EX_CHECKBOXES);
  LVCOLUMNW lvw;
  memset(&lvw, 0, sizeof(lvw));
  lvw.cx = 200;
  ListView_InsertColumn(appcas.hlview, 0, &lvw);
  const CapabilityName wncas[] = {
      {L"Basic - Internet Client", L"internetClient"},
      {L"Basic - Internet Client Server", L"internetClientServer"},
      {L"Basic - Private Network", L"privateNetworkClientServer"},
      {L"Basic - Pictures Library", L"picturesLibrary"},
      {L"Basic - Videos Library", L"videosLibrary"},
      {L"Basic - Music Library", L"musicLibrary"},
      {L"Basic - Documents Library", L"documentsLibrary"},
      {L"Basic - Shared User Certificates", L"sharedUserCertificates"},
      {L"Basic - Enterprise Authentication", L"enterpriseAuthentication"},
      {L"Basic - Removable Storage", L"removableStorage"},
      {L"Basic - Appointments", L"appointments"},
      {L"Basic - Contacts", L"contacts"},
      {L"LPAC - App Experience", L"lpacAppExperience"},
      {L"LPAC - Clipboard", L"lpacClipboard"},
      {L"LPAC - Com", L"lpacCom"},
      {L"LPAC - Crypto Services", L"lpacCryptoServices"},
      {L"LPAC - Enterprise Policy Change Notifications",
       L"lpacEnterprisePolicyChangeNotifications"},
      {L"LPAC - IME", L"lpacIME"},
      {L"LPAC - Identity Services", L"lpacIdentityServices"},
      {L"LPAC - Instrumentation", L"lpacInstrumentation"},
      {L"LPAC - Media", L"lpacMedia"},
      {L"LPAC - Package Manager Operation", L"lpacPackageManagerOperation"},
      {L"LPAC - Payments", L"lpacPayments"},
      {L"LPAC - PnP Notifications", L"lpacPnPNotifications"},
      {L"LPAC - Printing", L"lpacPrinting"},
      {L"LPAC - Services Management", L"lpacServicesManagement"},
      {L"LPAC - Session Management", L"lpacSessionManagement"},
      {L"LPAC - Web Platform", L"lpacWebPlatform"},
      {L"accessoryManager", L"accessoryManager"},
      {L"activateAsUser", L"activateAsUser"},
      {L"activity", L"activity"},
      {L"activityData", L"activityData"},
      {L"activitySystem", L"activitySystem"},
      {L"allAppMods", L"allAppMods"},
      {L"allJoyn", L"allJoyn"},
      {L"allowElevation", L"allowElevation"},
      {L"appBroadcast", L"appBroadcast"},
      {L"appBroadcastServices", L"appBroadcastServices"},
      {L"appBroadcastSettings", L"appBroadcastSettings"},
      {L"appCaptureServices", L"appCaptureServices"},
      {L"appCaptureSettings", L"appCaptureSettings"},
      {L"appDiagnostics", L"appDiagnostics"},
      {L"appLicensing", L"appLicensing"},
      {L"appManagementSystem", L"appManagementSystem"},
      {L"applicationDefaults", L"applicationDefaults"},
      {L"applicationViewActivation", L"applicationViewActivation"},
      {L"appointmentsSystem", L"appointmentsSystem"},
      {L"audioDeviceConfiguration", L"audioDeviceConfiguration"},
      {L"backgroundMediaPlayback", L"backgroundMediaPlayback"},
      {L"backgroundMediaRecording", L"backgroundMediaRecording"},
      {L"backgroundVoIP", L"backgroundVoIP"},
      {L"biometricSystem", L"biometricSystem"},
      {L"blockedChatMessages", L"blockedChatMessages"},
      {L"bluetooth", L"bluetooth"},
      {L"bluetooth.genericAttributeProfile",
       L"bluetooth.genericAttributeProfile"},
      {L"bluetooth.rfcomm", L"bluetooth.rfcomm"},
      {L"bluetoothAdapter", L"bluetoothAdapter"},
      {L"bluetoothDeviceSettings", L"bluetoothDeviceSettings"},
      {L"bluetoothSync", L"bluetoothSync"},
      {L"broadFileSystemAccess", L"broadFileSystemAccess"},
      {L"browserAppList", L"browserAppList"},
      {L"browserCredentials", L"browserCredentials"},
      {L"cameraProcessingExtension", L"cameraProcessingExtension"},
      {L"capabilityAccessConsentDeviceSettings",
       L"capabilityAccessConsentDeviceSettings"},
      {L"cellularData", L"cellularData"},
      {L"cellularDeviceControl", L"cellularDeviceControl"},
      {L"cellularDeviceIdentity", L"cellularDeviceIdentity"},
      {L"cellularMessaging", L"cellularMessaging"},
      {L"chat", L"chat"},
      {L"chatSystem", L"chatSystem"},
      {L"childWebContent", L"childWebContent"},
      {L"cloudExperienceHost", L"cloudExperienceHost"},
      {L"cloudStore", L"cloudStore"},
      {L"codeGeneration", L"codeGeneration"},
      {L"comPort", L"comPort"},
      {L"componentUiInWebContent", L"componentUiInWebContent"},
      {L"confirmAppClose", L"confirmAppClose"},
      {L"constrainedImpersonation", L"constrainedImpersonation"},
      {L"contactsSystem", L"contactsSystem"},
      {L"contentDeliveryManagerSettings", L"contentDeliveryManagerSettings"},
      {L"contentRestrictions", L"contentRestrictions"},
      {L"coreShell", L"coreShell"},
      {L"cortanaPermissions", L"cortanaPermissions"},
      {L"cortanaSettings", L"cortanaSettings"},
      {L"cortanaSpeechAccessory", L"cortanaSpeechAccessory"},
      {L"cortanaSurface", L"cortanaSurface"},
      {L"curatedTileCollections", L"curatedTileCollections"},
      {L"dateAndTimeDeviceSettings", L"dateAndTimeDeviceSettings"},
      {L"developerSettings", L"developerSettings"},
      {L"developmentModeNetwork", L"developmentModeNetwork"},
      {L"deviceEncryptionManagement", L"deviceEncryptionManagement"},
      {L"deviceIdentityManagement", L"deviceIdentityManagement"},
      {L"deviceLockManagement", L"deviceLockManagement"},
      {L"deviceManagementAdministrator", L"deviceManagementAdministrator"},
      {L"deviceManagementDeviceLockPolicies",
       L"deviceManagementDeviceLockPolicies"},
      {L"deviceManagementDmAccount", L"deviceManagementDmAccount"},
      {L"deviceManagementEmailAccount", L"deviceManagementEmailAccount"},
      {L"deviceManagementFoundation", L"deviceManagementFoundation"},
      {L"deviceManagementRegistration", L"deviceManagementRegistration"},
      {L"deviceManagementWapSecurityPolicies",
       L"deviceManagementWapSecurityPolicies"},
      {L"devicePortalProvider", L"devicePortalProvider"},
      {L"deviceProvisioningAdministrator", L"deviceProvisioningAdministrator"},
      {L"deviceUnlock", L"deviceUnlock"},
      {L"diagnostics", L"diagnostics"},
      {L"displayDeviceSettings", L"displayDeviceSettings"},
      {L"dualSimTiles", L"dualSimTiles"},
      {L"email", L"email"},
      {L"emailSystem", L"emailSystem"},
      {L"enterpriseCloudSSO", L"enterpriseCloudSSO"},
      {L"enterpriseDataPolicy", L"enterpriseDataPolicy"},
      {L"enterpriseDeviceLockdown", L"enterpriseDeviceLockdown"},
      {L"eraApplication", L"eraApplication"},
      {L"exclusiveResource", L"exclusiveResource"},
      {L"expandedResources", L"expandedResources"},
      {L"extendedBackgroundTaskTime", L"extendedBackgroundTaskTime"},
      {L"extendedExecutionBackgroundAudio",
       L"extendedExecutionBackgroundAudio"},
      {L"extendedExecutionCritical", L"extendedExecutionCritical"},
      {L"extendedExecutionUnconstrained", L"extendedExecutionUnconstrained"},
      {L"featureStagingInfo", L"featureStagingInfo"},
      {L"feedbackLogCollection", L"feedbackLogCollection"},
      {L"firstSignInSettings", L"firstSignInSettings"},
      {L"flashPlayerSupport", L"flashPlayerSupport"},
      {L"fullFileSystemAccess", L"fullFileSystemAccess"},
      {L"gameBarServices", L"gameBarServices"},
      {L"gameConfigStoreManagement", L"gameConfigStoreManagement"},
      {L"gameList", L"gameList"},
      {L"gameMonitor", L"gameMonitor"},
      {L"gazeInput", L"gazeInput"},
      {L"globalMediaControl", L"globalMediaControl"},
      {L"graphicsCapture", L"graphicsCapture"},
      {L"hevcPlayback", L"hevcPlayback"},
      {L"hfxSystem", L"hfxSystem"},
      {L"hidTelephony", L"hidTelephony"},
      {L"holographicCompositor", L"holographicCompositor"},
      {L"holographicCompositorSystem", L"holographicCompositorSystem"},
      {L"humanInterfaceDevice", L"humanInterfaceDevice"},
      {L"imeSystem", L"imeSystem"},
      {L"inProcessMediaExtension", L"inProcessMediaExtension"},
      {L"indexedContent", L"indexedContent"},
      {L"inputForegroundObservation", L"inputForegroundObservation"},
      {L"inputInjection", L"inputInjection"},
      {L"inputInjectionBrokered", L"inputInjectionBrokered"},
      {L"inputObservation", L"inputObservation"},
      {L"inputSettings", L"inputSettings"},
      {L"inputSuppression", L"inputSuppression"},
      {L"interopServices", L"interopServices"},
      {L"keyboardDeviceSettings", L"keyboardDeviceSettings"},
      {L"kinectAudio", L"kinectAudio"},
      {L"kinectExpressions", L"kinectExpressions"},
      {L"kinectFace", L"kinectFace"},
      {L"kinectGamechat", L"kinectGamechat"},
      {L"kinectRequired", L"kinectRequired"},
      {L"kinectVideo", L"kinectVideo"},
      {L"kinectVision", L"kinectVision"},
      {L"languageAndRegionDeviceSettings", L"languageAndRegionDeviceSettings"},
      {L"languageSettings", L"languageSettings"},
      {L"liveIdService", L"liveIdService"},
      {L"localExperienceInternal", L"localExperienceInternal"},
      {L"location", L"location"},
      {L"locationHistory", L"locationHistory"},
      {L"locationSystem", L"locationSystem"},
      {L"lockScreenCreatives", L"lockScreenCreatives"},
      {L"lowLevel", L"lowLevel"},
      {L"lowLevelDevices", L"lowLevelDevices"},
      {L"microphone", L"microphone"},
      {L"microsoftEdgeRemoteDebugging", L"microsoftEdgeRemoteDebugging"},
      {L"mixedRealityEnvironmentInternal", L"mixedRealityEnvironmentInternal"},
      {L"mmsTransportSystem", L"mmsTransportSystem"},
      {L"multiplaneOverlay", L"multiplaneOverlay"},
      {L"muma", L"muma"},
      {L"networkConnectionManagerProvisioning",
       L"networkConnectionManagerProvisioning"},
      {L"networkDataPlanProvisioning", L"networkDataPlanProvisioning"},
      {L"networkDataUsageManagement", L"networkDataUsageManagement"},
      {L"networkDeviceSettings", L"networkDeviceSettings"},
      {L"networkDiagnostics", L"networkDiagnostics"},
      {L"networkingVpnProvider", L"networkingVpnProvider"},
      {L"nfcSystem", L"nfcSystem"},
      {L"notificationsDeviceSettings", L"notificationsDeviceSettings"},
      {L"objects3D", L"objects3D"},
      {L"oemDeployment", L"oemDeployment"},
      {L"oemPublicDirectory", L"oemPublicDirectory"},
      {L"offlineMapsManagement", L"offlineMapsManagement"},
      {L"oneProcessVoIP", L"oneProcessVoIP"},
      {L"optical", L"optical"},
      {L"packageContents", L"packageContents"},
      {L"packageManagement", L"packageManagement"},
      {L"packagePolicySystem", L"packagePolicySystem"},
      {L"packageQuery", L"packageQuery"},
      {L"perceptionMonitoring", L"perceptionMonitoring"},
      {L"perceptionSensorsExperimental", L"perceptionSensorsExperimental"},
      {L"perceptionSystem", L"perceptionSystem"},
      {L"personalizationDeviceSettings", L"personalizationDeviceSettings"},
      {L"phoneCall", L"phoneCall"},
      {L"phoneCallHistory", L"phoneCallHistory"},
      {L"phoneCallHistoryPublic", L"phoneCallHistoryPublic"},
      {L"phoneCallHistorySystem", L"phoneCallHistorySystem"},
      {L"phoneCallSystem", L"phoneCallSystem"},
      {L"pointOfService", L"pointOfService"},
      {L"powerDeviceSettings", L"powerDeviceSettings"},
      {L"preemptiveCamera", L"preemptiveCamera"},
      {L"previewHfx", L"previewHfx"},
      {L"previewInkWorkspace", L"previewInkWorkspace"},
      {L"previewPenWorkspace", L"previewPenWorkspace"},
      {L"previewStore", L"previewStore"},
      {L"previewUiComposition", L"previewUiComposition"},
      {L"projectionDeviceSettings", L"projectionDeviceSettings"},
      {L"protectedApp", L"protectedApp"},
      {L"proximity", L"proximity"},
      {L"radios", L"radios"},
      {L"recordedCallsFolder", L"recordedCallsFolder"},
      {L"regionSettings", L"regionSettings"},
      {L"registryRead", L"registryRead"},
      {L"relatedPackages", L"relatedPackages"},
      {L"remoteFileAccess", L"remoteFileAccess"},
      {L"remotePassportAuthentication", L"remotePassportAuthentication"},
      {L"remoteSystem", L"remoteSystem"},
      {L"resetPhone", L"resetPhone"},
      {L"runFullTrust", L"runFullTrust"},
      {L"screenDuplication", L"screenDuplication"},
      {L"secondaryAuthenticationFactor", L"secondaryAuthenticationFactor"},
      {L"secureAssessment", L"secureAssessment"},
      {L"sensors.custom", L"sensors.custom"},
      {L"serialCommunication", L"serialCommunication"},
      {L"sessionImpersonation", L"sessionImpersonation"},
      {L"settingSyncConfiguration", L"settingSyncConfiguration"},
      {L"shellDisplayManagement", L"shellDisplayManagement"},
      {L"shellExperience", L"shellExperience"},
      {L"shellExperienceComposer", L"shellExperienceComposer"},
      {L"slapiQueryLicenseValue", L"slapiQueryLicenseValue"},
      {L"smbios", L"smbios"},
      {L"sms", L"sms"},
      {L"smsSend", L"smsSend"},
      {L"smsSystem", L"smsSystem"},
      {L"smsTransportSystem", L"smsTransportSystem"},
      {L"spatialPerception", L"spatialPerception"},
      {L"startScreenManagement", L"startScreenManagement"},
      {L"storeAppInstall", L"storeAppInstall"},
      {L"storeAppInstallation", L"storeAppInstallation"},
      {L"storeConfiguration", L"storeConfiguration"},
      {L"storeLicenseManagement", L"storeLicenseManagement"},
      {L"storeOptionalPackageInstallManagement",
       L"storeOptionalPackageInstallManagement"},
      {L"systemDialog", L"systemDialog"},
      {L"systemDialogEmergency", L"systemDialogEmergency"},
      {L"systemManagement", L"systemManagement"},
      {L"systemRegistrar", L"systemRegistrar"},
      {L"targetedContent", L"targetedContent"},
      {L"targetedContentSubscription", L"targetedContentSubscription"},
      {L"teamEditionExperience", L"teamEditionExperience"},
      {L"telemetryData", L"telemetryData"},
      {L"terminalPowerManagement", L"terminalPowerManagement"},
      {L"thumbnailCache", L"thumbnailCache"},
      {L"timezone", L"timezone"},
      {L"uiAutomationSystem", L"uiAutomationSystem"},
      {L"unzipFile", L"unzipFile"},
      {L"updateAndSecurityDeviceSettings", L"updateAndSecurityDeviceSettings"},
      {L"usb", L"usb"},
      {L"userAccountInformation", L"userAccountInformation"},
      {L"userDataAccountSetup", L"userDataAccountSetup"},
      {L"userDataAccountsProvider", L"userDataAccountsProvider"},
      {L"userDataSystem", L"userDataSystem"},
      {L"userDataTasks", L"userDataTasks"},
      {L"userDataTasksSystem", L"userDataTasksSystem"},
      {L"userManagementSystem", L"userManagementSystem"},
      {L"userNotificationListener", L"userNotificationListener"},
      {L"userPrincipalName", L"userPrincipalName"},
      {L"userSigninSupport", L"userSigninSupport"},
      {L"userSystemId", L"userSystemId"},
      {L"userWebAccounts", L"userWebAccounts"},
      {L"visualElementsSystem", L"visualElementsSystem"},
      {L"visualVoiceMail", L"visualVoiceMail"},
      {L"vmWorkerProcess", L"vmWorkerProcess"},
      {L"voipCall", L"voipCall"},
      {L"walletSystem", L"walletSystem"},
      {L"webPlatformMediaExtension", L"webPlatformMediaExtension"},
      {L"webcam", L"webcam"},
      {L"wiFiControl", L"wiFiControl"},
      {L"wiFiDirect", L"wiFiDirect"},
      {L"windowManagement", L"windowManagement"},
      {L"windowManagementSystem", L"windowManagementSystem"},
      {L"windowsHelloCredentialAccess", L"windowsHelloCredentialAccess"},
      {L"windowsPerformanceCounters", L"windowsPerformanceCounters"},
      {L"xboxAccessoryManagement", L"xboxAccessoryManagement"},
      {L"xboxBroadcaster", L"xboxBroadcaster"},
      {L"xboxGameSpeechWindow", L"xboxGameSpeechWindow"},
      {L"xboxLiveAuthenticationProvider", L"xboxLiveAuthenticationProvider"},
      {L"xboxSystemApplicationClipQuery", L"xboxSystemApplicationClipQuery"},
      {L"xboxTrackingStream", L"xboxTrackingStream"}
      //
  };
  constexpr std::size_t wnlen = sizeof(wncas) / sizeof(CapabilityName);
  for (std::size_t n = wnlen; n > 0; n--) {
    const auto &w = wncas[n - 1];
    LVITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.pszText = const_cast<LPWSTR>(w.name);
    item.lParam = (LPARAM)w.value;
    ListView_InsertItem(appcas.hlview, &item);
  }
  ListView_SetColumnWidth(appcas.hlview, 0, LVSCW_AUTOSIZE_USEHEADER);
  return true;
}

bool App::Initialize(HWND window) {
  hWnd = window;
  HICON icon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPLICATION_ICON));
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
  // Drag support
  ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
  ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
  ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
  ::DragAcceptFiles(hWnd, TRUE);

  auto elevated = priv::IsUserAdministratorsGroup();
  if (elevated) {
    // Update title when app run as admin
    WCHAR title[256];
    auto N = GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    wcscat_s(title, L" [Administrator]");
    SetWindowTextW(hWnd, title);
  }
  box = GetDlgItem(hWnd, IDC_USER_COMBOX);

  box.Append(priv::ProcessAppContainer, L"AppContainer");
  box.Append(priv::ProcessMandatoryIntegrityControl,
             L"Mandatory Integrity Control");
  if (elevated) {
    box.Append(priv::ProcessNoElevated, L"Not Elevated (UAC)", true);
    box.Append(priv::ProcessElevated, L"Administrator");
    box.Append(priv::ProcessSystem, L"System");
    box.Append(priv::ProcessTrustedInstaller, L"TrustedInstaller");
  } else {
    box.Append(priv::ProcessNoElevated, L"Not Elevated (UAC)");
    box.Append(priv::ProcessElevated, L"Administrator", true);
  }
  HMENU hSystemMenu = ::GetSystemMenu(hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_PRIVEXEC_ABOUT,
              L"About Privexec\tAlt+F1");
  cmd.hInput = GetDlgItem(hWnd, IDC_COMMAND_COMBOX);
  cmd.hButton = GetDlgItem(hWnd, IDB_COMMAND_TARGET);
  AppAliasInitialize(cmd.hInput, alias); // Initialize app alias
  ///
  cwd.hInput = GetDlgItem(hWnd, IDE_APPSTARTUP);
  cwd.hButton = GetDlgItem(hWnd, IDB_APPSTARTUP);
  appx.hInput = GetDlgItem(hWnd, IDE_APPCONTAINER_APPMANIFEST);
  appx.hButton = GetDlgItem(hWnd, IDB_APPCONTAINER_BUTTON);
  InitializeCapabilities();
  SelChanged(); /// disable appcontainer.

  return true;
}

bool App::SelChanged() {
  if (box.IsMatch(priv::ProcessAppContainer)) {
    appx.Update(L"");
    appx.Visible(TRUE);
    appcas.Visible(TRUE);
  } else {
    appx.Update(L"AppxManifest.xml or Package.appxmanifest");
    appx.Visible(FALSE);
    appcas.Visible(FALSE);
  }
  return true;
}

std::wstring App::ResolveCMD() {
  auto cmd_ = cmd.Content();
  auto it = alias.find(cmd_);
  if (it != alias.end()) {
    return ExpandEnv(it->second);
  }
  return ExpandEnv(cmd_);
}

std::wstring App::ResolveCWD() {
  auto cwd_ = ExpandEnv(cwd.Content());
  if (!cwd_.empty() &&
      (GetFileAttributesW(cwd_.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    /// resolved cwd  valid
    return cwd_;
  }
  auto N = GetCurrentDirectoryW(0, nullptr);
  if (N <= 0) {
    return L"";
  }
  cwd_.resize(N + 1);
  N = GetCurrentDirectoryW(N + 1, &cwd_[0]);
  cwd_.resize(N);
  return cwd_;
}

bool App::AppExecute() {
  constexpr const wchar_t *ei =
      L"Ask for help with this issue. \nVisit: <a "
      L"href=\"https://github.com/M2Team/Privexec/issues\">Privexec Issues</a>";
  auto appindex = box.AppIndex();
  auto cmd_ = ResolveCMD();
  if (cmd_.empty()) {
    utils::PrivMessageBox(hWnd, L"Please input command line", L"command empty",
                          ei, utils::kFatalWindow);
    return false;
  }
  auto cwd_ = ResolveCWD(); // app startup directory

  if (appindex == priv::ProcessAppContainer) {
    //// TODO app container.
    auto cas = appcas.Capabilities();
    auto xml = ExpandEnv(appx.Content());
    if (!xml.empty() && !priv::MergeFromAppmanifest(xml, cas)) {
      auto ec = priv::error_code::lasterror();
      utils::PrivMessageBox(hWnd, L"Privexec appmanifest error",
                            ec.message.c_str(), ei, utils::kFatalWindow);
      return false;
    }
    priv::appcontainer p(cmd_);
    p.enablelpac(appcas.IsLowPrivilegeAppContainer());
    p.cwd().assign(cwd_);
    if (!p.initialize(cas) || !p.execute()) {
      auto ec = priv::error_code::lasterror();
      if (!p.message().empty()) {
        ec.message.append(L" (").append(p.message()).append(L")");
      }
      utils::PrivMessageBox(hWnd,
                            L"Privexec create appconatiner process failed",
                            ec.message.c_str(), ei, utils::kFatalWindow);
      return false;
    }
    return true;
  }

  priv::process p(cmd_);
  p.cwd().assign(cwd_);
  if (!p.execute(appindex)) {
    if (appindex == priv::ProcessElevated &&
        !priv::IsUserAdministratorsGroup()) {
      return false;
    }
    auto ec = priv::error_code::lasterror();
    if (!p.message().empty()) {
      ec.message.append(L" (").append(p.message()).append(L")");
    }
    utils::PrivMessageBox(hWnd, L"Privexec create process failed",
                          ec.message.c_str(), ei, utils::kFatalWindow);
    return false;
  }
  return true;
}

bool App::AppLookupExecute() {
  const utils::filter_t filters[] = {
      {L"Windows Execute(*.exe;*.com;*.bat)", L"*.exe;*.com;*.bat"},
      {L"All Files (*.*)", L"*.*"}};
  auto exe =
      utils::PrivFilePicker(hWnd, L"Privexec: Select Execute", filters, 2);
  if (exe) {
    cmd.Update(*exe);
    return true;
  }
  return false;
}

bool App::AppLookupManifest() {
  const utils::filter_t filters[] = {
      {L"Windows Appxmanifest (*.appxmanifest;*.xml)", L"*.appxmanifest;*.xml"},
      {L"All Files (*.*)", L"*.*"}};
  auto xml =
      utils::PrivFilePicker(hWnd, L"Privexec: Select AppManifest", filters, 2);
  if (xml) {
    appx.Update(*xml);
    return true;
  }
  return false;
}
bool App::AppLookupCWD() {
  auto folder =
      utils::PrivFolderPicker(hWnd, L"Privexec: Select App Launcher Folder");
  if (folder) {
    cwd.Update(*folder);
    return true;
  }
  return false;
}

bool App::DropFiles(WPARAM wParam, LPARAM lParam) {
  const LPCWSTR appmanifest[] = {L".xml", L".appxmanifest"};
  HDROP hDrop = (HDROP)wParam;
  UINT nfilecounts = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
  WCHAR dfile[4096] = {0};
  for (UINT i = 0; i < nfilecounts; i++) {
    DragQueryFileW(hDrop, i, dfile, 4096);
    if (PathFindSuffixArrayW(dfile, appmanifest, ARRAYSIZE(appmanifest))) {
      if (appx.IsVisible()) {
        appx.Update(dfile);
      }
      continue;
    }
    cmd.Update(dfile);
  }
  DragFinish(hDrop);
  return true;
}

INT_PTR App::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) {
  constexpr const wchar_t *appurl =
      L"For more information about this tool. \nVisit: <a "
      L"href=\"https://github.com/M2Team/Privexec\">Privexec</a>\nVisit: <a "
      L"href=\"https://forcemz.net/\">forcemz.net</a>";
  switch (message) {
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC:
    return (INT_PTR)CreateSolidBrush(RGB(255, 255, 255));
  case WM_DROPFILES:
    DropFiles(wParam, lParam);
    return TRUE;
  case WM_SYSCOMMAND:
    switch (LOWORD(wParam)) {
    case IDM_PRIVEXEC_ABOUT:
      utils::PrivMessageBox(
          hWnd, L"About Privexec",
          L"Prerelease:"
          L" " PRIVEXEC_BUILD_VERSION L"\nCopyright \xA9 2018, Force "
          L"Charlie. All Rights Reserved.",
          appurl, utils::kAboutWindow);
      break;
    default:
      break;
    }
    break;
  case WM_COMMAND: {
    // WM_COMMAND
    switch (LOWORD(wParam)) {
    case IDC_USER_COMBOX:
      if (HIWORD(wParam) == CBN_SELCHANGE) {
        SelChanged();
      }
      return TRUE;
    case IDB_COMMAND_TARGET: /// lookup command
      AppLookupExecute();
      return TRUE;
    case IDB_APPSTARTUP: // select startup dir
      AppLookupCWD();
      return TRUE;
    case IDB_APPCONTAINER_BUTTON: // select appmanifest file
      AppLookupManifest();
      return TRUE;
    case IDB_EXECUTE_BUTTON: {
      auto hExecute = reinterpret_cast<HWND>(lParam);
      EnableWindow(hExecute, FALSE);
      AppExecute();
      EnableWindow(hExecute, TRUE);
    }
      return TRUE;
    default:
      return FALSE;
    }

  } break;
  case WM_CLOSE:
    DestroyWindow(hWnd);
    return TRUE;
  case WM_DESTROY:
    PostQuitMessage(0);
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

} // namespace priv
