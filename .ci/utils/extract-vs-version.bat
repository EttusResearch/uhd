@echo off
REM Extract Visual Studio version info from cmake compiler string
REM Input: $(cmakeCompiler) e.g., "Visual Studio 18 2026"
REM Output: vsToolVersion, vsYear, vsToolsInstallDir, vcpkgManifestSrc

setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Error: cmakeCompiler argument required ^(e.g., "Visual Studio 18 2026"^)
    exit /b 1
)

set "cmakeCompiler=%~1"

REM Extract vsToolVersion (the number after "Visual Studio")
REM Example: "Visual Studio 18 2026" -> "18"
for /f "tokens=3" %%A in ("%cmakeCompiler%") do set "vsToolVersion=%%A"

REM Extract vsYear (the last number)
REM Example: "Visual Studio 18 2026" -> "2026"
for /f "tokens=4" %%A in ("%cmakeCompiler%") do set "vsExtractedYear=%%A"

REM Validate that matrix variable matches extracted value
if not defined vsYear (
    echo Warning: Matrix vsYear dot defined, defining it vsYear=%vsExtractedYear%
    set vsYear=%vsExtractedYear%
    echo ##vso[task.setvariable variable=vsYear]%vsYear%
)
if not "%vsYear%"=="%vsExtractedYear%" (
    echo Warning: Matrix vsYear=%vsYear% does not match extracted vsYear=%vsExtractedYear%
)

REM Verify that vsToolVersion is numeric and less than 4 digits
if not defined vsToolVersion (
    echo Error: Could not extract vsToolVersion from "%cmakeCompiler%"
    exit /b 1
)

echo %vsToolVersion%| findstr /r "^[0-9][0-9]$" >nul || (
    echo Error: vsToolVersion must be exactly 2 digits, got "%vsToolVersion%"
    exit /b 1
)

REM Verify that vsYear is 4 digits and starts with '2'
if not defined vsYear (
    echo Error: Could not extract vsYear from "%cmakeCompiler%"
    exit /b 1
)
echo %vsYear%| findstr /r "^2[0-9][0-9][0-9]$" >nul || (
    echo Error: vsYear must be a valid year starting with 2, got "%vsYear%"
    exit /b 1
)

REM Set vcpkgManifestSrc (relative path to vcpkg manifest directory)
set "vcpkgManifestFile=vcpkg.json"
set "vsToolsUrl=https://aka.ms/vs/17/release/vs_BuildTools.exe"
if %vsToolVersion% == 18 (
    set "vcpkgManifestFile=vcpkg-vs2026.json"
    set "vsToolsUrl=https://aka.ms/vs/stable/vs_BuildTools.exe"
)

REM Set vsToolsInstallDir based on vsYear, handle version greater than or equal to 18 special
REM Note: These are the expected default installation path of vs_BuildTools.exe
if %vsToolVersion% GEQ 18 (
    set "vsToolsInstallDir=C:\Program Files (x86)\Microsoft Visual Studio\%vsToolVersion%\BuildTools"
) else (
    set "vsToolsInstallDir=C:\Program Files (x86)\Microsoft Visual Studio\%vsYear%\BuildTools"
)

REM Export as environment variables
endlocal & set "vsToolVersion=%vsToolVersion%" & set "vsYear=%vsYear%" & set "vsToolsInstallDir=%vsToolsInstallDir%" & set "vcpkgManifestFile=%vcpkgManifestFile%" & set "vsToolsUrl=%vsToolsUrl%"

REM Export as Azure DevOps variables
echo ##vso[task.setvariable variable=vsToolVersion]%vsToolVersion%
echo ##vso[task.setvariable variable=vsToolsUrl]%vsToolsUrl%
echo ##vso[task.setvariable variable=vsToolsInstallDir]%vsToolsInstallDir%
echo ##vso[task.setvariable variable=vcpkgManifestFile]%vcpkgManifestFile%

REM Display extracted values
echo.
echo Extracted from: %1
echo vsToolVersion: %vsToolVersion%
echo vsYear: %vsYear%
echo vsToolsUrl: %vsToolsUrl%
echo vsToolsInstallDir: %vsToolsInstallDir%
echo vcpkgManifestFile: %vcpkgManifestFile%
echo.

exit /b 0
