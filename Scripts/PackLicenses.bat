@echo off
rem Concentrates every licence the distribution carries into one Licenses folder — the engine's own and
rem every third party compiled into it or shipped beside it — each as Markdown, named for what it covers.
rem One place to look, instead of a note beside a font here and a file beside a DLL there.
rem
rem Called from the post-builds (see Editor/premake5.lua and Engine/premake5.lua):
rem   PackLicenses.bat <engine root> <output dir>

setlocal
set "ROOT=%~1"
set "TARGET=%~2\Licenses"

if not exist "%TARGET%" mkdir "%TARGET%"

copy /Y "%ROOT%\LICENCE"                    "%TARGET%\Lion.md" >nul
copy /Y "%ROOT%\Vendor\box2d\LICENSE"       "%TARGET%\box2d.md" >nul
copy /Y "%ROOT%\Vendor\glad\LICENSE"        "%TARGET%\glad.md" >nul
copy /Y "%ROOT%\Vendor\glfw\LICENSE.md"     "%TARGET%\glfw.md" >nul
copy /Y "%ROOT%\Vendor\glm\copying.txt"     "%TARGET%\glm.md" >nul
copy /Y "%ROOT%\Vendor\iconfont\licence.txt" "%TARGET%\iconfont.md" >nul
copy /Y "%ROOT%\Vendor\imgui\LICENSE.txt"   "%TARGET%\imgui.md" >nul
copy /Y "%ROOT%\Vendor\imguizmo\LICENSE"    "%TARGET%\imguizmo.md" >nul
copy /Y "%ROOT%\Vendor\json\LICENSE.MIT"    "%TARGET%\json.md" >nul
copy /Y "%ROOT%\Vendor\mdi\LICENSE"         "%TARGET%\mdi.md" >nul
copy /Y "%ROOT%\Vendor\spdlog\LICENSE"      "%TARGET%\spdlog.md" >nul
copy /Y "%ROOT%\Vendor\stb\LICENSE"         "%TARGET%\stb.md" >nul

endlocal
exit /b 0
