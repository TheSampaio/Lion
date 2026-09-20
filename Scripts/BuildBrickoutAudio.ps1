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
			double leadPhase = 2.0 * Math.PI * Frequency(root + 12 + melody[step % melody.Length]) * time;
			double lead = (Math.Sin(leadPhase + Math.Sin(leadPhase * 0.5) * 0.8)
				+ Math.Sin(leadPhase * 2.01) * 0.28) * envelope;
			double bassPhase = 2.0 * Math.PI * Frequency(root - 12) * time;
			double bass = Math.Sin(bassPhase) * 0.72 + Math.Sin(bassPhase * 2.0) * 0.18;
			int[] arpOffsets = { 0, 7, 12, 7 };
			double arpPhase = 2.0 * Math.PI * Frequency(root + arpOffsets[step % 4]) * time;
			double arp = Math.Sin(arpPhase) + Math.Sin(arpPhase * 2.0) * 0.22;
			double beatPhase = time % beat / beat;
			double kick = Math.Sin(2.0 * Math.PI * (70.0 - 28.0 * beatPhase) * time) * Math.Pow(1.0 - beatPhase, 8);
			double offbeat = (time + beat * 0.5) % beat / beat;
			double hat = Math.Sin(index * 1.618) * Math.Pow(1.0 - offbeat, 18);
			double value = (lead * 0.38 + bass * 0.32 + arp * 0.18 * envelope
				+ kick * 0.17 + hat * 0.035) * level;
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
	[int]$measures, [double]$level)
{
	[BrickoutSynth]::WriteMusic((Join-Path $soundRoot $name), $bpm, $roots, $melody, $measures, $level)
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
New-Sweep 'power-duplicate.wav' 0.390 700 1440 0.70 0.00 4
New-Sweep 'power-shockwave.wav' 0.680 360 58 0.82 0.30 3
New-Sweep 'achievement.wav' 0.720 740 2100 0.68 0.00 6
New-Sweep 'game-over.wav' 0.900 520 70 0.72 0.06 2
New-Sweep 'victory.wav' 0.950 620 1900 0.70 0.00 7

New-Music 'music-menu.wav' 100 @(45, 41, 38, 43) @(0, 7, 12, 7, 3, 10, 12, 10, 0, 7, 15, 12, 3, 7, 10, 7) 12 0.34
New-Music 'music-game.wav' 128 @(45, 48, 41, 43) @(0, 7, 12, 15, 12, 7, 3, 7, 0, 10, 12, 17, 15, 12, 7, 3) 16 0.38

Copy-Item -LiteralPath (Join-Path $soundRoot 'ball-wall.wav') -Destination (Join-Path $soundRoot 'ball-impact-general.wav') -Force
Copy-Item -LiteralPath (Join-Path $soundRoot 'ball-brick.wav') -Destination (Join-Path $soundRoot 'ball-impact-point.wav') -Force

Write-Host 'Brickout arcade SFX and seamless menu/game music are up to date.'
