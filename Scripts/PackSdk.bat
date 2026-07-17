@echo off
rem Assembles what the editor compiles game modules against — the headers and the import library a C++
rem project needs to build against the engine, gathered beside the editor the way Unreal installs ship
rem theirs. A distributed editor carries these folders, which is what lets it create and compile C++
rem components on a machine that has Visual Studio and nothing of the engine's source tree.
rem
rem Two folders, shaped like every SDK a C++ programmer has met:
rem   Include\  - the engine's headers and the vendored headers a module includes, one merged tree:
rem               every package keeps to its own subfolder (Lion\, glm\, GLFW\, ...), so merging them
rem               collides with nothing and a generated project needs exactly one include path.
rem   Bin\      - lion-core.lib, matching the configuration this editor was built in.
rem
rem The licences for everything here live in the Licenses folder beside it (see PackLicenses.bat).
rem
rem Called from the editor's post-build (see Editor/premake5.lua):
rem   PackSdk.bat <engine root> <editor output dir> <configuration>

setlocal
set "ROOT=%~1"
set "TARGET=%~2"
set "CONFIG=%~3"

rem The engine's own headers: the public umbrella and the source tree the game module includes.
xcopy /E /I /Y /Q "%ROOT%\Engine\Include" "%TARGET%\Include" >nul
xcopy /S /I /Y /Q "%ROOT%\Engine\Source\*.h" "%TARGET%\Include" >nul

rem The vendored headers a game module compiles against — the Sandbox's own include list.
for %%V in (box2d glad glfw glm spdlog stb) do (
    xcopy /E /I /Y /Q "%ROOT%\Vendor\%%V\include" "%TARGET%\Include" >nul
)

rem The engine's import library.
if not exist "%TARGET%\Bin" mkdir "%TARGET%\Bin"
copy /Y "%ROOT%\Build\Bin\%CONFIG%\Lion\lion-core.lib" "%TARGET%\Bin\" >nul

endlocal
exit /b 0
