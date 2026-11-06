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
$buildProcess = Start-Process -FilePath $editor -ArgumentList @('--compile-project', ('"' + $fixture + '"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$null = $buildProcess.Handle
# Start-Process -Wait includes descendants such as persistent Roslyn servers; wait only for Mane.
if (-not $buildProcess.WaitForExit(60000)) { Stop-Process -Id $buildProcess.Id; throw 'Project compilation timed out.' }
Get-Content -LiteralPath $stdout, $stderr | Write-Host
if ($buildProcess.ExitCode -ne 0) { throw 'The editor could not compile the native bootstrap and C# gameplay.' }
foreach ($file in @("Build\Bin\$Configuration\lion-game.dll", "Build\Managed\$Configuration\lion-scripts.dll"))
{
	if (-not (Test-Path -LiteralPath (Join-Path $fixture $file))) { throw "Compilation omitted $file." }
}

$destination = Join-Path $repository "Build\Managed\PlayerExports\$Configuration-$run"
$shippingOutput = Join-Path $fixture 'Build\Managed\Shipping'
New-Item -ItemType Directory -Force -Path $shippingOutput | Out-Null
[IO.File]::WriteAllBytes((Join-Path $shippingOutput 'Stale.pdb'), [byte[]]@())
$stdout = Join-Path $fixture 'Build\export.stdout.log'
$stderr = Join-Path $fixture 'Build\export.stderr.log'
$exportProcess = Start-Process -FilePath $editor -ArgumentList @('--export-windows', ('"' + $fixture + '"'), ('"' + $destination + '"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$null = $exportProcess.Handle
if (-not $exportProcess.WaitForExit(60000)) { Stop-Process -Id $exportProcess.Id; throw 'Project export timed out.' }
$message = Get-Content -LiteralPath $stdout, $stderr | Out-String
$exportCode = $exportProcess.ExitCode
if ($exportCode -ne 0)
{
	throw "Managed export failed: $message"
}
$player = Join-Path $destination 'CSharp Authoring'
foreach ($file in @('CSharp Authoring.exe', 'Managed\lion-scripts.dll', 'Managed\Lion.Engine.dll', 'Config\Player.lnplayer', 'Licenses\Dotnet-LICENSE.md', 'Licenses\Dotnet-ThirdPartyNotices.md'))
{
	if (-not (Test-Path -LiteralPath (Join-Path $player $file))) { throw "Export omitted $file." }
}
if (Get-ChildItem -LiteralPath $player -Recurse -File | Where-Object { $_.Extension -in '.cs', '.cpp', '.pdb' }) { throw 'Shipping export leaked source or debug symbols.' }
& (Join-Path $PSScriptRoot 'VerifyCSharpPlayer.ps1') -PlayerDirectory $player
Write-Host 'Editor compilation and app-local managed-player checks passed.'
Write-Host "Open the fixture for UI/Play/Stop checks: $fixture"
