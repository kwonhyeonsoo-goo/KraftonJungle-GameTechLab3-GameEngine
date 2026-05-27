@echo off
setlocal

set "SYMBOL_SERVER=\\172.21.10.28\symbols"

set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=%~dp0Bin\Debug\"
if "%BUILD_DIR:~-1%"=="\" set "BUILD_DIR=%BUILD_DIR:~0,-1%"

set "CONFIGURATION=%~2"
if "%CONFIGURATION%"=="" set "CONFIGURATION=Debug"

set "PLATFORM=%~3"
if "%PLATFORM%"=="" set "PLATFORM=x64"

set "SCRIPT_PATH=%~dp0..\Scripts\AddSymbols.ps1"

echo [Info] Running symbol upload script...
echo [Info] BuildDir: %BUILD_DIR%
echo [Info] Configuration: %CONFIGURATION%
echo [Info] Platform: %PLATFORM%
echo [Info] SymbolServer: %SYMBOL_SERVER%

powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -BuildDir "%BUILD_DIR%" -Configuration "%CONFIGURATION%" -Platform "%PLATFORM%" -SymbolServer "%SYMBOL_SERVER%" -EnableSourceServer

if not "%ERRORLEVEL%"=="0" (
    echo [Warn] Symbol upload script returned %ERRORLEVEL%. Build will continue.
)

endlocal
exit /b 0
