@echo off
setlocal EnableDelayedExpansion

echo.
echo ==========================================
echo     PacketForge Installer Script
echo ==========================================
echo.
echo This script will deploy and package PacketForge
echo for distribution.
echo.

REM 1. Run Deployment First
call "%~dp0deploy.bat"
if %errorlevel% neq 0 (
    echo Deployment failed. Setup aborted.
    pause
    exit /b %errorlevel%
)

echo.
echo ==========================================
echo        CREATE INSTALLER PACKAGE
echo ==========================================
echo.

REM 2. Ask for Filename
set "setupName=PacketForge_Setup.zip"
set /p "inputName=Enter Installer Filename (Default: PacketForge_Setup.zip): "

if not "%inputName%"=="" (
    set "setupName=%inputName%"
    REM Add .zip extension if missing
    if /i not "!setupName:~-4!"==".zip" set "setupName=!setupName!.zip"
)

echo.
echo Creating installer package: %setupName%
echo.
echo Package Contents:
echo   - bin/           (Application and Qt dependencies)
echo   - Files/         (Resources, logs, stylesheets)
echo.

REM 3. Copy Files folder into bin directory (same level as EXE)
echo Copying Files folder to bin directory...
set "binDir=%~dp0..\bin"
set "filesDir=%~dp0..\Files"

if exist "%filesDir%" (
    xcopy /E /I /Y "%filesDir%" "%binDir%\Files"
    if %errorlevel% neq 0 (
        echo [WARNING] Failed to copy Files folder to bin directory.
    ) else (
        echo [OK] Files folder copied to bin directory.
    )
) else (
    echo [WARNING] Files folder not found at: %filesDir%
)

REM 4. Create Zip containing only the bin folder (which now has Files inside)
echo.
echo Creating archive...

set "psCommand=Compress-Archive -Path '%binDir%\*' -DestinationPath '%~dp0..\%setupName%' -Force"

powershell -NoProfile -Command "%psCommand%"

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Installer created at: ..\%setupName%
    echo.
    echo Package structure:
    echo   %setupName%
    echo   ├── PacketForge.exe
    echo   ├── Qt6*.dll (dependencies)
    echo   ├── Files/
    echo   │   ├── Images/
    echo   │   ├── Logs/
    echo   │   ├── Stylesheet/
    echo   │   └── InterfaceDetails.txt
    echo   └── (other Qt runtime files)
) else (
    echo.
    echo [ERROR] Failed to create installer archive.
)

echo.
pause
endlocal
