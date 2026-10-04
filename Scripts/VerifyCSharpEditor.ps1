param(
	[ValidateSet('Debug', 'Release', 'Shipping')]
	[string]$Configuration = 'Debug',
	[string]$EditorDirectory
)

$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $EditorDirectory)
{
	$EditorDirectory = Join-Path $repository "Build\Bin\$Configuration\Mane"
}
$editor = Join-Path $EditorDirectory 'Lion.exe'
$run = [Guid]::NewGuid().ToString('N').Substring(0, 8)
$fixture = Join-Path $repository "Build\Managed\EditorProjects\$Configuration\CSharp Authoring-$run"
if (Test-Path -LiteralPath $fixture)
{
	throw "Verification fixture already exists: $fixture. Use a fresh build-artifact directory to preserve previous results."
}
New-Item -ItemType Directory -Force -Path $fixture | Out-Null
Copy-Item -Path (Join-Path $repository 'Scripting\Tests\EditorProject\*') -Destination $fixture -Recurse

New-Item -ItemType Directory -Force -Path (Join-Path $fixture 'Build') | Out-Null
$stdout = Join-Path $fixture 'Build\compile.stdout.log'
$stderr = Join-Path $fixture 'Build\compile.stderr.log'
$buildProcess = Start-Process -FilePath $editor -ArgumentList @('--compile-project', ('"' + $fixture + '"')) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
Get-Content -LiteralPath $stdout, $stderr | Write-Host
if ($buildProcess.ExitCode -ne 0) { throw 'The editor could not compile the native bootstrap and C# gameplay.' }
foreach ($file in @("Build\Bin\$Configuration\lion-game.dll", "Build\Managed\$Configuration\lion-scripts.dll"))
{
	if (-not (Test-Path -LiteralPath (Join-Path $fixture $file))) { throw "Compilation omitted $file." }
}

$destination = Join-Path $repository "Build\Managed\RejectedExport-$Configuration"
$stdout = Join-Path $fixture 'Build\export.stdout.log'
$stderr = Join-Path $fixture 'Build\export.stderr.log'
$exportProcess = Start-Process -FilePath $editor -ArgumentList @('--export-windows', ('"' + $fixture + '"'), ('"' + $destination + '"')) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$message = Get-Content -LiteralPath $stdout, $stderr | Out-String
$exportCode = $exportProcess.ExitCode
if ($exportCode -eq 0 -or $message -notmatch 'C# player export is not implemented')
{
	throw 'Managed export did not fail safely with the expected diagnostic.'
}
if (Test-Path -LiteralPath $destination) { throw 'Rejected managed export created a partial player.' }
Write-Host 'Editor compilation and managed-export guard checks passed.'
Write-Host "Open the fixture for UI/Play/Stop checks: $fixture"
