$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$assetRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout'))
$uiRoot = [IO.Path]::GetFullPath((Join-Path $assetRoot '..\UI'))
$spriteSourcePath = Join-Path $PSScriptRoot 'GeneratedArt\brickout-pixel-source.png'
$buttonSourcePath = Join-Path $PSScriptRoot 'GeneratedArt\button-panel-pixel-source.png'
$backgroundSourcePath = Join-Path $PSScriptRoot 'GeneratedArt\background-pixel-source.png'
$atlasPath = Join-Path $assetRoot 'brickout-spritesheet.png'
$buttonPath = Join-Path $uiRoot 'button-panel.png'
$backgroundPath = Join-Path $assetRoot 'background.png'
$spriteSize = 16
$alphaThreshold = 32

function Get-AlphaBounds([Drawing.Bitmap]$bitmap, [Drawing.Rectangle]$region)
{
	$minimumX = $region.Right
	$minimumY = $region.Bottom
	$maximumX = -1
	$maximumY = -1

	for ($y = $region.Top; $y -lt $region.Bottom; $y++)
	{
		for ($x = $region.Left; $x -lt $region.Right; $x++)
		{
			if ($bitmap.GetPixel($x, $y).A -lt $alphaThreshold)
			{
				continue
			}

			$minimumX = [Math]::Min($minimumX, $x)
			$minimumY = [Math]::Min($minimumY, $y)
			$maximumX = [Math]::Max($maximumX, $x)
			$maximumY = [Math]::Max($maximumY, $y)
		}
	}

	if ($maximumX -lt $minimumX -or $maximumY -lt $minimumY)
	{
		throw "No visible sprite was found in cell $region."
	}

	return [Drawing.Rectangle]::new(
		$minimumX,
		$minimumY,
		$maximumX - $minimumX + 1,
		$maximumY - $minimumY + 1)
}

function New-PixelSprite([Drawing.Bitmap]$source, [Drawing.Rectangle]$region)
{
	$bounds = Get-AlphaBounds $source $region
	$bitmap = [Drawing.Bitmap]::new($spriteSize, $spriteSize, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.Clear([Drawing.Color]::Transparent)
	$graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
	$graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighSpeed
	$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
	$graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::None
	$graphics.DrawImage($source, [Drawing.Rectangle]::new(0, 0, $spriteSize, $spriteSize),
		$bounds, [Drawing.GraphicsUnit]::Pixel)
	$graphics.Dispose()

	for ($y = 0; $y -lt $spriteSize; $y++)
	{
		for ($x = 0; $x -lt $spriteSize; $x++)
		{
			$color = $bitmap.GetPixel($x, $y)
			$cleanColor = if ($color.A -lt $alphaThreshold)
			{
				[Drawing.Color]::Transparent
			}
			else
			{
				[Drawing.Color]::FromArgb(255, $color.R, $color.G, $color.B)
			}
			$bitmap.SetPixel($x, $y, $cleanColor)
		}
	}

	return $bitmap
}

function New-CenteredPixelSprite([Drawing.Bitmap]$source)
{
	return New-PixelSprite $source ([Drawing.Rectangle]::new(0, 0, $source.Width, $source.Height))
}

function Save-PixelSprite([Drawing.Bitmap]$sprite, [string]$path)
{
	$sprite.Save($path, [Drawing.Imaging.ImageFormat]::Png)
}

New-Item -ItemType Directory -Path $assetRoot -Force | Out-Null
New-Item -ItemType Directory -Path $uiRoot -Force | Out-Null

$source = [Drawing.Bitmap]::FromFile($spriteSourcePath)
$cellWidth = [Math]::Floor($source.Width / 4)
$cellHeight = [Math]::Floor($source.Height / 2)
$spriteNames = @(
	'ball.png',
	'player.png',
	'tile-1.png',
	'tile-2.png',
	'tile-3.png',
	'tile-4.png',
	'tile-5.png',
	'particle.png'
)
$sprites = [Collections.Generic.List[Drawing.Bitmap]]::new()

for ($index = 0; $index -lt $spriteNames.Count; $index++)
{
	$column = $index % 4
	$row = [Math]::Floor($index / 4)
	$left = $column * $cellWidth
	$top = $row * $cellHeight
	$width = if ($column -eq 3) { $source.Width - $left } else { $cellWidth }
	$height = if ($row -eq 1) { $source.Height - $top } else { $cellHeight }
	$sprite = New-PixelSprite $source ([Drawing.Rectangle]::new($left, $top, $width, $height))
	Save-PixelSprite $sprite (Join-Path $assetRoot $spriteNames[$index])
	$sprites.Add($sprite)
}

$atlas = [Drawing.Bitmap]::new($spriteSize * 4, $spriteSize * 2, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$atlasGraphics = [Drawing.Graphics]::FromImage($atlas)
$atlasGraphics.Clear([Drawing.Color]::Transparent)
$atlasGraphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
$atlasGraphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$atlasGraphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half

for ($index = 0; $index -lt $sprites.Count; $index++)
{
	$atlasGraphics.DrawImageUnscaled($sprites[$index], ($index % 4) * $spriteSize,
		[Math]::Floor($index / 4) * $spriteSize)
	$sprites[$index].Dispose()
}

$atlasGraphics.Dispose()
$atlas.Save($atlasPath, [Drawing.Imaging.ImageFormat]::Png)
$atlas.Dispose()
$source.Dispose()

$buttonSource = [Drawing.Bitmap]::FromFile($buttonSourcePath)
$button = New-CenteredPixelSprite $buttonSource
Save-PixelSprite $button $buttonPath
$button.Dispose()
$buttonSource.Dispose()

$backgroundSource = [Drawing.Bitmap]::FromFile($backgroundSourcePath)
$sourceRatio = $backgroundSource.Width / $backgroundSource.Height
$targetRatio = 16.0 / 9.0

if ($sourceRatio -gt $targetRatio)
{
	$cropHeight = $backgroundSource.Height
	$cropWidth = [Math]::Floor($cropHeight * $targetRatio)
	$cropX = [Math]::Floor(($backgroundSource.Width - $cropWidth) * 0.5)
	$cropY = 0
}
else
{
	$cropWidth = $backgroundSource.Width
	$cropHeight = [Math]::Floor($cropWidth / $targetRatio)
	$cropX = 0
	$cropY = [Math]::Floor(($backgroundSource.Height - $cropHeight) * 0.5)
}

$background = [Drawing.Bitmap]::new(320, 180, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$backgroundGraphics = [Drawing.Graphics]::FromImage($background)
$backgroundGraphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
$backgroundGraphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighSpeed
$backgroundGraphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$backgroundGraphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
$backgroundGraphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::None
$backgroundGraphics.DrawImage($backgroundSource, [Drawing.Rectangle]::new(0, 0, 320, 180),
	[Drawing.Rectangle]::new($cropX, $cropY, $cropWidth, $cropHeight), [Drawing.GraphicsUnit]::Pixel)
$backgroundGraphics.Dispose()
$backgroundSource.Dispose()
$background.Save($backgroundPath, [Drawing.Imaging.ImageFormat]::Png)
$background.Dispose()

Write-Host 'Brickout pixel-art sprites are ready: eight 16x16 sprites, a 64x32 atlas, a 16x16 UI panel and a 320x180 background.'
