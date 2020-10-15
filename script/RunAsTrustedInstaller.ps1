#!/usr/bin/env pwsh
$PrivexeRoot = Split-Path -Parent $PSScriptRoot
$WSudoExe = Join-Path -Path $PrivexeRoot -ChildPath "wsudo.exe"

if (!(Test-Path $WSudoExe)) {
    Write-Host -ForegroundColor Red "$WSudoExe not exists"
    Exit 1
}


$hive = [Microsoft.Win32.RegistryKey]::OpenBaseKey('ClassesRoot', 'Default')
$subKey = $hive.CreateSubKey('*\shell\RunAsTrustedInstaller', $true)
$subkey.SetValue($null, 'Run as TrustedInstaller', 'String')
$subKey = $subKey.CreateSubKey('command', $true)
$subkey.SetValue($null, "`"$WSudoExe`" -T `"%1`"", 'String')
