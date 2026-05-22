@echo off
setlocal

:: ==========================================
:: 1. 환경 설정 (프로젝트에 맞게 수정하세요)
:: ==========================================

:: 공유 폴더 (심볼 서버) 경로
set SYMBOL_SERVER=\\172.21.10.28\symbols

:: 방금 빌드된 파일들(.exe, .dll, .pdb)이 있는 경로 (상대 경로 또는 절대 경로)
set BUILD_DIR=%~dp0Bin\Debug

:: 프로젝트 메타데이터
set PRODUCT_NAME=KraftonEngine
set PRODUCT_VERSION=1.0.0
set COMMENT="Auto Upload by Build Script"

:: symstore.exe 파일이 있는 실제 경로 (Windows SDK 버전에 따라 다를 수 있음)
set SYMSTORE_EXE="C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\symstore.exe"

:: ==========================================
:: 2. 업로드 실행
:: ==========================================

echo [Info] 심볼 업로드를 시작합니다...
echo Target Directory: %BUILD_DIR%
echo Symbol Server: %SYMBOL_SERVER%

:: symstore.exe 실행 (PDB 파일 업로드)
%SYMSTORE_EXE% add /f "%BUILD_DIR%\*.pdb" /s "%SYMBOL_SERVER%" /t "%PRODUCT_NAME%" /v "%PRODUCT_VERSION%" /c %COMMENT%

:: (선택) 실행 파일(.exe, .dll)도 같이 올려두면 덤프 분석이 더 수월합니다.
:: %SYMSTORE_EXE% add /f "%BUILD_DIR%\*.exe" /s "%SYMBOL_SERVER%" /t "%PRODUCT_NAME%" /v "%PRODUCT_VERSION%" /c %COMMENT%
:: %SYMSTORE_EXE% add /f "%BUILD_DIR%\*.dll" /s "%SYMBOL_SERVER%" /t "%PRODUCT_NAME%" /v "%PRODUCT_VERSION%" /c %COMMENT%

:: ==========================================
:: 3. 결과 확인
:: ==========================================

if %ERRORLEVEL% EQU 0 (
    echo [Success] 심볼이 성공적으로 업로드되었습니다.
) else (
    echo [Error] 심볼 업로드 중 문제가 발생했습니다. 경로와 권한을 확인하세요.
)

endlocal