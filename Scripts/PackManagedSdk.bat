@echo off
rem C# authoring is optional: native tools still build on machines without the .NET 10 SDK.
setlocal
set "ROOT=%~1"
set "TARGET=%~2"
set "CONFIG=%~3"
if "%CONFIG%"=="Shipping" set "CONFIG=Release"
where dotnet >nul 2>nul
if errorlevel 1 exit /b 0
dotnet --list-sdks | findstr /B "10." >nul
if errorlevel 1 exit /b 0
dotnet build "%ROOT%\Scripting\Lion.Engine\Lion.Engine.csproj" -c %CONFIG% --nologo --ignore-failed-sources -p:NuGetAudit=false
if errorlevel 1 exit /b 1
if not exist "%TARGET%\Managed" mkdir "%TARGET%\Managed"
for %%F in (Lion.Engine.dll Lion.Engine.deps.json Lion.Engine.runtimeconfig.json Lion.Engine.xml) do (
    copy /Y "%ROOT%\Build\Managed\Bin\Lion.Engine\%CONFIG%\net10.0\%%F" "%TARGET%\Managed\" >nul
    if errorlevel 1 exit /b 1
)
endlocal
exit /b 0
