#!/usr/bin/env pwsh

# On Windows, Start-Process -Wait will wait job process, obObject.WaitOne(_waithandle);
# Don't use it
Function Invoke-BatchFile {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,
        [string] $ArgumentList
    )
    Set-StrictMode -Version Latest
    $tempFile = [IO.Path]::GetTempFileName()

    cmd.exe /c " `"$Path`" $argumentList && set > `"$tempFile`" " | Out-Host
    ## Go through the environment variables in the temp file.
    ## For each of them, set the variable in our local environment.
    Get-Content $tempFile | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") {
            Set-Content "env:\$($matches[1])" $matches[2]
        }
    }
    Remove-Item $tempFile
}

$vsinstalls = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools"
)
$vsvars = ""

foreach ($vi in $vsinstalls) {
    if (Test-Path "$vi\VC\Auxiliary\Build\vcvarsall.bat") {
        $vsvars = "$vi\VC\Auxiliary\Build\vcvarsall.bat"
        break;
    }
}

if ($vsvars.Length -eq 0) {
    Write-Host "Not Found visual studio installed."
    exit 1
}

Write-Host "We try run '$vsvars amd64' and initialize environment."

Invoke-BatchFile -Path $vsvars -ArgumentList "amd64" # initialize x64 env


$RootDir = "c:\projects\Privexec"

if (Test-Path "Env:\APPVEYOR_BUILD_NUMBER") {
    $BUILDID = $env:APPVEYOR_BUILD_NUMBER
    $BUILDVER = $env:APPVEYOR_BUILD_VERSION
    
    "// build.ps1 generated file: DO NOT EDIT!
    #define APPVEYOR_BUILD_NUMBER $BUILDID
    #define APPVEYOR_BUILD_VERSION L`"$BUILDVER (appveyor)`" 
    " | Out-File "$RootDir\include\config.h" -Encoding utf8
}

$workdir = Join-Path -Path $RootDir -ChildPath "build"

if (!(Test-Path $workdir)) {
    New-Item -ItemType Directory $workdir
}

Set-Location $workdir

&cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if ($LASTEXITCODE -ne 0) {
    exit 1;
}

&cmake --build .
if ($LASTEXITCODE -ne 0) {
    exit 1;
}

&cpack -G ZIP
if ($LASTEXITCODE -ne 0) {
    exit 1;
}

