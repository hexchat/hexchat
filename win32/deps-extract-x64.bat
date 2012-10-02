:: run this from a command prompt
@echo off
set PATH=%PATH%;%ProgramFiles%\7-zip

cd x64
7z x *.7z
pause
