@ECHO OFF
:: Builds everything from a fresh clone: generates the projects, then builds the solution.
::
:: Build order is not this script's problem. Every project declares what it depends on, so MSBuild
:: works it out: the vendored libraries, then the engine, then the game module, then the tools that
:: load it. Each project's post-build creates the output directories it needs under Build/.
::
:: Usage:  Scripts\Build.bat [Debug|Release|Shipping]   (defaults to Debug)

SETLOCAL
TITLE Lion — Build

SET CONFIG=%~1
IF "%CONFIG%"=="" SET CONFIG=Debug

CD /D "%~dp0.."

:: --- Dependencies -------------------------------------------------------------------------------
:: The libraries are submodules, and a plain "git clone" leaves them empty — so fetch them rather
:: than failing on a missing Vendor/ folder. Already-present ones are left alone.
WHERE git >NUL 2>NUL
IF ERRORLEVEL 1 (
    ECHO [Lion] git was not found on your PATH.
    EXIT /B 1
)

IF NOT EXIST "Vendor\box2d\premake5.lua" (
    ECHO [Lion] Fetching the vendored libraries...
    git submodule update --init --recursive || EXIT /B 1
)

:: --- Projects -----------------------------------------------------------------------------------
:: premake globs the source tree, so this has to run before every build: a file added or removed
:: since the last one is not in the .vcxproj yet.
WHERE premake5 >NUL 2>NUL
IF ERRORLEVEL 1 (
    ECHO [Lion] premake5 was not found on your PATH.
    ECHO [Lion] Get it from https://premake.github.io and put premake5.exe somewhere on PATH.
    EXIT /B 1
)

ECHO [Lion] Generating the projects...
CALL premake5 vs2022 || EXIT /B 1

:: --- MSBuild ------------------------------------------------------------------------------------
:: Its path moves between Visual Studio installs, so ask the installer's own locator for it.
SET VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
IF NOT EXIST %VSWHERE% (
    ECHO [Lion] Visual Studio was not found. Install it with the "Desktop development with C++" workload.
    EXIT /B 1
)

FOR /F "usebackq tokens=*" %%i IN (`%VSWHERE% -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) DO SET MSBUILD=%%i

IF NOT DEFINED MSBUILD (
    ECHO [Lion] MSBuild was not found. Install the "Desktop development with C++" workload.
    EXIT /B 1
)

ECHO [Lion] Building %CONFIG%...
"%MSBUILD%" Lion.sln -p:Configuration=%CONFIG% -p:Platform=x64 -m -v:minimal -nologo || EXIT /B 1

ECHO.
ECHO [Lion] Done. The editor is at Build\Bin\%CONFIG%\Editor\Lion.exe
ENDLOCAL
