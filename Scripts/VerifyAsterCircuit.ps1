param(
	[Parameter(Mandatory = $true)][string]$PlayerDirectory,
	[string]$OutputDirectory,
	[switch]$Campaign,
	[string]$ProgressDirectory,
	[ValidateRange(-1, 7)][int]$ExpectedCleared = -1,
	[switch]$FailurePaths
)

$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repository ('Build\Verification\AsterCircuit-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)) }
$PlayerDirectory = [IO.Path]::GetFullPath($PlayerDirectory)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$report = Join-Path $OutputDirectory 'State.json'
$savedReport = $env:ASTER_TEST_REPORT
$savedProgress = $env:ASTER_SAVE_DIRECTORY
$env:ASTER_TEST_REPORT = $report
$env:ASTER_SAVE_DIRECTORY = if ($ProgressDirectory) { [IO.Path]::GetFullPath($ProgressDirectory) } else { Join-Path $OutputDirectory 'Save' }
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class AsterOwnedWindow
{
	public delegate bool Callback(IntPtr window, IntPtr argument);
	[StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
	[DllImport("user32.dll")] public static extern bool EnumWindows(Callback callback, IntPtr argument);
	[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint owner);
	[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
	[DllImport("user32.dll")] public static extern bool IsIconic(IntPtr window);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Rect rectangle);
	[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr window, int command);
	[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int width, int height, uint flags);
	[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, UIntPtr key, IntPtr flags);
	[DllImport("user32.dll")] public static extern uint MapVirtualKey(uint key, uint type);
	[DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr window, Callback callback, IntPtr argument);
}
'@
$process = $null
$script:window = [IntPtr]::Zero
$held = [Collections.Generic.HashSet[byte]]::new()
$script:lastSnapshot = ''
function Key([byte]$code, [bool]$down)
{
	if (-not $down -and $process.HasExited) { $held.Remove($code) | Out-Null; return }
	$owner = 0
	[AsterOwnedWindow]::GetWindowThreadProcessId($script:window, [ref]$owner) | Out-Null
	if (-not $process -or $owner -ne $process.Id) { throw 'Input must target only the owned test player.' }
	# Hidden automation launches must stay restored while sending owned-window input.
	[AsterOwnedWindow]::ShowWindow($script:window, 9) | Out-Null
	[AsterOwnedWindow]::SetWindowPos($script:window, [IntPtr](-1), 0, 0, 0, 0, 0x43) | Out-Null
	if ($down) { $held.Add($code) | Out-Null }
	else { if (-not $held.Remove($code)) { return } }
	$flags = 1 -bor ([AsterOwnedWindow]::MapVirtualKey($code, 0) -shl 16)
	if ($code -ge 37 -and $code -le 40) { $flags = $flags -bor 0x1000000 }
	if (-not $down) { $flags = $flags -bor [int]0xC0000000 }
	[AsterOwnedWindow]::PostMessage($script:window, $(if ($down) { 0x100 } else { 0x101 }), [UIntPtr]$code, [IntPtr]$flags) | Out-Null
}
function ReleaseKeys { foreach ($code in @($held)) { Key $code $false } }
function Tap([byte]$code, [int]$duration = 80) { Key $code $true; Start-Sleep -Milliseconds $duration; Key $code $false; Start-Sleep -Milliseconds 100 }
function State
{
	for ($attempt = 0; $attempt -lt 20; $attempt++)
	{
		try
		{
			$snapshot = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
			# File.WriteAllText briefly truncates the observational report before replacing its contents.
			if ($null -ne $snapshot -and $snapshot.Screen)
			{
				$json = $snapshot | ConvertTo-Json -Compress
				if ($json -ne $script:lastSnapshot) { [IO.File]::AppendAllText((Join-Path $OutputDirectory 'StateHistory.jsonl'), $json + [Environment]::NewLine); $script:lastSnapshot = $json }
				return $snapshot
			}
			Start-Sleep -Milliseconds 20
		}
		catch { Start-Sleep -Milliseconds 20 }
	}
	throw 'The gameplay state report did not become readable.'
}
function WaitScreen([string]$screen, [int]$seconds = 10)
{
	$clock = [Diagnostics.Stopwatch]::StartNew()
	do
	{
		Start-Sleep -Milliseconds 100
		[AsterOwnedWindow]::ShowWindow($script:window, 9) | Out-Null
		[AsterOwnedWindow]::SetWindowPos($script:window, [IntPtr](-1), 0, 0, 0, 0, 0x43) | Out-Null
		try { $state = State; if ($state.Screen -eq $screen) { return $state } }
		catch { if ($process.HasExited) { throw } }
	} while ($clock.Elapsed.TotalSeconds -lt $seconds -and -not $process.HasExited)
	throw "Expected $screen, got $($state.Screen)."
}
function Capture([string]$name)
{
	[AsterOwnedWindow]::ShowWindow($script:window, 9) | Out-Null
	[AsterOwnedWindow]::SetWindowPos($script:window, [IntPtr](-1), 0, 0, 0, 0, 0x43) | Out-Null
	$rectangle = [AsterOwnedWindow+Rect]::new()
	[AsterOwnedWindow]::GetWindowRect($script:window, [ref]$rectangle) | Out-Null
	$bitmap = [Drawing.Bitmap]::new($rectangle.Right - $rectangle.Left, $rectangle.Bottom - $rectangle.Top)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	try { $graphics.CopyFromScreen($rectangle.Left, $rectangle.Top, 0, 0, $bitmap.Size); $bitmap.Save((Join-Path $OutputDirectory ($name + '.png'))) }
	finally { $graphics.Dispose(); $bitmap.Dispose() }
}
function Deploy([int]$stage)
{
	$state = WaitScreen 'Select'
	while ($state.Selection -ne $stage) { Tap 39; $state = State }
	Tap 13
	$state = WaitScreen 'Stage'
	if ($state.Stage -ne $stage) { throw 'Stage selection loaded the wrong authored scene.' }
	Start-Sleep -Milliseconds 350
	return (State)
}
try
{
	$executable = Join-Path $PlayerDirectory 'Aster Circuit.exe'
	$process = Start-Process -FilePath $executable -WorkingDirectory $env:TEMP -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $OutputDirectory 'Runtime.stdout.log') -RedirectStandardError (Join-Path $OutputDirectory 'Runtime.stderr.log')
	$null = $process.Handle
	$startupClock = [Diagnostics.Stopwatch]::StartNew()
	while ($script:window -eq [IntPtr]::Zero -and $startupClock.Elapsed.TotalSeconds -lt 15)
	{
		[AsterOwnedWindow]::EnumWindows({
			param($candidate, $argument)
			$owner = 0
			[AsterOwnedWindow]::GetWindowThreadProcessId($candidate, [ref]$owner) | Out-Null
			if ($owner -ne $process.Id -or -not [AsterOwnedWindow]::IsWindowVisible($candidate)) { return $true }
			$title = [Text.StringBuilder]::new(256)
			[AsterOwnedWindow]::GetWindowText($candidate, $title, $title.Capacity) | Out-Null
			if ($title.ToString() -ne 'Aster Circuit') { return $true }
			$script:window = $candidate
			return $false
		}, [IntPtr]::Zero) | Out-Null
		Start-Sleep -Milliseconds 10
	}
	if ($script:window -eq [IntPtr]::Zero) { throw 'The owned Aster Circuit player did not start.' }
	Capture 'Splash-Start'
	Start-Sleep -Milliseconds 500
	Capture 'Splash-Hold'
	Start-Sleep -Milliseconds 850
	Capture 'Splash-Fade'
	$menu = WaitScreen 'Menu'
	if ($ExpectedCleared -ge 0 -and $menu.Cleared -ne $ExpectedCleared) { throw 'Saved campaign progress was not restored on startup.' }
	Start-Sleep -Seconds 2
	Capture 'Menu'
	Tap 13
	$null = WaitScreen 'Select'
	Capture 'Select'
	foreach ($stage in 0..2)
	{
		$initial = Deploy $stage
		if (-not $initial.Audio -or -not $initial.Grounded -or $initial.Health -ne 24) { throw "Stage $stage did not initialize music/physics/health: $($initial | ConvertTo-Json -Compress)" }
		Key 39 $true; Key 88 $true; Key 90 $true
		Start-Sleep -Milliseconds 380
		Key 90 $false
		Start-Sleep -Milliseconds 330
		ReleaseKeys
		$moving = State
		if ($moving.X -le $initial.X + 60 -or $moving.HighestY -lt 260 -or $moving.ShotsFired -lt 2 -or $moving.Jumps -lt 1) { throw "Stage $stage failed movement/jump/fire." }
		Capture "Stage-$stage"
		Tap 80; Start-Sleep -Milliseconds 150; $paused = State
		Key 39 $true; Start-Sleep -Milliseconds 250; Key 39 $false; $still = State
		if (-not $still.Paused -or [Math]::Abs($still.X - $paused.X) -gt 1) { throw 'Pause did not freeze native simulation.' }
		Capture "Pause-$stage"
		Tap 80; Tap 27
		$null = WaitScreen 'Select'
		Write-Host "Stage ${stage}: audio, movement, jump, shooting, pause and scene return passed."
	}
	if ($FailurePaths)
	{
		$state = Deploy 2
		$failureClock = [Diagnostics.Stopwatch]::StartNew()
		while ($state.Screen -eq 'Stage' -and $failureClock.Elapsed.TotalSeconds -lt 60)
		{
			if ($state.Ending -eq 0) { Key 39 $true } else { ReleaseKeys }
			Start-Sleep -Milliseconds 50; $state = State
		}
		ReleaseKeys
		if ($state.Screen -ne 'GameOver') { throw 'Three normal falls did not reach Game Over.' }
		Capture 'GameOver'
		Tap 13; $state = WaitScreen 'Stage'; Start-Sleep -Milliseconds 350; $state = State
		if ($state.Lives -ne 3 -or $state.Health -ne 24) { throw 'Retry did not reset health and lives.' }
		if ($ExpectedCleared -eq 7)
		{
			foreach ($weapon in 1..3) { Tap 67; $state = State; if ($state.Weapon -ne $weapon) { throw 'A saved weapon unlock was not restored.' } }
			Tap 67; $state = State; if ($state.Weapon -ne 0) { throw 'Weapon cycling did not return to pulse.' }
		}
		Capture 'Retry'; Tap 27; $null = WaitScreen 'Select'
		Write-Host 'Normal falls, Game Over, retry and saved weapon unlocks passed.'
	}
	if ($Campaign)
	{
		foreach ($stage in 0..2)
		{
			$state = Deploy $stage
			if ($stage -gt 0)
			{
				for ($cycle = 0; $cycle -lt 4 -and $state.Weapon -ne 1; $cycle++) { Tap 67; $state = State }
				if ($state.Weapon -ne 1) { throw 'The first cleared stage did not unlock Ember Fan.' }
			}
			$campaignClock = [Diagnostics.Stopwatch]::StartNew()
			$jumpUntil = -1.0
			$lastProgress = -5.0
			$lastDeath = -1
			$simulationSeconds = 0.0
			$lastElapsed = 0.0
			# Rendering can be suspended by the desktop host. Limit active gameplay as well as wall time.
			while ($campaignClock.Elapsed.TotalSeconds -lt 600 -and $simulationSeconds -lt 180)
			{
				$state = State
				if ($state.Screen -ne 'Stage') { break }
				$simulationSeconds += [Math]::Max(0.0, [double]$state.Elapsed - $lastElapsed)
				$lastElapsed = $state.Elapsed
				if ($state.Ending -ne 0)
				{
					if ($state.Ending -eq 1 -and $lastDeath -ne $state.Lives) { Write-Host "Stage $stage death: x=$($state.X), y=$($state.Y), height=$($state.HighestY), jumps=$($state.Jumps), checkpoint=$($state.Checkpoint), lives=$($state.Lives)."; $lastDeath = $state.Lives }
					$jumpUntil = -1; ReleaseKeys; Start-Sleep -Milliseconds 150; continue
				}
				if ($state.Elapsed -gt $lastProgress + 5) { Write-Host "Stage ${stage}: t=$([Math]::Round($state.Elapsed, 1)), x=$([Math]::Round($state.X)), hp=$($state.Health), boss=$($state.BossHealth)."; $lastProgress = $state.Elapsed }
				Key 88 $true
				if (-not $state.BossActive)
				{
					Key 39 $true
					# Glacier's first gap is bridged by a high platform reached through its ladder.
					if ($stage -eq 1 -and $state.X -gt 1180 -and $state.X -lt 1350 -and $state.Y -lt 337)
					{
						$stoppingX = $state.X + $state.HorizontalSpeed * 0.1 + $state.HorizontalSpeed * [Math]::Abs($state.HorizontalSpeed) / 1300
						Key 39 ($stoppingX -lt 1310); Key 37 ($stoppingX -gt 1330); Key 38 $true; Key 90 $false
						Start-Sleep -Milliseconds 35; continue
					}
					Key 38 $false; Key 37 $false
					$gaps = if ($stage -eq 0) { @(1152, 2784, 4272) } elseif ($stage -eq 1) { @(1392, 3408) } else { @(912, 2160, 3792) }
					$needsJump = $false
					$jumpLead = if ($stage -eq 2) { 30 } else { 60 }
					foreach ($gap in $gaps) { if ($state.X -gt $gap - $jumpLead -and $state.X -lt $gap + 15) { $needsJump = $true } }
					if ($stage -eq 0)
					{
						foreach ($ledge in @(536, 1356, 2176, 2996, 3816)) { if ($state.X -gt $ledge - 85 -and $state.X -lt $ledge + 20) { $needsJump = $true } }
					}
					# Jump over platforms and enemies too, using normal input rather than a gameplay bypass.
					if ($state.Grounded -and $needsJump -and -not $held.Contains([byte]90)) { Write-Host "Stage $stage jump at x=$([Math]::Round($state.X))."; $jumpUntil = $state.Elapsed + 0.75 }
				}
				else
				{
					$direction = [Math]::Sign($state.BossX - $state.X)
					$distance = [Math]::Abs($state.BossX - $state.X)
					$move = if ($distance -gt 270) { $direction } elseif ($distance -lt 140) { -$direction } elseif ($state.Facing -ne $direction) { $direction } else { 0 }
					$cross = ($state.X -lt 5350 -and $move -lt 0) -or ($state.X -gt 6010 -and $move -gt 0)
					if ($cross) { $move = -$move }
					Key 39 ($move -gt 0)
					Key 37 ($move -lt 0)
					if ($state.Grounded -and ($state.BossY -gt $state.Y + 70 -or $cross) -and -not $held.Contains([byte]90)) { $jumpUntil = $state.Elapsed + 0.65 }
				}
				Key 90 ($state.Elapsed -lt $jumpUntil)
				Start-Sleep -Milliseconds 35
			}
			ReleaseKeys
			$state = State
			Write-Host "Stage $stage budget: wall=$($campaignClock.Elapsed.TotalSeconds), simulation=$simulationSeconds."
			Capture "Campaign-$stage"
			if ($state.Screen -eq 'GameOver' -or -not ($state.Cleared -band (1 -shl $stage)))
			{
				[AsterOwnedWindow]::EnumWindows({
					param($candidate, $argument)
					$owner = 0; [AsterOwnedWindow]::GetWindowThreadProcessId($candidate, [ref]$owner) | Out-Null
					if ($owner -eq $process.Id)
					{
						$title = [Text.StringBuilder]::new(4096); [AsterOwnedWindow]::GetWindowText($candidate, $title, $title.Capacity) | Out-Null
						Write-Host "Owned window: $title"
						[AsterOwnedWindow]::EnumChildWindows($candidate, {
							param($child, $unused)
							$text = [Text.StringBuilder]::new(4096); [AsterOwnedWindow]::GetWindowText($child, $text, $text.Capacity) | Out-Null
							if ($text.Length) { Write-Host "Owned child: $text" }; return $true
						}, [IntPtr]::Zero) | Out-Null
					}
					return $true
				}, [IntPtr]::Zero) | Out-Null
				throw "Campaign stage $stage not completed: $($state | ConvertTo-Json -Compress)"
			}
			Write-Host "Campaign stage $stage cleared through normal controls."
		}
		$null = WaitScreen 'Ending'
		Capture 'Ending'
		Tap 13
		$null = WaitScreen 'Menu'
	}
	else { Tap 27; $null = WaitScreen 'Menu' }
	Tap 27
	if (-not $process.WaitForExit(5000) -or $process.ExitCode -ne 0) { throw 'Clean player shutdown failed.' }
	Write-Host "Aster Circuit verification passed: $OutputDirectory"
}
finally
{
	if ($process -and -not $process.HasExited) { ReleaseKeys; Stop-Process -Id $process.Id }
	$env:ASTER_TEST_REPORT = $savedReport
	$env:ASTER_SAVE_DIRECTORY = $savedProgress
}
