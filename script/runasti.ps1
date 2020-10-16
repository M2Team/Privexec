#!/usr/bin/env pwsh
$PrivexeRoot = Split-Path -Parent $PSScriptRoot
$WSudoExe = Join-Path -Path $PrivexeRoot -ChildPath "wsudo.exe"
$WSudoBridge = Join-Path -Path $PrivexeRoot -ChildPath "wsudo-bridge.exe"

if (!(Test-Path $WSudoExe)) {
    Write-Host -ForegroundColor Red "$WSudoExe not exists"
    Exit 1
}

if (!(Test-Path $WSudoBridge)) {
    Write-Host -ForegroundColor Red "$WSudoBridge not exists"
    Exit 1
}

$subKeys = @( 'exefile\shell\runastrustedinstaller', 'batfile\shell\runastrustedinstaller', 'cmdfile\shell\runastrustedinstaller' )


# https://powershell.org/forums/topic/cant-set-new-itemproperty-to-registry-path-containing-astrix/

$hive = [Microsoft.Win32.RegistryKey]::OpenBaseKey('ClassesRoot', 'Default')
foreach ($sk in $subKeys) {
    $subKey = $hive.CreateSubKey($sk, $true)
    $subkey.SetValue($null, 'Run as TrustedInstaller', 'String')
    $subkey.SetValue('Icon', $WSudoBridge, 'String')
    $subKey = $subKey.CreateSubKey('command', $true)
    $subkey.SetValue($null, "`"$WSudoExe`" -T `"%1`"", 'String')
}

