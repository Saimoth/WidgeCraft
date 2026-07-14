@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found. Install the Visual Studio CMake tools first.
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
echo Startup project: widgecraft_ui_sandbox

if /I not "%~1"=="--no-open" (
    if exist "%SOLUTION%" start "" "%SOLUTION%"
)

popd
endlocal
