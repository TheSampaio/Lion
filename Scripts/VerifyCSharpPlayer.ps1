param([Parameter(Mandatory = $true)][string]$PlayerDirectory)

$ErrorActionPreference = 'Stop'
$player = [IO.Path]::GetFullPath($PlayerDirectory)
$executable = Join-Path $player 'CSharp Authoring.exe'
$report = Join-Path $player 'Gameplay.report.json'
if (Test-Path -LiteralPath $report) { throw 'Use a fresh exported verification player.' }
$keys = @('LION_SCRIPT_TEST_REPORT', 'DOTNET_ROOT', 'DOTNET_ROOT_X64', 'DOTNET_MULTILEVEL_LOOKUP', 'PATH')
$previous = @{}
foreach ($key in $keys) { $previous[$key] = [Environment]::GetEnvironmentVariable($key, 'Process') }
$process = $null
try
{
	$env:LION_SCRIPT_TEST_REPORT = $report
	$env:DOTNET_ROOT = Join-Path $player 'MissingSystemRuntime'
	$env:DOTNET_ROOT_X64 = $env:DOTNET_ROOT
	$env:DOTNET_MULTILEVEL_LOOKUP = '0'
	$env:PATH = Join-Path $env:SystemRoot 'System32'
	$process = Start-Process -FilePath $executable -WorkingDirectory $env:TEMP -PassThru -WindowStyle Hidden -RedirectStandardError (Join-Path $player 'Startup.stderr.log')
	$null = $process.Handle
	if (-not $process.WaitForExit(20000)) { throw 'The packaged C# gameplay did not finish its verification callback.' }
	if ($process.ExitCode -ne 0) { throw "Managed player startup failed ($($process.ExitCode)): $(Get-Content (Join-Path $player 'Startup.stderr.log') -Raw)" }
	if (-not (Test-Path -LiteralPath $report)) { throw 'The player did not execute its C# update callback.' }
	$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
	if ($result.Position -le 0) { throw 'Managed gameplay did not change the native Transform.' }
	$framework = [IO.Path]::GetFullPath($result.Framework)
	$script = [IO.Path]::GetFullPath($result.Script)
	if (-not $framework.StartsWith((Join-Path $player 'Dotnet\'), [StringComparison]::OrdinalIgnoreCase) -or
		-not $script.StartsWith((Join-Path $player 'Managed\'), [StringComparison]::OrdinalIgnoreCase))
	{
		throw 'The player used a system runtime or a development script assembly instead of its package.'
	}
	Write-Host "C# player passed with app-local runtime: $framework"
	$api = Join-Path $player 'Managed\Lion.Engine.dll'
	$disabledApi = $api + '.disabled'
	Move-Item -LiteralPath $api -Destination $disabledApi
	try
	{
		$process = Start-Process -FilePath $executable -WorkingDirectory $env:TEMP -PassThru -WindowStyle Hidden -RedirectStandardError (Join-Path $player 'MissingApi.stderr.log')
		$null = $process.Handle
		if (-not $process.WaitForExit(10000) -or $process.ExitCode -eq 0) { throw 'A missing managed SDK did not fail startup explicitly.' }
		Write-Host 'Missing managed SDK correctly returned a failing player exit code.'
	}
	finally
	{
		if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id }
		Move-Item -LiteralPath $disabledApi -Destination $api
	}
}
finally
{
	if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id }
	foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $previous[$key], 'Process') }
}
