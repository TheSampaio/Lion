param(
	[string]$AssetRoot = (Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force -Path $AssetRoot | Out-Null

$basePath = Join-Path $AssetRoot 'background.png'
if (!(Test-Path -LiteralPath $basePath))
{
	throw "Brickout base background was not found: $basePath"
}

$palettes = @(
	@('#00D9FF','#FF2BB5'), @('#39F7FF','#8D5CFF'), @('#00E5FF','#FF5A8A'), @('#55FFD5','#36A8FF'),
	@('#44CCFF','#E14DFF'), @('#28F0D0','#FF4CAC'), @('#4DA3FF','#FF713D'), @('#36E6FF','#A862FF'),
	@('#00D7FF','#FFCC38'), @('#54F6FF','#FF497D'), @('#40FFB8','#5785FF'), @('#7AE6FF','#FF4FC4'),
	@('#32D5FF','#FF8A45'), @('#59FFD0','#C64DFF'), @('#45B8FF','#FF4A6E'), @('#4DEBFF','#FFB33D'),
	@('#7CFFDF','#8464FF'), @('#44CFFF','#FF64A8'), @('#75DEFF','#D653FF'), @('#E7FAFF','#27CFFF')
)

function To-Color([string]$hex, [int]$alpha = 255)
{
	$color = [Drawing.ColorTranslator]::FromHtml($hex)
	return [Drawing.Color]::FromArgb($alpha, $color.R, $color.G, $color.B)
}

for ($theme = 1; $theme -le 20; $theme++)
{
	$source = [Drawing.Bitmap]::FromFile($basePath)
	$bitmap = [Drawing.Bitmap]::new(1280, 720, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$graphics.DrawImage($source, 0, 0, 1280, 720)

	$primary = $palettes[$theme - 1][0]
	$secondary = $palettes[$theme - 1][1]
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
