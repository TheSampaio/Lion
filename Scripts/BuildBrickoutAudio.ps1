$ErrorActionPreference = 'Stop'

$soundRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets\Sounds'))
[IO.Directory]::CreateDirectory($soundRoot) | Out-Null
$sampleRate = 44100

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class BrickoutSynth
{
	private const int SampleRate = 44100;

	private static double Triangle(double phase)
	{
		return 2.0 * Math.Asin(Math.Sin(phase)) / Math.PI;
	}

	private static double Frequency(double midi)
	{
		return 440.0 * Math.Pow(2.0, (midi - 69.0) / 12.0);
	}

	public static void WriteMusic(string path, double bpm, int[] roots, int[] melody,
		int measures, double level, int style)
	{
		double beat = 60.0 / bpm;
		int family = Math.Abs(style) % 5;
		int count = (int)(measures * 4.0 * beat * SampleRate);
		short[] samples = new short[count];
		int[][] arpPatterns = {
			new int[] { 0, 7, 12, 7 }, new int[] { 0, 3, 10, 15 }, new int[] { 0, 12, 7, 19 },
			new int[] { 0, 5, 12, 17 }, new int[] { 0, 10, 15, 22 }
		};
		for (int index = 0; index < count; ++index)
		{
			double time = (double)index / SampleRate;
			int beatNumber = (int)Math.Floor(time / beat);
			int absoluteMeasure = beatNumber / 4;
			int measure = absoluteMeasure % roots.Length;
			int section = Math.Min(absoluteMeasure * 4 / Math.Max(measures, 1), 3);
			int root = roots[measure];
			double subdivision = family == 2 ? 4.0 : family == 3 ? 1.0 : 2.0;
			double stepLength = beat / subdivision;
			int step = (int)Math.Floor(time / stepLength);
			double stepPhase = time % stepLength / stepLength;
			double envelope = Math.Min(stepPhase / (family == 3 ? 0.24 : 0.06), 1.0)
				* Math.Pow(1.0 - stepPhase, family == 3 ? 0.22 : 0.72);
			int melodyIndex = step % melody.Length;
			int melodyNote = melody[melodyIndex];
			double octave = family == 1 ? 0.0 : family == 3 ? 12.0 : 7.0;
			double lead = 0.0;
			if (melodyNote > -50)
			{
				double leadPhase = 2.0 * Math.PI * Frequency(root + octave + melodyNote) * time;
				if (family == 0)
					lead = Math.Sin(leadPhase + Math.Sin(leadPhase * 0.5) * 0.9) + Math.Sin(leadPhase * 2.01) * 0.24;
				else if (family == 1)
					lead = Triangle(leadPhase) * 0.82 + Math.Sin(leadPhase * 0.5) * 0.18;
				else if (family == 2)
					lead = (Math.Sin(leadPhase) >= 0.0 ? 0.72 : -0.72) + Math.Sin(leadPhase * 2.0) * 0.16;
				else if (family == 3)
					lead = Math.Sin(leadPhase) * 0.72 + Math.Sin(leadPhase * 1.501) * 0.30;
				else
					lead = Math.Sin(leadPhase) + Math.Sin(leadPhase * 2.0) * 0.34 + Math.Sin(leadPhase * 3.0) * 0.17;
				lead *= envelope;
			}

			int bassOffset = family == 4 && beatNumber % 4 == 3 ? 7 : 0;
			double bassPhase = 2.0 * Math.PI * Frequency(root - 12 + bassOffset) * time;
			double bassGate = family == 1 ? (beatNumber % 2 == 0 ? 1.0 : 0.28)
				: family == 2 ? (step % 3 == 0 ? 1.0 : 0.38) : 0.78;
			double bass = (Math.Sin(bassPhase) * 0.76 + Triangle(bassPhase) * 0.18) * bassGate;
			int[] arpOffsets = arpPatterns[family];
			double arpPhase = 2.0 * Math.PI * Frequency(root + arpOffsets[(step + measure) % 4]) * time;
			double arp = (family == 2 ? Triangle(arpPhase) : Math.Sin(arpPhase))
				+ Math.Sin(arpPhase * 2.0) * 0.18;
			int counterOffset = arpOffsets[(step / 2 + style + section) % 4] + (section >= 2 ? 12 : 0);
			double counterPhase = 2.0 * Math.PI * Frequency(root + counterOffset) * time;
			double counterEnvelope = Math.Pow(1.0 - stepPhase, 1.25);
			double counter = (Math.Sin(counterPhase) + Math.Sin(counterPhase * 1.997) * 0.22)
				* counterEnvelope;
			double beatPhase = time % beat / beat;
			double kickPattern = family == 3 ? (beatNumber % 4 == 0 ? 1.0 : 0.0)
				: family == 4 ? (beatNumber % 4 == 0 || beatNumber % 4 == 3 ? 1.0 : 0.25) : 1.0;
			double kick = Math.Sin(2.0 * Math.PI * (76.0 - 36.0 * beatPhase) * time)
				* Math.Pow(1.0 - beatPhase, 9) * kickPattern;
			double hatPhase = (time + beat * (family == 2 ? 0.25 : 0.5)) % beat / beat;
			double hat = Math.Sin(index * (1.37 + family * 0.113)) * Math.Pow(1.0 - hatPhase, family == 3 ? 28 : 16);
			double snarePhase = (time + beat * 2.0) % (beat * 4.0) / beat;
			double snare = Math.Sin(index * 0.731) * Math.Pow(Math.Max(1.0 - snarePhase, 0.0), 12);
			double leadLevel = (family == 3 ? 0.28 : family == 2 ? 0.30 : 0.36)
				* (section == 0 ? 0.72 : 1.0);
			double arpLevel = (family == 1 ? 0.08 : family == 3 ? 0.22 : 0.15)
				* (section == 1 ? 0.55 : 1.0);
			double counterLevel = section == 0 ? 0.0 : section == 1 ? 0.055 : section == 2 ? 0.10 : 0.075;
			double drumLevel = section == 0 ? 0.68 : section == 3 ? 1.12 : 1.0;
			double value = (lead * leadLevel + bass * (section == 1 ? 0.24 : 0.29)
				+ arp * arpLevel * envelope + counter * counterLevel
				+ (kick * 0.18 + hat * 0.04 + snare * (family == 4 ? 0.07 : 0.035)) * drumLevel) * level;
			double edgeFade = Math.Min(Math.Min(index / (SampleRate * 0.012),
				(count - index - 1) / (SampleRate * 0.012)), 1.0);
			value *= Math.Max(edgeFade, 0.0);
			samples[index] = (short)(Math.Max(-1.0, Math.Min(1.0, value)) * 32767.0);
		}

		using (BinaryWriter writer = new BinaryWriter(File.Open(path, FileMode.Create)))
		{
			int dataSize = samples.Length * 2;
			writer.Write(System.Text.Encoding.ASCII.GetBytes("RIFF"));
			writer.Write(36 + dataSize);
			writer.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt "));
			writer.Write(16);
			writer.Write((short)1);
			writer.Write((short)1);
			writer.Write(SampleRate);
			writer.Write(SampleRate * 2);
			writer.Write((short)2);
			writer.Write((short)16);
			writer.Write(System.Text.Encoding.ASCII.GetBytes("data"));
			writer.Write(dataSize);
			foreach (short sample in samples) writer.Write(sample);
		}
	}
}
'@

function Write-Wave([string]$path, [int16[]]$samples)
{
	$stream = [IO.File]::Open($path, [IO.FileMode]::Create)
	$writer = [IO.BinaryWriter]::new($stream)
	$dataSize = $samples.Length * 2
	$writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
	$writer.Write([int](36 + $dataSize))
	$writer.Write([Text.Encoding]::ASCII.GetBytes('WAVEfmt '))
	$writer.Write([int]16)
	$writer.Write([int16]1)
	$writer.Write([int16]1)
	$writer.Write([int]$sampleRate)
	$writer.Write([int]($sampleRate * 2))
	$writer.Write([int16]2)
	$writer.Write([int16]16)
	$writer.Write([Text.Encoding]::ASCII.GetBytes('data'))
	$writer.Write([int]$dataSize)
	foreach ($sample in $samples) { $writer.Write($sample) }
	$writer.Dispose()
	$stream.Dispose()
}

function Get-Triangle([double]$phase)
{
	return 2.0 * [Math]::Asin([Math]::Sin($phase)) / [Math]::PI
}

function New-Sweep([string]$name, [double]$duration, [double]$startFrequency,
	[double]$endFrequency, [double]$level = 0.55, [double]$noise = 0.0, [int]$pulses = 1)
{
	$count = [int]($duration * $sampleRate)
	$samples = [int16[]]::new($count)
	$random = [Random]::new(1979 + $name.Length)
	$phase = 0.0
	for ($index = 0; $index -lt $count; $index++)
	{
		$time = $index / $sampleRate
		$progress = $index / [Math]::Max($count - 1, 1)
		$frequency = $startFrequency + ($endFrequency - $startFrequency) * $progress
		$attack = [Math]::Min($progress / 0.045, 1.0)
		$release = [Math]::Pow(1.0 - $progress, 1.7)
		$gate = if ($pulses -le 1) { 1.0 } else { 0.3 + 0.7 * [Math]::Pow([Math]::Max([Math]::Sin($progress * $pulses * [Math]::PI), 0), 0.35) }
		$phase += 2.0 * [Math]::PI * $frequency / $sampleRate
		$tone = [Math]::Sin($phase + [Math]::Sin($phase * 0.51) * 0.65)
		$harmonic = [Math]::Sin($phase * 2.01) * 0.38 + [Math]::Sin($phase * 3.03) * 0.16
		$randomSample = $random.NextDouble() * 2.0 - 1.0
		$transient = [Math]::Sin(2.0 * [Math]::PI * 2200.0 * $time) * [Math]::Pow(1.0 - $progress, 12)
		$value = (($tone * 0.62 + $harmonic * 0.28 + $transient * 0.10) * (1.0 - $noise) + $randomSample * $noise)
		$value *= $level * $attack * $release * $gate
		$samples[$index] = [int16]([Math]::Max(-1.0, [Math]::Min(1.0, $value)) * 32767)
	}
	Write-Wave (Join-Path $soundRoot $name) $samples
}

function Get-Frequency([double]$midi)
{
	return 440.0 * [Math]::Pow(2.0, ($midi - 69.0) / 12.0)
}

function New-Music([string]$name, [double]$bpm, [int[]]$roots, [int[]]$melody,
	[int]$measures, [double]$level, [int]$style = 0)
{
	[BrickoutSynth]::WriteMusic((Join-Path $soundRoot $name), $bpm, $roots, $melody, $measures, $level, $style)
}

function Get-ThemeRoots([int[]]$progression, [int]$theme, [int]$measures)
{
	$roots = [int[]]::new($measures)
	$middleShift = if (($theme % 2) -eq 0) { 2 } else { -2 }
	$sectionShifts = @(0, $middleShift, 5, 0)
	for ($measure = 0; $measure -lt $measures; $measure++)
	{
		$section = [Math]::Min([Math]::Floor($measure * 4 / $measures), 3)
		$chord = ($measure + [Math]::Floor($measure / 8)) % $progression.Count
		$roots[$measure] = $progression[$chord] + $sectionShifts[$section]
	}
	return $roots
}

function Get-ThemeMelody([int]$theme, [int]$measures, [int]$subdivision)
{
	# These original interval phrases are transformed per section instead of repeating a short loop.
	$motifs = @(
		@(0,7,12,10,3,7,15,12, 7,10,17,15,12,7,3,5),
		@(0,3,7,10,12,7,5,3, 8,12,15,10,7,5,3,0),
		@(0,12,7,15,10,17,12,19, 15,10,7,12,5,3,7,0),
		@(0,5,10,12,7,15,10,17, 3,7,14,12,10,5,7,3),
		@(0,7,10,14,17,10,7,12, 2,9,16,12,19,14,9,7),
		@(0,3,10,7,15,12,10,5, 7,14,17,12,10,7,3,0),
		@(0,8,12,15,7,10,17,12, 5,12,20,15,10,8,3,7),
		@(0,5,12,10,17,15,7,10, 3,10,14,12,19,15,10,5),
		@(0,10,7,12,15,19,12,17, 5,8,15,12,10,7,3,0),
		@(0,7,14,10,17,12,19,15, 2,9,16,14,12,7,5,9)
	)
	$stepsPerMeasure = 4 * $subdivision
	$melody = [int[]]::new($measures * $stepsPerMeasure)
	$motif = $motifs[($theme - 1) % $motifs.Count]
	for ($step = 0; $step -lt $melody.Count; $step++)
	{
		$measure = [Math]::Floor($step / $stepsPerMeasure)
		$local = $step % $stepsPerMeasure
		$section = [Math]::Min([Math]::Floor($measure * 4 / $measures), 3)
		$rotation = (($measure % 4) * (1 + ($theme % 3)) + $section * 3) % $motif.Count
		$note = $motif[($local + $rotation) % $motif.Count]
		if ($section -eq 1 -and (($measure + $theme) % 2) -eq 0) { $note += 5 }
		if ($section -eq 2 -and ($local % 4) -eq 3) { $note += 12 }
		if ($section -eq 3 -and $measure -eq ($measures - 1)) { $note = if ($local -eq 0) { 0 } else { -99 } }
		elseif ((($step + $theme * 3 + $measure) % (7 + ($theme % 4))) -eq 0) { $note = -99 }
		$melody[$step] = $note
	}
	return $melody
}

New-Sweep 'ui-hover.wav' 0.055 980 1320 0.50 0.00 1
New-Sweep 'ui-select.wav' 0.115 720 1480 0.66 0.00 2
New-Sweep 'ball-paddle.wav' 0.095 520 280 0.78 0.01 1
New-Sweep 'ball-wall.wav' 0.055 1180 760 0.62 0.00 1
New-Sweep 'ball-brick.wav' 0.085 1580 920 0.70 0.00 1
New-Sweep 'ball-bumper.wav' 0.145 620 1760 0.74 0.01 2
New-Sweep 'ball-lost.wav' 0.480 540 110 0.72 0.04 1
New-Sweep 'power-drop.wav' 0.220 1680 760 0.62 0.00 3
New-Sweep 'power-life.wav' 0.420 660 1760 0.70 0.00 4
New-Sweep 'power-multiball.wav' 0.360 520 1980 0.70 0.00 5
New-Sweep 'power-bomb.wav' 0.520 420 72 0.80 0.24 2
New-Sweep 'power-wide.wav' 0.360 880 380 0.70 0.00 3
New-Sweep 'power-piercing.wav' 0.390 920 2380 0.72 0.00 5
New-Sweep 'power-shockwave.wav' 0.680 360 58 0.82 0.30 3
New-Sweep 'achievement.wav' 0.720 740 2100 0.68 0.00 6
New-Sweep 'game-over.wav' 0.900 520 70 0.72 0.06 2
New-Sweep 'victory.wav' 0.950 620 1900 0.70 0.00 7

$musicMeasures = 16
$menuRoots = Get-ThemeRoots @(45, 41, 38, 43) 8 $musicMeasures
$menuMelody = Get-ThemeMelody 8 $musicMeasures 1
New-Music 'music-menu.wav' 100 $menuRoots $menuMelody $musicMeasures 0.34 8
$rootSets = @(
	@(45,48,41,43), @(43,46,50,48), @(48,44,41,46), @(41,45,48,43), @(50,46,43,45),
	@(38,41,45,43), @(46,50,43,48), @(44,48,51,46), @(40,43,47,45), @(49,45,42,47),
	@(42,46,49,44), @(47,43,50,45), @(39,46,42,44), @(51,48,44,46), @(45,50,47,42),
	@(43,49,46,41), @(48,42,45,50), @(41,47,44,49), @(46,40,48,43), @(50,45,48,52)
)
for ($theme = 1; $theme -le 20; $theme++)
{
	$bpm = 116 + (($theme - 1) % 5) * 6 + [Math]::Floor(($theme - 1) / 5) * 2
	$family = ($theme - 1) % 5
	$subdivision = if ($family -eq 2) { 4 } elseif ($family -eq 3) { 1 } else { 2 }
	$roots = Get-ThemeRoots $rootSets[$theme - 1] $theme $musicMeasures
	$melody = Get-ThemeMelody $theme $musicMeasures $subdivision
	New-Music ("music-theme-{0:D2}.wav" -f $theme) $bpm $roots $melody $musicMeasures 0.36 ($theme - 1)
}
$overdriveRoots = Get-ThemeRoots @(45,48,50,52) 19 $musicMeasures
$overdriveMelody = Get-ThemeMelody 19 $musicMeasures 4
New-Music 'music-overdrive.wav' 168 $overdriveRoots $overdriveMelody $musicMeasures 0.38 2
Remove-Item -LiteralPath (Join-Path $soundRoot 'music-game.wav') -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath (Join-Path $soundRoot 'power-duplicate.wav') -Force -ErrorAction SilentlyContinue

Copy-Item -LiteralPath (Join-Path $soundRoot 'ball-wall.wav') -Destination (Join-Path $soundRoot 'ball-impact-general.wav') -Force
Copy-Item -LiteralPath (Join-Path $soundRoot 'ball-brick.wav') -Destination (Join-Path $soundRoot 'ball-impact-point.wav') -Force

Write-Host 'Brickout arcade SFX and 20 seamless stage themes are up to date.'
