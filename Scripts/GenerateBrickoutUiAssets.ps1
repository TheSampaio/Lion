param(
	[string]$AssetRoot = (Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$brickout = Join-Path $AssetRoot 'Brickout'
$ui = Join-Path $AssetRoot 'UI'
New-Item -ItemType Directory -Force -Path $brickout, $ui | Out-Null

function Get-AlphaBounds([Drawing.Bitmap]$bitmap)
{
	$rectangle = [Drawing.Rectangle]::new(0, 0, $bitmap.Width, $bitmap.Height)
	$data = $bitmap.LockBits($rectangle, [Drawing.Imaging.ImageLockMode]::ReadOnly,
		[Drawing.Imaging.PixelFormat]::Format32bppArgb)
	try
	{
		$length = [Math]::Abs($data.Stride) * $bitmap.Height
		$bytes = [byte[]]::new($length)
		[Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $length)
		$left = $bitmap.Width
		$top = $bitmap.Height
		$right = -1
		$bottom = -1
		for ($y = 0; $y -lt $bitmap.Height; $y++)
		{
			$row = $y * [Math]::Abs($data.Stride)
			for ($x = 0; $x -lt $bitmap.Width; $x++)
			{
				if ($bytes[$row + $x * 4 + 3] -le 8) { continue }
				$left = [Math]::Min($left, $x)
				$top = [Math]::Min($top, $y)
				$right = [Math]::Max($right, $x)
				$bottom = [Math]::Max($bottom, $y)
			}
		}
	}
	finally
	{
		$bitmap.UnlockBits($data)
	}

	if ($right -lt $left) { return $rectangle }
	$padding = 4
	$left = [Math]::Max(0, $left - $padding)
	$top = [Math]::Max(0, $top - $padding)
	$right = [Math]::Min($bitmap.Width - 1, $right + $padding)
	$bottom = [Math]::Min($bitmap.Height - 1, $bottom + $padding)
	return [Drawing.Rectangle]::new($left, $top, $right - $left + 1, $bottom - $top + 1)
}

function Export-Resized([string]$sourcePath, [string]$targetPath, [int]$width, [int]$height)
{
	$source = [Drawing.Bitmap]::FromFile($sourcePath)
	try
	{
		$bounds = Get-AlphaBounds $source
		$target = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
		$graphics = [Drawing.Graphics]::FromImage($target)
		try
		{
			$graphics.Clear([Drawing.Color]::Transparent)
			$graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
			$graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
			$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
			$graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
			$graphics.DrawImage($source, [Drawing.Rectangle]::new(0, 0, $width, $height),
				$bounds.X, $bounds.Y, $bounds.Width, $bounds.Height, [Drawing.GraphicsUnit]::Pixel)
		}
		finally { $graphics.Dispose() }
		$target.Save($targetPath, [Drawing.Imaging.ImageFormat]::Png)
		$target.Dispose()
	}
	finally { $source.Dispose() }
}

function New-RoundedPath([Drawing.RectangleF]$rectangle, [float]$radius)
{
	$diameter = $radius * 2
	$path = [Drawing.Drawing2D.GraphicsPath]::new()
	$path.AddArc($rectangle.X, $rectangle.Y, $diameter, $diameter, 180, 90)
	$path.AddArc($rectangle.Right - $diameter, $rectangle.Y, $diameter, $diameter, 270, 90)
	$path.AddArc($rectangle.Right - $diameter, $rectangle.Bottom - $diameter, $diameter, $diameter, 0, 90)
	$path.AddArc($rectangle.X, $rectangle.Bottom - $diameter, $diameter, $diameter, 90, 90)
	$path.CloseFigure()
	return $path
}

function New-Keycap([string]$path, [int]$width, [string]$label, [float]$fontSize = 12)
{
	$height = 32
	$bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
	$shape = New-RoundedPath ([Drawing.RectangleF]::new(1.5, 1.5, $width - 3, $height - 3)) 5
	$fill = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(235, 5, 18, 34))
	$edge = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 42, 224, 255), 2)
	$font = [Drawing.Font]::new('Consolas', $fontSize, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
	$brush = [Drawing.SolidBrush]::new([Drawing.Color]::White)
	$format = [Drawing.StringFormat]::new()
	$format.Alignment = [Drawing.StringAlignment]::Center
	$format.LineAlignment = [Drawing.StringAlignment]::Center
	$graphics.FillPath($fill, $shape)
	$graphics.DrawPath($edge, $shape)
	$graphics.DrawString($label, $font, $brush, [Drawing.RectangleF]::new(0, -1, $width, $height), $format)
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$format.Dispose(); $brush.Dispose(); $font.Dispose(); $edge.Dispose(); $fill.Dispose(); $shape.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
}

function New-ArrowKeys([string]$path)
{
	$bitmap = [Drawing.Bitmap]::new(68, 32, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
	$fill = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(235, 5, 18, 34))
	$edge = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 42, 224, 255), 1.5)
	$arrow = [Drawing.Pen]::new([Drawing.Color]::White, 2)
	$arrow.StartCap = [Drawing.Drawing2D.LineCap]::Round
	$arrow.EndCap = [Drawing.Drawing2D.LineCap]::Round
	$positions = @(1, 18, 35, 52)
	foreach ($x in $positions)
	{
		$shape = New-RoundedPath ([Drawing.RectangleF]::new($x, 8, 15, 16)) 3
		$graphics.FillPath($fill, $shape)
		$graphics.DrawPath($edge, $shape)
		$shape.Dispose()
	}
	$graphics.DrawLine($arrow, 5, 16, 12, 16); $graphics.DrawLine($arrow, 5, 16, 8, 13); $graphics.DrawLine($arrow, 5, 16, 8, 19)
	$graphics.DrawLine($arrow, 25, 20, 25, 12); $graphics.DrawLine($arrow, 25, 12, 22, 15); $graphics.DrawLine($arrow, 25, 12, 28, 15)
	$graphics.DrawLine($arrow, 42, 12, 42, 20); $graphics.DrawLine($arrow, 42, 20, 39, 17); $graphics.DrawLine($arrow, 42, 20, 45, 17)
	$graphics.DrawLine($arrow, 56, 16, 63, 16); $graphics.DrawLine($arrow, 63, 16, 60, 13); $graphics.DrawLine($arrow, 63, 16, 60, 19)
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$arrow.Dispose(); $edge.Dispose(); $fill.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
}

function New-ControllerButton([string]$path, [string]$label)
{
	$bitmap = [Drawing.Bitmap]::new(32, 32, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
	$glow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(110, 20, 170, 255), 4)
	$fill = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255, 5, 42, 88))
	$edge = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 55, 225, 255), 2)
	$inner = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 155, 245, 255), 1)
	$font = [Drawing.Font]::new('Arial', 16, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
	$brush = [Drawing.SolidBrush]::new([Drawing.Color]::White)
	$format = [Drawing.StringFormat]::new()
	$format.Alignment = [Drawing.StringAlignment]::Center
	$format.LineAlignment = [Drawing.StringAlignment]::Center
	$graphics.DrawEllipse($glow, 3, 3, 26, 26)
	$graphics.FillEllipse($fill, 3, 3, 26, 26)
	$graphics.DrawEllipse($edge, 3, 3, 26, 26)
	$graphics.DrawEllipse($inner, 6, 6, 20, 20)
	$graphics.DrawString($label, $font, $brush, [Drawing.RectangleF]::new(0, -1, 32, 32), $format)
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$format.Dispose(); $brush.Dispose(); $font.Dispose(); $inner.Dispose(); $edge.Dispose(); $fill.Dispose(); $glow.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
}

function New-WidgetAssets
{
	$frame = [Drawing.Bitmap]::new(32, 32, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($frame)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.Clear([Drawing.Color]::Transparent)
	$fill = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(245, 3, 12, 29))
	$edge = [Drawing.Pen]::new([Drawing.Color]::White, 2)
	$graphics.FillRectangle($fill, 4, 4, 24, 24)
	$graphics.DrawRectangle($edge, 4, 4, 23, 23)
	$frame.Save((Join-Path $ui 'checkbox-frame.png'), [Drawing.Imaging.ImageFormat]::Png)
	$edge.Dispose(); $fill.Dispose(); $graphics.Dispose(); $frame.Dispose()

	$check = [Drawing.Bitmap]::new(32, 32, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($check)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.Clear([Drawing.Color]::Transparent)
	$glow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(105, 30, 225, 255), 6)
	$mark = [Drawing.Pen]::new([Drawing.Color]::White, 3)
	$mark.StartCap = [Drawing.Drawing2D.LineCap]::Round; $mark.EndCap = [Drawing.Drawing2D.LineCap]::Round
	$graphics.DrawLines($glow, [Drawing.Point[]]@([Drawing.Point]::new(8, 16), [Drawing.Point]::new(14, 22), [Drawing.Point]::new(25, 9)))
	$graphics.DrawLines($mark, [Drawing.Point[]]@([Drawing.Point]::new(8, 16), [Drawing.Point]::new(14, 22), [Drawing.Point]::new(25, 9)))
	$check.Save((Join-Path $ui 'checkbox-check.png'), [Drawing.Imaging.ImageFormat]::Png)
	$mark.Dispose(); $glow.Dispose(); $graphics.Dispose(); $check.Dispose()

	foreach ($name in @('progress-track.png', 'progress-fill.png'))
	{
		$bitmap = [Drawing.Bitmap]::new(16, 16, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
		$graphics = [Drawing.Graphics]::FromImage($bitmap)
		$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
		$graphics.Clear([Drawing.Color]::Transparent)
		$shape = New-RoundedPath ([Drawing.RectangleF]::new(1, 3, 14, 10)) 4
		$color = if ($name -eq 'progress-track.png') { [Drawing.Color]::FromArgb(255, 20, 55, 75) } else { [Drawing.Color]::White }
		$brush = [Drawing.SolidBrush]::new($color)
		$graphics.FillPath($brush, $shape)
		$bitmap.Save((Join-Path $ui $name), [Drawing.Imaging.ImageFormat]::Png)
		$brush.Dispose(); $shape.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
	}
}

function New-Title([string]$path)
{
	$bitmap = [Drawing.Bitmap]::new(560, 96, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$graphics.Clear([Drawing.Color]::Transparent)
	$family = [Drawing.FontFamily]::new('Consolas')
	$shape = [Drawing.Drawing2D.GraphicsPath]::new()
	$format = [Drawing.StringFormat]::new()
	$format.Alignment = [Drawing.StringAlignment]::Center
	$format.LineAlignment = [Drawing.StringAlignment]::Center
	$shape.AddString('BRICKOUT', $family, [int][Drawing.FontStyle]::Bold, 70,
		[Drawing.RectangleF]::new(0, -2, 560, 96), $format)
	$cyanGlow = [Drawing.Pen]::new([Drawing.Color]::FromArgb(90, 0, 225, 255), 9)
	$darkEdge = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 3, 12, 29), 5)
	$white = [Drawing.SolidBrush]::new([Drawing.Color]::White)
	$graphics.DrawPath($cyanGlow, $shape)
	$graphics.DrawPath($darkEdge, $shape)
	$graphics.FillPath($white, $shape)
	$accent = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 255, 150, 45), 2)
	$graphics.DrawLine($accent, 90, 85, 470, 85)
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$accent.Dispose(); $white.Dispose(); $darkEdge.Dispose(); $cyanGlow.Dispose(); $format.Dispose(); $shape.Dispose(); $family.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
}

function New-PowerIcon([string]$path, [string]$kind)
{
	$size = 32
	$bitmap = [Drawing.Bitmap]::new($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$cyan = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 55, 235, 255), 2.5)
	$pink = [Drawing.Pen]::new([Drawing.Color]::FromArgb(255, 255, 45, 178), 2.5)
	$white = [Drawing.Pen]::new([Drawing.Color]::White, 2)
	$dark = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(230, 4, 12, 30))
	if ($kind -eq 'Bomb')
	{
		$graphics.FillEllipse($dark, 6, 8, 20, 20)
		$graphics.DrawEllipse($pink, 6, 8, 20, 20)
		$graphics.DrawArc($cyan, 15, 2, 12, 12, 185, 100)
		$graphics.DrawLine($white, 25, 3, 28, 0)
		$graphics.DrawLine($white, 27, 5, 31, 5)
	}
	elseif ($kind -eq 'Wide')
	{
		$graphics.FillRectangle($dark, 5, 12, 22, 8)
		$graphics.DrawRectangle($cyan, 5, 12, 22, 8)
		$graphics.DrawLine($pink, 2, 16, 8, 16); $graphics.DrawLine($pink, 2, 16, 5, 13); $graphics.DrawLine($pink, 2, 16, 5, 19)
		$graphics.DrawLine($pink, 30, 16, 24, 16); $graphics.DrawLine($pink, 30, 16, 27, 13); $graphics.DrawLine($pink, 30, 16, 27, 19)
	}
	else
	{
		$graphics.FillRectangle($dark, 4, 8, 22, 7); $graphics.DrawRectangle($cyan, 4, 8, 22, 7)
		$graphics.FillRectangle($dark, 7, 18, 22, 7); $graphics.DrawRectangle($pink, 7, 18, 22, 7)
	}
	$bitmap.Save($path, [Drawing.Imaging.ImageFormat]::Png)
	$dark.Dispose(); $white.Dispose(); $pink.Dispose(); $cyan.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
}

$source = Join-Path $brickout 'Source'
Export-Resized (Join-Path $source 'collider-wall-horizontal-source.png') (Join-Path $brickout 'wall-horizontal-v2.png') 800 40
Export-Resized (Join-Path $source 'collider-wall-vertical-source.png') (Join-Path $brickout 'wall-vertical-v2.png') 48 600
Export-Resized (Join-Path $source 'collider-wall-horizontal-source.png') (Join-Path $brickout 'pinball-rail.png') 120 20

New-Keycap (Join-Path $ui 'key-enter.png') 52 'ENTER' 10
New-Keycap (Join-Path $ui 'key-esc.png') 44 'ESC' 11
New-Keycap (Join-Path $ui 'key-space.png') 64 'SPACE' 10
New-Keycap (Join-Path $ui 'key-e.png') 32 'E' 14
New-ArrowKeys (Join-Path $ui 'key-arrows.png')
New-ControllerButton (Join-Path $ui 'controller-x.png') 'X'
New-Keycap (Join-Path $ui 'controller-start.png') 54 'START' 10
New-WidgetAssets
New-Title (Join-Path $ui 'brickout-title.png')

New-PowerIcon (Join-Path $brickout 'power-bomb.png') 'Bomb'
New-PowerIcon (Join-Path $brickout 'power-wide.png') 'Wide'
New-PowerIcon (Join-Path $brickout 'power-duplicate.png') 'Duplicate'

Write-Host 'Generated Brickout collider, prompt and power sprites.'
