$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$assetRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout'))
$uiRoot = [IO.Path]::GetFullPath((Join-Path $assetRoot '..\UI'))
$gameplaySourcePath = Join-Path $assetRoot 'Source\neon-gameplay-atlas.png'
$backgroundSourcePath = Join-Path $assetRoot 'Source\neon-arena-background.png'
$controllerSourcePath = Join-Path $uiRoot 'Source\controller-prompts-atlas.png'

foreach ($sourcePath in @($gameplaySourcePath, $backgroundSourcePath, $controllerSourcePath))
{
	if (!(Test-Path -LiteralPath $sourcePath))
	{
		throw "Missing Brickout art source: $sourcePath"
	}
}

function New-Canvas([int]$width, [int]$height, [Drawing.Color]$color)
{
	$bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.Clear($color)
	$graphics.Dispose()
	return $bitmap
}

function New-QualityGraphics([Drawing.Bitmap]$bitmap)
{
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
	$graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
	return $graphics
}

function Get-VisibleBounds([Drawing.Bitmap]$bitmap, [Drawing.Rectangle]$cell, [int]$alphaThreshold = 96)
{
	$minimumX = $cell.Right
	$minimumY = $cell.Bottom
	$maximumX = -1
	$maximumY = -1

	for ($y = $cell.Top; $y -lt $cell.Bottom; $y++)
	{
		for ($x = $cell.Left; $x -lt $cell.Right; $x++)
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
		return $cell
	}

	$padding = 8
	$left = [Math]::Max($cell.Left, $minimumX - $padding)
	$top = [Math]::Max($cell.Top, $minimumY - $padding)
	$right = [Math]::Min($cell.Right, $maximumX + $padding + 1)
	$bottom = [Math]::Min($cell.Bottom, $maximumY + $padding + 1)
	return [Drawing.Rectangle]::FromLTRB($left, $top, $right, $bottom)
}

function Export-AtlasCell([Drawing.Bitmap]$atlas, [int]$column, [int]$row,
	[int]$columns, [int]$rows, [int]$width, [int]$height, [string]$path)
{
	$cellWidth = [int]($atlas.Width / $columns)
	$cellHeight = [int]($atlas.Height / $rows)
	$cell = [Drawing.Rectangle]::new($column * $cellWidth, $row * $cellHeight, $cellWidth, $cellHeight)
	$source = Get-VisibleBounds $atlas $cell
	$bitmap = New-Canvas $width $height ([Drawing.Color]::Transparent)
	$graphics = New-QualityGraphics $bitmap
	$graphics.DrawImage($atlas, [Drawing.Rectangle]::new(0, 0, $width, $height),
		$source.X, $source.Y, $source.Width, $source.Height, [Drawing.GraphicsUnit]::Pixel)
	$graphics.Dispose()
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
}

function New-CutCornerPath([float]$width, [float]$height, [float]$cut)
{
	$path = [Drawing.Drawing2D.GraphicsPath]::new()
	$points = [Drawing.PointF[]]@(
		[Drawing.PointF]::new($cut, 0),
		[Drawing.PointF]::new($width - $cut, 0),
		[Drawing.PointF]::new($width, $cut),
		[Drawing.PointF]::new($width, $height - $cut),
		[Drawing.PointF]::new($width - $cut, $height),
		[Drawing.PointF]::new($cut, $height),
		[Drawing.PointF]::new(0, $height - $cut),
		[Drawing.PointF]::new(0, $cut)
	)
	$path.AddPolygon($points)
	return $path
}

New-Item -ItemType Directory -Path $assetRoot -Force | Out-Null
New-Item -ItemType Directory -Path $uiRoot -Force | Out-Null

$gameplayAtlas = [Drawing.Bitmap]::FromFile($gameplaySourcePath)
$spriteSpecs = @(
	@('ball.png', 0, 0, 14, 14),
	@('player.png', 1, 0, 100, 20),
	@('tile-1.png', 2, 0, 60, 24),
	@('tile-2.png', 3, 0, 60, 24),
	@('tile-3.png', 0, 1, 60, 24),
	@('tile-4.png', 1, 1, 60, 24),
	@('tile-5.png', 2, 1, 60, 24),
	@('particle.png', 3, 1, 24, 24),
	@('bumper.png', 0, 2, 40, 40),
	@('power-life.png', 1, 2, 18, 18),
	@('power-multiball.png', 2, 2, 18, 18),
	@('rail.png', 3, 2, 120, 12),
	@('wall-horizontal.png', 3, 2, 800, 20)
)

foreach ($spec in $spriteSpecs)
{
	Export-AtlasCell $gameplayAtlas $spec[1] $spec[2] 4 3 $spec[3] $spec[4] (Join-Path $assetRoot $spec[0])
}

$horizontalWall = [Drawing.Bitmap]::FromFile((Join-Path $assetRoot 'wall-horizontal.png'))
$horizontalWall.RotateFlip([Drawing.RotateFlipType]::Rotate90FlipNone)
$verticalWall = New-Canvas 20 600 ([Drawing.Color]::Transparent)
$verticalGraphics = New-QualityGraphics $verticalWall
$verticalGraphics.DrawImage($horizontalWall, 0, 0, 20, 600)
$verticalGraphics.Dispose()
$verticalWall.Save((Join-Path $assetRoot 'wall-vertical.png'), [Drawing.Imaging.ImageFormat]::Png)
$verticalWall.Dispose()
$horizontalWall.Dispose()

$preview = New-Canvas 512 384 ([Drawing.Color]::FromArgb(255, 1, 5, 16))
$previewGraphics = New-QualityGraphics $preview
$previewItems = @($spriteSpecs | Where-Object { $_[0] -ne 'wall-horizontal.png' })
for ($index = 0; $index -lt $previewItems.Count; $index++)
{
	$item = [Drawing.Bitmap]::FromFile((Join-Path $assetRoot $previewItems[$index][0]))
	$cellX = ($index % 4) * 128
	$cellY = [Math]::Floor($index / 4) * 128
	$maximum = 92.0
	$scale = [Math]::Min($maximum / $item.Width, $maximum / $item.Height)
	$drawWidth = [int]($item.Width * $scale)
	$drawHeight = [int]($item.Height * $scale)
	$previewGraphics.DrawImage($item, $cellX + [int]((128 - $drawWidth) / 2),
		$cellY + [int]((128 - $drawHeight) / 2), $drawWidth, $drawHeight)
	$item.Dispose()
}
$previewGraphics.Dispose()
$preview.Save((Join-Path $assetRoot 'brickout-spritesheet.png'), [Drawing.Imaging.ImageFormat]::Png)
$preview.Dispose()
$gameplayAtlas.Dispose()

$backgroundSource = [Drawing.Bitmap]::FromFile($backgroundSourcePath)
$background = New-Canvas 1280 720 ([Drawing.Color]::Black)
$backgroundGraphics = New-QualityGraphics $background
$backgroundGraphics.DrawImage($backgroundSource, 0, 0, 1280, 720)
$backgroundGraphics.Dispose()
$background.Save((Join-Path $assetRoot 'background.png'), [Drawing.Imaging.ImageFormat]::Png)
$background.Dispose()
$backgroundSource.Dispose()

$controllerAtlas = [Drawing.Bitmap]::FromFile($controllerSourcePath)
$controllerNames = @('controller-stick.png', 'controller-dpad.png', 'controller-a.png', 'controller-b.png')
for ($column = 0; $column -lt $controllerNames.Count; $column++)
{
	Export-AtlasCell $controllerAtlas $column 0 4 1 32 32 (Join-Path $uiRoot $controllerNames[$column])
}
$controllerAtlas.Dispose()

$button = New-Canvas 360 54 ([Drawing.Color]::Transparent)
$buttonGraphics = New-QualityGraphics $button
$buttonPath = New-CutCornerPath 359 53 10
$buttonFill = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(238, 3, 9, 22))
$buttonGlow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(70, 255, 255, 255), 7)
$buttonCore = [Drawing.Pen]::new([Drawing.Color]::FromArgb(235, 255, 255, 255), 2)
$buttonDetail = [Drawing.Pen]::new([Drawing.Color]::FromArgb(150, 255, 255, 255), 1)
$buttonGraphics.FillPath($buttonFill, $buttonPath)
$buttonGraphics.DrawPath($buttonGlow, $buttonPath)
$buttonGraphics.DrawPath($buttonCore, $buttonPath)
$buttonGraphics.DrawLine($buttonDetail, 24, 7, 112, 7)
$buttonFill.Dispose()
$buttonGlow.Dispose()
$buttonCore.Dispose()
$buttonDetail.Dispose()
$buttonPath.Dispose()
$buttonGraphics.Dispose()
$button.Save((Join-Path $uiRoot 'button-panel.png'), [Drawing.Imaging.ImageFormat]::Png)
$button.Dispose()

$panel = New-Canvas 256 256 ([Drawing.Color]::Transparent)
$panelGraphics = New-QualityGraphics $panel
$panelPath = New-CutCornerPath 255 255 18
$panelFill = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(242, 1, 6, 19))
$panelGlow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(50, 30, 225, 255), 12)
$panelCore = [Drawing.Pen]::new([Drawing.Color]::FromArgb(220, 70, 235, 255), 2)
$panelGraphics.FillPath($panelFill, $panelPath)
$panelGraphics.DrawPath($panelGlow, $panelPath)
$panelGraphics.DrawPath($panelCore, $panelPath)
$panelFill.Dispose()
$panelGlow.Dispose()
$panelCore.Dispose()
$panelPath.Dispose()
$panelGraphics.Dispose()
$panel.Save((Join-Path $uiRoot 'neon-panel.png'), [Drawing.Imaging.ImageFormat]::Png)
$panel.Dispose()

$topBar = New-Canvas 1280 72 ([Drawing.Color]::FromArgb(245, 1, 5, 16))
$topBarGraphics = New-QualityGraphics $topBar
$topBarGlow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(85, 30, 225, 255), 10)
$topBarCore = [Drawing.Pen]::new([Drawing.Color]::FromArgb(240, 75, 235, 255), 2)
$topBarGraphics.DrawLine($topBarGlow, 0, 68, 1280, 68)
$topBarGraphics.DrawLine($topBarCore, 0, 70, 1280, 70)
$topBarGlow.Dispose()
$topBarCore.Dispose()
$topBarGraphics.Dispose()
$topBar.Save((Join-Path $uiRoot 'top-bar.png'), [Drawing.Imaging.ImageFormat]::Png)
$topBar.Dispose()

Write-Host 'Brickout neon art is ready: generated source art normalized for native-size rendering.'
