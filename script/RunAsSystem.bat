@echo off
where pwsh.exe >nul 2>nul
powershell -NoProfile -NoLogo -ExecutionPolicy unrestricted -File "%~dp0RunAsSystem.ps1" %*