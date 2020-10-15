#!/usr/bin/env pwsh
$PrivexeRoot = Split-Path -Parent $PSScriptRoot

$WSudoExe = Join-Path -Path $PrivexeRoot -ChildPath "wsudo.exe"

if (!(Test-Path $WSudoExe)) {
    Write-Host -ForegroundColor Red "$WSudoExe not exists"
    Exit 1
}

# https://powershell.org/forums/topic/cant-set-new-itemproperty-to-registry-path-containing-astrix/

$hive = [Microsoft.Win32.RegistryKey]::OpenBaseKey('ClassesRoot', 'Default')
$subKey = $hive.CreateSubKey('*\shell\RunAsSystem', $true)
$subkey.SetValue($null, 'Run as System', 'String')
$subKey = $subKey.CreateSubKey('command', $true)
$subkey.SetValue($null, "`"$WSudoExe`" -S `"%1`"", 'String')
