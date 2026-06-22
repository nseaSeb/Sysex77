@echo off
rem ===========================================================================
rem  Sysex77 - build Windows (sans Projucer).
rem  Compile via CMake ; JUCE est telecharge automatiquement (FetchContent) si
rem  absent. Necessite : CMake >= 3.22, Git, et Visual Studio (Desktop C++).
rem
rem  Usage :
rem    build.bat            (Release dans .\build)
rem    build.bat --debug    (Debug)
rem    build.bat --test     (build puis tests unitaires)
rem    set JUCE_DIR=C:\JUCE && build.bat   (utiliser une copie locale de JUCE)
rem ===========================================================================
setlocal enabledelayedexpansion

set BUILD_TYPE=Release
set RUN_TESTS=0
for %%A in (%*) do (
    if "%%A"=="--debug"   set BUILD_TYPE=Debug
    if "%%A"=="--release" set BUILD_TYPE=Release
    if "%%A"=="--test"    set RUN_TESTS=1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [Erreur] CMake introuvable. Installez CMake ^>= 3.22 ^(et Visual Studio Desktop C++^).
    exit /b 1
)

set CFG_ARGS=-B build -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if defined JUCE_DIR set CFG_ARGS=%CFG_ARGS% -DJUCE_DIR="%JUCE_DIR%"

echo ==^> Configuration (%BUILD_TYPE%)...
cmake %CFG_ARGS% || exit /b 1

echo ==^> Compilation...
cmake --build build --config %BUILD_TYPE% || exit /b 1

set "APP=build\Sysex77_artefacts\%BUILD_TYPE%\Sysex77.exe"
echo ==^> Build termine.
if exist "%APP%" echo     Binaire : %APP%

if "%RUN_TESTS%"=="1" (
    echo ==^> Tests unitaires...
    "%APP%" --test
)

endlocal
