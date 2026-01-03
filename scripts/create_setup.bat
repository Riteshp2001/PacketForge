@echo off
setlocal EnableDelayedExpansion

REM 1. Run Deployment First
call "%~dp0deploy.bat"
if %errorlevel% neq 0 (
    echo Deployment failed. Setup aborted.
    pause
    exit /b %errorlevel%
)

echo.
echo ==========================================
echo           CREATE SETUP ZIP
echo ==========================================
echo.

REM 2. Ask for Filename
set "setupName=Setup.zip"
set /p "inputName=Enter Setup Filename (Default: Setup.zip): "

if not "%inputName%"=="" (
    set "setupName=%inputName%"
    REM Add .zip extension if missing
    if /i not "!setupName:~-4!"==".zip" set "setupName=!setupName!.zip"
)

echo.
echo Creating archive: %setupName%
echo Including: bin/, Files/
echo.

REM 3. Create Zip using PowerShell
REM We zip folders from the Parent Directory (PacketTransmitter root)
REM Target is also in Parent Directory

set "psCommand=Compress-Archive -Path '%~dp0..\bin', '%~dp0..\Files' -DestinationPath '%~dp0..\%setupName%' -Force"

powershell -NoProfile -Command "%psCommand%"

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Setup created at: ..\%setupName%
) else (
    echo.
    echo [ERROR] Failed to create zip archive.
)

pause
endlocal
