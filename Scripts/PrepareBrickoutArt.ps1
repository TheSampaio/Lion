$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$assetRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout'))
$sourcePath = Join-Path $PSScriptRoot 'GeneratedArt\brickout-source.png'
$atlasPath = Join-Path $assetRoot 'brickout-spritesheet.png'
$particlePath = Join-Path $assetRoot 'particle.png'
$buttonSourcePath = Join-Path $PSScriptRoot 'GeneratedArt\button-panel-source.png'
$buttonOutputPath = [IO.Path]::GetFullPath((Join-Path $assetRoot '..\UI\button-panel.png'))
$source = [Drawing.Bitmap]::FromFile($sourcePath)

function New-RoundedPath([Drawing.RectangleF]$bounds, [float]$radius)
{
	$diameter = $radius * 2
	$path = [Drawing.Drawing2D.GraphicsPath]::new()
	$path.AddArc($bounds.Left, $bounds.Top, $diameter, $diameter, 180, 90)
	$path.AddArc($bounds.Right - $diameter, $bounds.Top, $diameter, $diameter, 270, 90)
	$path.AddArc($bounds.Right - $diameter, $bounds.Bottom - $diameter, $diameter, $diameter, 0, 90)
	$path.AddArc($bounds.Left, $bounds.Bottom - $diameter, $diameter, $diameter, 90, 90)
	$path.CloseFigure()
	return $path
}

function New-ExtractedSprite([Drawing.Rectangle]$crop, [int]$width, [int]$height, [string]$shape)
{
	$bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.Clear([Drawing.Color]::Transparent)
	$graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
	$graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
	$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::HighQuality
	$bounds = [Drawing.RectangleF]::new(0, 0, $width, $height)

	if ($shape -eq 'Circle')
	{
		$path = [Drawing.Drawing2D.GraphicsPath]::new()
		$path.AddEllipse($bounds)
	}
	elseif ($shape -eq 'Capsule')
	{
		$path = New-RoundedPath $bounds ($height * 0.5)
	}
	else
	{
		$path = New-RoundedPath $bounds ([Math]::Min($width, $height) * 0.18)
	}

	$graphics.SetClip($path)
	$graphics.DrawImage($source, $bounds, $crop, [Drawing.GraphicsUnit]::Pixel)
	$graphics.Dispose()
	$path.Dispose()
	return $bitmap
}

function Save-Sprite([Drawing.Rectangle]$crop, [int]$width, [int]$height, [string]$shape, [string]$name)
{
	$bitmap = New-ExtractedSprite $crop $width $height $shape
	$bitmap.Save((Join-Path $assetRoot $name), [Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
}

Save-Sprite ([Drawing.Rectangle]::new(81, 56, 205, 205)) 12 12 'Circle' 'ball.png'
Save-Sprite ([Drawing.Rectangle]::new(353, 92, 605, 136)) 100 20 'Capsule' 'player.png'

$brickCrops = @(
	[Drawing.Rectangle]::new(460, 277, 618, 108),
	[Drawing.Rectangle]::new(460, 411, 618, 108),
	[Drawing.Rectangle]::new(460, 545, 618, 116),
	[Drawing.Rectangle]::new(460, 686, 618, 119),
	[Drawing.Rectangle]::new(460, 829, 618, 119)
)

for ($index = 0; $index -lt $brickCrops.Count; $index++)
{
	Save-Sprite $brickCrops[$index] 60 24 'Rounded' ("tile-{0}.png" -f ($index + 1))
}

# Rebuild the deliverable atlas from isolated sprites so it has real alpha rather than a preview grid.
$atlas = [Drawing.Bitmap]::new(1536, 1024, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$atlasGraphics = [Drawing.Graphics]::FromImage($atlas)
$atlasGraphics.Clear([Drawing.Color]::Transparent)
$atlasGraphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
$atlasGraphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
$atlasGraphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$atlasGraphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::HighQuality

$ball = New-ExtractedSprite ([Drawing.Rectangle]::new(81, 56, 205, 205)) 220 220 'Circle'
$paddle = New-ExtractedSprite ([Drawing.Rectangle]::new(353, 92, 605, 136)) 610 145 'Capsule'
$atlasGraphics.DrawImage($ball, 55, 45)
$atlasGraphics.DrawImage($paddle, 340, 85)
$ball.Dispose()
$paddle.Dispose()

for ($index = 0; $index -lt $brickCrops.Count; $index++)
{
	$brick = New-ExtractedSprite $brickCrops[$index] 618 116 'Rounded'
	$atlasGraphics.DrawImage($brick, 455, (275 + $index * 140))
	$brick.Dispose()
}

$particle = [Drawing.Bitmap]::FromFile($particlePath)
$atlasGraphics.DrawImage($particle, [Drawing.Rectangle]::new(1190, 55, 230, 230))
$particle.Dispose()
$atlasGraphics.Dispose()
$source.Dispose()
$atlas.Save($atlasPath, [Drawing.Imaging.ImageFormat]::Png)
$atlas.Dispose()

if (Test-Path $buttonSourcePath)
{
	$buttonSource = [Drawing.Bitmap]::FromFile($buttonSourcePath)
	$button = [Drawing.Bitmap]::new(1750, 365, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$buttonGraphics = [Drawing.Graphics]::FromImage($button)
	$buttonGraphics.Clear([Drawing.Color]::Transparent)
	$buttonGraphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
	$buttonGraphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
	$buttonGraphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$buttonGraphics.DrawImage($buttonSource, [Drawing.Rectangle]::new(0, 0, 1750, 365),
		[Drawing.Rectangle]::new(115, 214, 1750, 365), [Drawing.GraphicsUnit]::Pixel)
	$buttonGraphics.Dispose()
	$buttonSource.Dispose()
	$button.Save($buttonOutputPath, [Drawing.Imaging.ImageFormat]::Png)
	$button.Dispose()
}

Write-Host 'Brickout sprites and transparent atlas are ready.'
