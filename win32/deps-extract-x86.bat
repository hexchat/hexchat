:: run this from a command prompt
@echo off
set PATH=%PATH%;%ProgramFiles%\7-zip

cd Win32
7z x *.7z
pause
