@echo off

reg delete "HKCR\*\shell\RunAsSystem" /f
reg delete "HKCR\*\shell\RunAsTrustedInstaller" /f