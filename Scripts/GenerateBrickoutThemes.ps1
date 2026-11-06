param(
	[string]$AssetRoot = (Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force -Path $AssetRoot | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class BrickoutThemeTint
{
	public static Bitmap Colorize(string path, Color primary, Color secondary)
	{
		using (Bitmap loaded = new Bitmap(path))
		{
			Bitmap result = new Bitmap(1280, 720, PixelFormat.Format32bppArgb);
			using (Graphics graphics = Graphics.FromImage(result))
				graphics.DrawImage(loaded, 0, 0, result.Width, result.Height);

			Rectangle rectangle = new Rectangle(0, 0, result.Width, result.Height);
			BitmapData data = result.LockBits(rectangle, ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
			byte[] pixels = new byte[Math.Abs(data.Stride) * data.Height];
			Marshal.Copy(data.Scan0, pixels, 0, pixels.Length);
			for (int y = 0; y < result.Height; ++y)
			{
				for (int x = 0; x < result.Width; ++x)
				{
					int offset = y * data.Stride + x * 4;
					double brightness = Math.Max(pixels[offset], Math.Max(pixels[offset + 1], pixels[offset + 2])) / 255.0;
					double edge = Math.Pow(Math.Abs(x - result.Width * 0.5) / (result.Width * 0.5), 1.35);
					double blend = 0.18 + edge * 0.64;
					double red = primary.R + (secondary.R - primary.R) * blend;
					double green = primary.G + (secondary.G - primary.G) * blend;
					double blue = primary.B + (secondary.B - primary.B) * blend;
					double glow = Math.Pow(brightness, 0.82);
					pixels[offset] = Clamp(pixels[offset] * 0.08 + blue * glow * 1.08);
					pixels[offset + 1] = Clamp(pixels[offset + 1] * 0.08 + green * glow * 1.08);
					pixels[offset + 2] = Clamp(pixels[offset + 2] * 0.08 + red * glow * 1.08);
				}
			}
			Marshal.Copy(pixels, 0, data.Scan0, pixels.Length);
			result.UnlockBits(data);
			return result;
		}
	}

	private static byte Clamp(double value)
	{
		return (byte)Math.Max(0.0, Math.Min(255.0, value));
	}
}
'@ -ReferencedAssemblies System.Drawing

$basePath = Join-Path $AssetRoot 'background.png'
if (!(Test-Path -LiteralPath $basePath))
{
	throw "Brickout base background was not found: $basePath"
}

$palettes = @(
	@('#FF3500','#FFB000'), @('#00F5D4','#45FF72'), @('#9B3DFF','#FF24D6'), @('#B8FF16','#00E5FF'),
	@('#00C8FF','#FF2BB5'), @('#FF5733','#FFE033'), @('#35FFB4','#3185FF'), @('#E64DFF','#FF406C'),
	@('#27DFFF','#8D5CFF'), @('#FF8A24','#FF335F'), @('#4CFF6A','#C9FF29'), @('#4DDCFF','#FF54CF'),
	@('#FFD43B','#FF4D1F'), @('#34FFD5','#7A5CFF'), @('#FF49B8','#7B38FF'), @('#72FFEF','#39A8FF'),
	@('#C5FF38','#29E2A6'), @('#FF6245','#D93BFF'), @('#52B7FF','#8AFFE3'), @('#F4F8FF','#27CFFF')
)

function To-Color([string]$hex, [int]$alpha = 255)
{
	$color = [Drawing.ColorTranslator]::FromHtml($hex)
	return [Drawing.Color]::FromArgb($alpha, $color.R, $color.G, $color.B)
}

for ($theme = 1; $theme -le 20; $theme++)
{
	$primary = $palettes[$theme - 1][0]
	$secondary = $palettes[$theme - 1][1]
	$source = [BrickoutThemeTint]::Colorize($basePath, (To-Color $primary), (To-Color $secondary))
	$bitmap = [Drawing.Bitmap]::new(1280, 720, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$graphics.DrawImage($source, 0, 0, 1280, 720)

	$random = [Random]::new(3907 + $theme * 811)
	$starPrimary = [Drawing.SolidBrush]::new((To-Color $primary 145))
	$starSecondary = [Drawing.SolidBrush]::new((To-Color $secondary 120))
	$linePrimary = [Drawing.Pen]::new((To-Color $primary 115), 1.5)
	$lineSecondary = [Drawing.Pen]::new((To-Color $secondary 100), 1.5)
	$glowPrimary = [Drawing.Pen]::new((To-Color $primary 55), 10)
	$glowSecondary = [Drawing.Pen]::new((To-Color $secondary 50), 10)
	$brightPrimary = [Drawing.Pen]::new((To-Color $primary 210), 2.5)
	$brightSecondary = [Drawing.Pen]::new((To-Color $secondary 200), 2.5)

	for ($star = 0; $star -lt 70 + ($theme % 4) * 12; $star++)
	{
		$x = $random.Next(155, 1125)
		$y = $random.Next(75, 665)
		$size = if (($star + $theme) % 9 -eq 0) { 3 } else { 1 }
		$graphics.FillEllipse($(if ($star % 2) { $starPrimary } else { $starSecondary }), $x, $y, $size, $size)
	}

	$nodeYs = switch ($theme % 4) { 0 { @(190,505) } 1 { @(235,490) } 2 { @(175,360,545) } default { @(250,470) } }
	foreach ($nodeY in $nodeYs)
	{
		foreach ($nodeX in @(58, 1222))
		{
			$penGlow = if ($nodeX -lt 640) { $glowPrimary } else { $glowSecondary }
			$penBright = if ($nodeX -lt 640) { $brightPrimary } else { $brightSecondary }
			$graphics.DrawEllipse($penGlow, $nodeX - 29, $nodeY - 29, 58, 58)
			$graphics.DrawEllipse($penBright, $nodeX - 26, $nodeY - 26, 52, 52)
			$graphics.DrawEllipse($penBright, $nodeX - 13, $nodeY - 13, 26, 26)
			$graphics.DrawLine($penBright, $nodeX - 34, $nodeY, $nodeX - 26, $nodeY)
			$graphics.DrawLine($penBright, $nodeX + 26, $nodeY, $nodeX + 34, $nodeY)
		}
	}

	$family = ($theme - 1) % 5
	if ($family -eq 0)
	{
		for ($i = 0; $i -lt 5; $i++)
		{
			$y = 110 + $i * 112
			$graphics.DrawArc($linePrimary, 120, $y - 42, 210, 84, 185, 170)
			$graphics.DrawArc($lineSecondary, 950, $y - 42, 210, 84, 5, 170)
		}
	}
	elseif ($family -eq 1)
	{
		for ($i = 0; $i -lt 7; $i++)
		{
			$inset = 165 + $i * 18
			$graphics.DrawRectangle($(if ($i % 2) { $linePrimary } else { $lineSecondary }), $inset, 78 + $i * 12, 1280 - $inset * 2, 564 - $i * 24)
		}
	}
	elseif ($family -eq 2)
	{
		for ($i = 0; $i -lt 10; $i++)
		{
			$y = 95 + $i * 58
			$graphics.DrawLine($linePrimary, 155, $y, 235 + ($i % 3) * 35, $y)
			$graphics.DrawLine($lineSecondary, 1045 - ($i % 3) * 35, $y, 1125, $y)
		}
	}
	elseif ($family -eq 3)
	{
		for ($i = 0; $i -lt 12; $i++)
		{
			$x = 210 + ($i % 6) * 172; $y = 125 + [Math]::Floor($i / 6) * 440
			$points = [Drawing.Point[]]@([Drawing.Point]::new($x,$y-12),[Drawing.Point]::new($x+12,$y),[Drawing.Point]::new($x,$y+12),[Drawing.Point]::new($x-12,$y))
			$graphics.DrawPolygon($(if ($i % 2) { $linePrimary } else { $lineSecondary }), $points)
		}
	}
	else
	{
		for ($i = 0; $i -lt 6; $i++)
		{
			$radius = 55 + $i * 24
			$graphics.DrawArc($linePrimary, 640-$radius, 355-$radius, $radius*2, $radius*2, 205, 55)
			$graphics.DrawArc($lineSecondary, 640-$radius, 355-$radius, $radius*2, $radius*2, 25, 55)
		}
	}

	$starPrimary.Dispose(); $starSecondary.Dispose(); $linePrimary.Dispose(); $lineSecondary.Dispose()
	$glowPrimary.Dispose(); $glowSecondary.Dispose(); $brightPrimary.Dispose(); $brightSecondary.Dispose()
	$graphics.Dispose(); $source.Dispose()
	$path = Join-Path $AssetRoot ("background-theme-{0:D2}.png" -f $theme)
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
}

Write-Host 'Generated 20 original neon-circuit Brickout stage themes.'
