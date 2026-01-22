@echo off
setlocal

if not exist "extract_cmake_var.py" (
    echo Missing script extract_cmake_var.py in current folder %cd%
    goto :eof
)
if "%~1" == "" (
    echo Missing input parameter to local path of uhd repo
    goto :eof
)

set python_base_cmd=python extract_cmake_var.py

set "_var_source=%~1\host\build\CMakeCache.txt"
set "_vars= VCPKG_INSTALLED_DIR VCPKG_TARGET_TRIPLET CPACK_SOURCE_PACKAGE_FILE_NAME"
set "python_cmd=%python_base_cmd% cmake_cache --cmake_file %_var_source% --variables %_vars%"
if exist "%_var_source%" (call :_cache %_vars%)

set "_var_source=%~1\host\build\CPackConfig.cmake"
set "_vars=CPACK_PACKAGE_COPYRIGHT CPACK_PACKAGE_VENDOR"
set "python_cmd=%python_base_cmd% cmake_source --cmake_file %_var_source% --variables %_vars%"
if exist "%_var_source%" (call :_source %_vars%)

set "_var_source=%~1\host\build\cmake\Modules\UHDConfigVersion.cmake"
set "_vars=PACKAGE_VERSION MAJOR_VERSION API_VERSION ABI_VERSION PATCH_VERSION DEVEL_VERSION"
set "python_cmd=%python_base_cmd% cmake_source --cmake_file %_var_source% --variables %_vars%"
if exist "%_var_source%" (call :_source %_vars%)


goto :end

:_cache

echo Running Cmd "%python_cmd%"
for /f "usebackq tokens=1,* delims==" %%A in (`%python_cmd%`) do set %%A=%%~B
setlocal enabledelayedexpansion
for %%A in (%*) do (
    echo %%A=!%%A!
)    
endlocal
goto :eof

:_source
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: cmake not found in PATH.
    goto :eof
)

echo Running Cmd "%python_cmd%"
for /f "usebackq tokens=1,* delims==" %%A in (`%python_cmd%`) do set %%A=%%~B
setlocal enabledelayedexpansion
for %%A in (%*) do (
    echo %%A=!%%A!
)    
endlocal
goto :eof

:end
endlocal