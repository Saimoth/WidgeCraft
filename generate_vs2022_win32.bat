@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found. Install CMake or run this from a Visual Studio Developer Command Prompt.
    pause
    exit /b 1
)

pushd "%~dp0"
cmake --preset vs2022-win32-release
if errorlevel 1 (
    echo.
    echo WidgeCraft solution generation failed.
    popd
    pause
    exit /b 1
)

set "SOLUTION=%CD%\build\win32-release\WidgeCraft.sln"
echo.
echo Generated: %SOLUTION%

if exist "%SOLUTION%" start "" "%SOLUTION%"
popd
endlocal
