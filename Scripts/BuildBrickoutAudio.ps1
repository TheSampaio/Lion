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
		int measures, double level)
	{
		double beat = 60.0 / bpm;
		int count = (int)(measures * 4.0 * beat * SampleRate);
		short[] samples = new short[count];
		for (int index = 0; index < count; ++index)
		{
			double time = (double)index / SampleRate;
			int beatNumber = (int)Math.Floor(time / beat);
			int measure = beatNumber / 4 % roots.Length;
			int root = roots[measure];
			double stepLength = beat / 2.0;
			int step = (int)Math.Floor(time / stepLength);
			double stepPhase = time % stepLength / stepLength;
			double envelope = Math.Min(stepPhase / 0.08, 1.0) * Math.Pow(1.0 - stepPhase, 0.55);
			double lead = Triangle(2.0 * Math.PI * Frequency(root + 12 + melody[step % melody.Length]) * time) * envelope;
			double bass = Triangle(2.0 * Math.PI * Frequency(root - 12) * time);
			int[] arpOffsets = { 0, 7, 12, 7 };
			double arp = Triangle(2.0 * Math.PI * Frequency(root + arpOffsets[step % 4]) * time);
			double beatPhase = time % beat / beat;
			double kick = Math.Sin(2.0 * Math.PI * (70.0 - 28.0 * beatPhase) * time) * Math.Pow(1.0 - beatPhase, 8);
			double value = (lead * 0.42 + bass * 0.28 + arp * 0.18 * envelope + kick * 0.12) * level;
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
	for ($index = 0; $index -lt $count; $index++)
	{
		$time = $index / $sampleRate
		$progress = $index / [Math]::Max($count - 1, 1)
		$frequency = $startFrequency + ($endFrequency - $startFrequency) * $progress
		$attack = [Math]::Min($progress / 0.045, 1.0)
		$release = [Math]::Pow(1.0 - $progress, 1.7)
		$gate = if ($pulses -le 1) { 1.0 } else { 0.55 + 0.45 * [Math]::Sin($progress * $pulses * [Math]::PI) }
		$tone = Get-Triangle (2.0 * [Math]::PI * $frequency * $time)
		$square = if ([Math]::Sin(2.0 * [Math]::PI * $frequency * 0.5 * $time) -ge 0) { 1.0 } else { -1.0 }
		$randomSample = $random.NextDouble() * 2.0 - 1.0
		$value = (($tone * 0.78 + $square * 0.22) * (1.0 - $noise) + $randomSample * $noise)
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
	[int]$measures, [double]$level)
{
	[BrickoutSynth]::WriteMusic((Join-Path $soundRoot $name), $bpm, $roots, $melody, $measures, $level)
}

New-Sweep 'ui-hover.wav' 0.065 430 560 0.34 0.00 1
New-Sweep 'ui-select.wav' 0.120 610 900 0.42 0.00 2
New-Sweep 'ball-paddle.wav' 0.095 185 105 0.48 0.10 1
New-Sweep 'ball-wall.wav' 0.060 330 245 0.34 0.04 1
New-Sweep 'ball-brick.wav' 0.085 620 410 0.46 0.02 1
New-Sweep 'ball-bumper.wav' 0.145 360 920 0.44 0.03 2
New-Sweep 'ball-lost.wav' 0.480 330 92 0.48 0.06 1
New-Sweep 'power-drop.wav' 0.220 940 520 0.35 0.00 3
New-Sweep 'power-life.wav' 0.420 520 1040 0.46 0.00 4
New-Sweep 'power-multiball.wav' 0.360 420 1180 0.42 0.00 5
New-Sweep 'power-bomb.wav' 0.520 185 58 0.58 0.38 2
New-Sweep 'power-wide.wav' 0.360 480 250 0.42 0.00 3
New-Sweep 'power-duplicate.wav' 0.390 520 780 0.42 0.00 4
New-Sweep 'power-shockwave.wav' 0.680 190 42 0.62 0.48 3
New-Sweep 'achievement.wav' 0.720 520 1320 0.44 0.00 6
New-Sweep 'game-over.wav' 0.900 310 62 0.48 0.12 2
New-Sweep 'victory.wav' 0.950 440 1180 0.46 0.00 7

New-Music 'music-menu.wav' 100 @(45, 41, 38, 43) @(0, 7, 12, 7, 3, 10, 12, 10, 0, 7, 15, 12, 3, 7, 10, 7) 12 0.34
New-Music 'music-game.wav' 128 @(45, 48, 41, 43) @(0, 7, 12, 15, 12, 7, 3, 7, 0, 10, 12, 17, 15, 12, 7, 3) 16 0.38

Copy-Item -LiteralPath (Join-Path $soundRoot 'ball-wall.wav') -Destination (Join-Path $soundRoot 'retro-impact-general.wav') -Force
Copy-Item -LiteralPath (Join-Path $soundRoot 'ball-brick.wav') -Destination (Join-Path $soundRoot 'retro-impact-point.wav') -Force

Write-Host 'Brickout retro SFX and seamless menu/game music are up to date.'
