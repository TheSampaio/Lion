@echo off
rem Assembles the SDK the editor compiles game modules against — the headers and the import library a
rem C++ project needs to build against the engine, gathered beside the editor the way Unreal installs
rem ship theirs. A distributed editor carries this folder, which is what lets it create and compile C++
rem components on a machine that has Visual Studio and nothing of the engine's source tree.
rem
rem Called from the editor's post-build (see Editor/premake5.lua):
rem   PackSdk.bat <engine root> <editor output dir> <configuration>
rem
rem Every vendored licence travels with the headers copied out of its library.

setlocal
set "ROOT=%~1"
set "TARGET=%~2\SDK"
set "CONFIG=%~3"

rem The engine's own headers: the public umbrella and the source tree the game module includes.
xcopy /E /I /Y /Q "%ROOT%\Engine\Include" "%TARGET%\Engine\Include" >nul
xcopy /S /I /Y /Q "%ROOT%\Engine\Source\*.h" "%TARGET%\Engine\Source" >nul

rem The vendored headers a game module compiles against (the Sandbox's own include list), licences included.
for %%V in (box2d glad glfw glm spdlog stb) do (
    xcopy /E /I /Y /Q "%ROOT%\Vendor\%%V\include" "%TARGET%\Vendor\%%V\include" >nul
    for %%L in ("%ROOT%\Vendor\%%V\LICENSE*" "%ROOT%\Vendor\%%V\copying*") do copy /Y "%%L" "%TARGET%\Vendor\%%V\" >nul 2>nul
)

rem The engine's import library, matching the configuration this editor was built in.
if not exist "%TARGET%\Lib" mkdir "%TARGET%\Lib"
copy /Y "%ROOT%\Build\Bin\%CONFIG%\Lion\lion-core.lib" "%TARGET%\Lib\" >nul

endlocal
exit /b 0
