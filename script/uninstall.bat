@echo off

reg delete "HKCR\exefile\shell\runassystem" /f
reg delete "HKCR\batfile\shell\runassystem" /f
reg delete "HKCR\cmdfile\shell\runassystem" /f
reg delete "HKCR\exefile\shell\runastrustedinstaller" /f
reg delete "HKCR\batfile\shell\runastrustedinstaller" /f
reg delete "HKCR\cmdfile\shell\runastrustedinstaller" /f