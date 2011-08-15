@echo off
echo.Compiling translations . . .
cd ..\po
rmdir /q /s locale
mkdir locale
for %%A in (*.po) do (
	mkdir locale\%%~nA\LC_MESSAGES
	msgfmt -co locale\%%~nA\LC_MESSAGES\xchat.mo %%A
)
cd ..\build
