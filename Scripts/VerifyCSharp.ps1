param(
	[ValidateSet('Debug', 'Release', 'Shipping')]
	[string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$managedConfiguration = if ($Configuration -eq 'Debug') { 'Debug' } else { 'Release' }

foreach ($project in @('Examples\Lion.Scripting.Examples.csproj', 'Tests\Lion.Scripting.Tests.csproj'))
{
	& dotnet build (Join-Path $repository "Scripting\$project") --configuration $managedConfiguration --ignore-failed-sources -p:NuGetAudit=false
	if ($LASTEXITCODE -ne 0) { throw 'Managed scripting build failed.' }
}

$dotnetDirectory = Split-Path (Get-Command dotnet).Source
$hostfxr = Get-ChildItem -LiteralPath (Join-Path $dotnetDirectory 'host\fxr') -Directory |
	Where-Object { $_.Name -match '^10\.\d+\.\d+$' } |
	Sort-Object { [version]$_.Name } -Descending |
	Select-Object -First 1
if (-not $hostfxr) { throw 'Install the Windows x64 .NET 10 runtime/SDK before verifying scripting.' }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$visualStudio = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudio) { throw 'Visual Studio C++ tools are required for native integration tests.' }

$outputDirectory = Join-Path $repository "Build\Managed\Tests\$Configuration"
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$sdk = Join-Path $repository "Build\Bin\$Configuration\Mane"
if (-not (Test-Path -LiteralPath (Join-Path $sdk 'Bin\lion-core.lib')))
{
	throw "Build the native $Configuration configuration before running this verification."
}

$runtimeFlag = if ($Configuration -eq 'Debug') { '/MDd' } else { '/MD' }
$vcvars = Join-Path $visualStudio 'VC\Auxiliary\Build\vcvars64.bat'
$source = Join-Path $repository 'Scripting\Tests\Native\CSharpIntegration.cpp'
$executable = Join-Path $outputDirectory 'CSharpIntegration.exe'
$object = Join-Path $outputDirectory 'CSharpIntegration.obj'
$command = "`"$vcvars`" >nul && cl /nologo /std:c++20 /EHsc /utf-8 /O2 $runtimeFlag /DLN_PLATFORM_WIN /DLN_DISABLE_WARNINGS=6294 /I`"$sdk\Include`" `"$source`" /Fo`"$object`" /Fe`"$executable`" /link /LIBPATH:`"$sdk\Bin`" lion-core.lib"
& $env:ComSpec /D /S /C $command
if ($LASTEXITCODE -ne 0) { throw 'Native scripting integration test compilation failed.' }

Copy-Item -LiteralPath (Join-Path $sdk 'lion-core.dll'), (Join-Path $sdk 'lion-platform.dll') -Destination $outputDirectory
$managedRoot = Join-Path $repository 'Build\Managed\Bin'
$runtime = Join-Path $managedRoot "Lion.Engine\$managedConfiguration\net10.0"
$examples = Join-Path $managedRoot "Lion.Scripting.Examples\$managedConfiguration\net10.0\Lion.Scripting.Examples.dll"
$tests = Join-Path $managedRoot "Lion.Scripting.Tests\$managedConfiguration\net10.0\Lion.Scripting.Tests.dll"
& $executable $runtime (Join-Path $hostfxr.FullName 'hostfxr.dll') $examples $tests
if ($LASTEXITCODE -ne 0) { throw 'Native/managed scripting integration checks failed.' }
