#!/usr/bin/env pwsh

# https://github.com/winsiderss/systeminformer/blob/master/SystemInformer/resources/capslist.txt

$capslistFile = Join-Path $PSScriptRoot -ChildPath "capabilities.txt"
$generated = Join-Path $PSScriptRoot -ChildPath "capabilities.hpp"
Write-Host "source file: $capslistFile"

# {L"Basic - Videos Library", L"videosLibrary"},
# {L"Basic - Music Library", L"musicLibrary"},
# {L"Basic - Documents Library", L"documentsLibrary"},
# {L"Basic - Shared User Certificates", L"sharedUserCertificates"},
# {L"Basic - Enterprise Authentication", L"enterpriseAuthentication"},
# {L"Basic - Removable Storage", L"removableStorage"},
# {L"Basic - Appointments", L"appointments"},
# {L"Basic - Contacts", L"contacts"},

$basicCaps = [ordered]@{
    "internetClient"             = "Basic - Internet Client";
    "internetClientServer"       = "Basic - Internet Client Server";
    "privateNetworkClientServer" = "Basic - Private Network Client Server";
    "picturesLibrary"            = "Basic - Pictures Library";
    "musicLibrary"               = "Basic - Music Library";
    "documentsLibrary"           = "Basic - Documents Library";
    "sharedUserCertificates"     = "Basic - Shared User Certificates";
    "enterpriseAuthentication"   = "Basic - Enterprise Authentication";
    "removableStorage"           = "Basic - Removable Storage";
    "appointments"               = "Basic - Appointments";
    "contacts"                   = "Basic - Contacts";
}

$caps = Get-Content -Path $capslistFile
$buffer = [System.Text.StringBuilder]::new()


## BEGIN
[void]$buffer.Append(@"
#ifndef WSUDO_CAPABILITIES_HPP
#define WSUDO_CAPABILITIES_HPP

// SEE:
// https://github.com/googleprojectzero/sandbox-attacksurface-analysis-tools/blob/main/NtApiDotNet/SecurityCapabilities.cs
// https://github.com/winsiderss/systeminformer/blob/master/SystemInformer/resources/capslist.txt

namespace priv {
constexpr const wchar_t *KnownCapabilityNames[] = {

"@)

foreach ($c in $caps) {
    [void]$buffer.Append("    L`"$c`",`n")
}
[void]$buffer.Append(@"
};
// kv
struct CapabilityName {
  const wchar_t *name;
  const wchar_t *value;
};
constexpr const CapabilityName capabilityNames[] = {

"@)

foreach ($key in $basicCaps.Keys) {
    $value = $basicCaps[$key]
    [void]$buffer.Append("    {L`"$value`", L`"$key`"},`n")
}

foreach ($c in $caps) {
    if ($c.StartsWith("lpac")) {
        $Name = $c.Substring(4)
        [void]$buffer.Append("    {L`"LPAC - $Name`", L`"$c`"},`n")
    }
}

foreach ($c in $caps) {
    if ($c.StartsWith("lpac") -eq $false -and $basicCaps.Contains($c) -eq $false) {
        [void]$buffer.Append("    {L`"$c`", L`"$c`"},`n")
    }
}

[void]$buffer.Append(@"
};
} // namespace priv

#endif
"@)

$buffer.ToString() | Out-File -Encoding utf8NoBOM -NoNewline -FilePath $generated