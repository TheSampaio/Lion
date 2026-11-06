$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$prepareArt = Join-Path $PSScriptRoot 'PrepareBrickoutArt.ps1'
$prepareUi = Join-Path $PSScriptRoot 'GenerateBrickoutUiAssets.ps1'
$prepareThemes = Join-Path $PSScriptRoot 'GenerateBrickoutThemes.ps1'
& $prepareArt | Out-Null
& $prepareUi | Out-Null
& $prepareThemes | Out-Null

$assetRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout'))
$sourcePath = Join-Path $assetRoot 'Source\power-icons-atlas-v2.png'
$atlas = [Drawing.Bitmap]::new($sourcePath)

$icons = @(
	@{ Name = 'power-life.png'; Column = 0; Row = 0 },
	@{ Name = 'power-multiball.png'; Column = 1; Row = 0 },
	@{ Name = 'power-bomb.png'; Column = 2; Row = 0 },
	@{ Name = 'power-wide.png'; Column = 0; Row = 1 },
	@{ Name = 'power-shockwave.png'; Column = 2; Row = 1 }
)

foreach ($icon in $icons)
{
	$bitmap = [Drawing.Bitmap]::new(64, 64, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.Clear([Drawing.Color]::Transparent)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::HighQuality
	$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$path = [Drawing.Drawing2D.GraphicsPath]::new()
	$path.AddEllipse(1, 1, 62, 62)
	$graphics.SetClip($path)
	$source = [Drawing.Rectangle]::new($icon.Column * 512, $icon.Row * 512, 512, 512)
	$target = [Drawing.Rectangle]::new(0, 0, 64, 64)
	$graphics.DrawImage($atlas, $target, $source, [Drawing.GraphicsUnit]::Pixel)
	$graphics.ResetClip()
	$graphics.Dispose()
	$path.Dispose()
	$bitmap.Save((Join-Path $assetRoot $icon.Name), [Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
}

$atlas.Dispose()

for ($frame = 0; $frame -lt 12; $frame++)
{
	$bitmap = [Drawing.Bitmap]::new(80, 80, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.Clear([Drawing.Color]::Transparent)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$ratio = $frame / 11.0
	$cyanGlow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(85, 40, 215, 255), 9)
	$cyan = [Drawing.Pen]::new([Drawing.Color]::FromArgb(245, 180, 250, 255), 4)
	$magenta = [Drawing.Pen]::new([Drawing.Color]::FromArgb(230, 255, 55, 190), 2)
	$graphics.DrawArc($cyanGlow, 7, 7, 66, 66, -90, 360 * $ratio)
	$graphics.DrawArc($cyan, 7, 7, 66, 66, -90, 360 * $ratio)
	$graphics.DrawArc($magenta, 12, 12, 56, 56, -90, 360 * $ratio)
	$cyanGlow.Dispose()
	$cyan.Dispose()
	$magenta.Dispose()
	$graphics.Dispose()
	$bitmap.Save((Join-Path $assetRoot ("power-timer-{0:D2}.png" -f $frame)), [Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
}

$achievementSources = @(
	'power-life.png', 'power-bomb.png', 'power-multiball.png', 'power-life.png', 'power-shockwave.png',
	'power-wide.png', 'power-bomb.png', 'power-piercing.png', 'power-shockwave.png', 'power-multiball.png'
)
$achievementNames = @('first', 'century', 'combo', 'power', 'wave', 'clear', 'score', 'depth', 'time', 'master')
for ($index = 0; $index -lt $achievementNames.Count; $index++)
{
	Copy-Item -LiteralPath (Join-Path $assetRoot $achievementSources[$index]) -Destination (Join-Path $assetRoot ("achievement-{0}.png" -f $achievementNames[$index])) -Force
}

$whiteAssets = @('ball.png', 'player.png')
$matrix = [Drawing.Imaging.ColorMatrix]::new(@(
	[float[]]@(0.2126, 0.2126, 0.2126, 0, 0),
	[float[]]@(0.7152, 0.7152, 0.7152, 0, 0),
	[float[]]@(0.0722, 0.0722, 0.0722, 0, 0),
	[float[]]@(0, 0, 0, 1, 0),
	[float[]]@(0.22, 0.22, 0.22, 0, 1)
))
$attributes = [Drawing.Imaging.ImageAttributes]::new()
$attributes.SetColorMatrix($matrix)
foreach ($name in $whiteAssets)
{
	$path = Join-Path $assetRoot $name
	$source = [Drawing.Bitmap]::new($path)
	$output = [Drawing.Bitmap]::new($source.Width, $source.Height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($output)
	$graphics.DrawImage($source, [Drawing.Rectangle]::new(0, 0, $source.Width, $source.Height), 0, 0,
		$source.Width, $source.Height, [Drawing.GraphicsUnit]::Pixel, $attributes)
	$graphics.Dispose()
	$source.Dispose()
	$output.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$output.Dispose()
}
$attributes.Dispose()

$blueAssets = @(
	'bumper.png', 'pinball-rail.png', 'rail.png',
	'wall-horizontal.png', 'wall-horizontal-v2.png', 'wall-vertical.png', 'wall-vertical-v2.png'
)
$blueMatrix = [Drawing.Imaging.ColorMatrix]::new(@(
	[float[]]@(0.0043, 0.117, 0.2126, 0, 0),
	[float[]]@(0.0143, 0.393, 0.7152, 0, 0),
	[float[]]@(0.0014, 0.040, 0.0722, 0, 0),
	[float[]]@(0, 0, 0, 1, 0),
	[float[]]@(0.02, 0.08, 0.15, 0, 1)
))
$blueAttributes = [Drawing.Imaging.ImageAttributes]::new()
$blueAttributes.SetColorMatrix($blueMatrix)
foreach ($name in $blueAssets)
{
	$path = Join-Path $assetRoot $name
	$source = [Drawing.Bitmap]::new($path)
	$output = [Drawing.Bitmap]::new($source.Width, $source.Height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($output)
	$graphics.DrawImage($source, [Drawing.Rectangle]::new(0, 0, $source.Width, $source.Height), 0, 0,
		$source.Width, $source.Height, [Drawing.GraphicsUnit]::Pixel, $blueAttributes)
	$graphics.Dispose()
	$source.Dispose()
	$output.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$output.Dispose()
}
$blueAttributes.Dispose()

Write-Host 'Brickout power icons, radial timers, achievement badges, white player sprites and blue arena sprites are up to date.'
