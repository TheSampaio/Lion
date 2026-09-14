$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$assetRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets\Sprites\Brickout'))
$uiRoot = [IO.Path]::GetFullPath((Join-Path $assetRoot '..\UI'))
$spriteSize = 32

function New-Canvas([int]$width, [int]$height, [Drawing.Color]$color)
{
	$bitmap = [Drawing.Bitmap]::new($width, $height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [Drawing.Graphics]::FromImage($bitmap)
	$graphics.Clear($color)
	$graphics.Dispose()
	return $bitmap
}

function Set-Pixel([Drawing.Bitmap]$bitmap, [int]$x, [int]$y, [Drawing.Color]$color)
{
	if ($x -ge 0 -and $x -lt $bitmap.Width -and $y -ge 0 -and $y -lt $bitmap.Height)
	{
		$bitmap.SetPixel($x, $y, $color)
	}
}

function Draw-Rectangle([Drawing.Bitmap]$bitmap, [Drawing.Color]$color,
	[int]$x, [int]$y, [int]$width, [int]$height, [bool]$filled = $false)
{
	for ($row = $y; $row -lt $y + $height; $row++)
	{
		for ($column = $x; $column -lt $x + $width; $column++)
		{
			if ($filled -or $row -eq $y -or $row -eq $y + $height - 1 -or $column -eq $x -or $column -eq $x + $width - 1)
			{
				Set-Pixel $bitmap $column $row $color
			}
		}
	}
}

function Save-Sprite([Drawing.Bitmap]$bitmap, [string]$name)
{
	$bitmap.Save((Join-Path $assetRoot $name), [Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
}

function New-NeonTile([Drawing.Color]$color)
{
	$bitmap = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
	Draw-Rectangle $bitmap ([Drawing.Color]::FromArgb(255, 2, 8, 18)) 0 5 32 22 $true
	Draw-Rectangle $bitmap $color 0 5 32 22
	Draw-Rectangle $bitmap ([Drawing.Color]::FromArgb(255, $color.R / 2, $color.G / 2, $color.B / 2)) 2 7 28 18
	Draw-Rectangle $bitmap ([Drawing.Color]::White) 3 7 9 2 $true
	Draw-Rectangle $bitmap $color 6 11 20 1 $true
	Set-Pixel $bitmap 28 24 ([Drawing.Color]::White)
	Set-Pixel $bitmap 29 23 ([Drawing.Color]::White)
	return $bitmap
}

New-Item -ItemType Directory -Path $assetRoot -Force | Out-Null
New-Item -ItemType Directory -Path $uiRoot -Force | Out-Null

$ball = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
for ($y = 4; $y -le 27; $y++)
{
	for ($x = 4; $x -le 27; $x++)
	{
		$dx = $x - 15.5
		$dy = $y - 15.5
		$distanceSquared = $dx * $dx + $dy * $dy
		if ($distanceSquared -le 118)
		{
			Set-Pixel $ball $x $y ([Drawing.Color]::White)
		}
	}
}
Draw-Rectangle $ball ([Drawing.Color]::White) 10 8 6 2 $true
Save-Sprite $ball 'ball.png'

$player = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
Draw-Rectangle $player ([Drawing.Color]::White) 2 10 28 12 $true
Draw-Rectangle $player ([Drawing.Color]::FromArgb(255, 220, 220, 225)) 4 12 24 3 $true
Draw-Rectangle $player ([Drawing.Color]::FromArgb(255, 20, 55, 75)) 7 16 18 3 $true
Draw-Rectangle $player ([Drawing.Color]::White) 0 13 3 6 $true
Draw-Rectangle $player ([Drawing.Color]::White) 29 13 3 6 $true
Save-Sprite $player 'player.png'

$durabilityColors = @(
	[Drawing.Color]::FromArgb(255, 255, 45, 70),
	[Drawing.Color]::FromArgb(255, 255, 224, 55),
	[Drawing.Color]::FromArgb(255, 60, 255, 120),
	[Drawing.Color]::FromArgb(255, 40, 225, 255),
	[Drawing.Color]::FromArgb(255, 225, 65, 255)
)

$sprites = [Collections.Generic.List[Drawing.Bitmap]]::new()
$sprites.Add((New-NeonTile $durabilityColors[0]))
$sprites.Add((New-NeonTile $durabilityColors[1]))
$sprites.Add((New-NeonTile $durabilityColors[2]))
$sprites.Add((New-NeonTile $durabilityColors[3]))
$sprites.Add((New-NeonTile $durabilityColors[4]))

for ($index = 0; $index -lt $sprites.Count; $index++)
{
	$sprites[$index].Save((Join-Path $assetRoot ("tile-{0}.png" -f ($index + 1))),
		[Drawing.Imaging.ImageFormat]::Png)
}

$particle = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
for ($y = 8; $y -le 23; $y++)
{
	$halfWidth = 8 - [Math]::Abs(15.5 - $y)
	for ($x = [Math]::Ceiling(15.5 - $halfWidth); $x -le [Math]::Floor(15.5 + $halfWidth); $x++)
	{
		Set-Pixel $particle $x $y ([Drawing.Color]::White)
	}
}
Save-Sprite $particle 'particle.png'

$bumper = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
for ($y = 1; $y -le 30; $y++)
{
	for ($x = 1; $x -le 30; $x++)
	{
		$distance = [Math]::Sqrt(($x - 15.5) * ($x - 15.5) + ($y - 15.5) * ($y - 15.5))
		if (($distance -ge 12.0 -and $distance -le 14.0) -or ($distance -ge 5.0 -and $distance -le 7.0))
		{
			Set-Pixel $bumper $x $y ([Drawing.Color]::FromArgb(255, 40, 225, 255))
		}
	}
}
Draw-Rectangle $bumper ([Drawing.Color]::White) 14 1 4 3 $true
Draw-Rectangle $bumper ([Drawing.Color]::White) 14 28 4 3 $true
Save-Sprite $bumper 'bumper.png'

$life = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
$heartRows = @('  ## ##  ', ' ####### ', '#########', '#########', ' ####### ', '  #####  ', '   ###   ', '    #    ')
for ($y = 0; $y -lt $heartRows.Count; $y++)
{
	for ($x = 0; $x -lt $heartRows[$y].Length; $x++)
	{
		if ($heartRows[$y][$x] -eq '#')
		{
			Draw-Rectangle $life ([Drawing.Color]::White) ($x * 2 + 6) ($y * 2 + 8) 2 2 $true
		}
	}
}
Save-Sprite $life 'power-life.png'

$multiball = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
foreach ($center in @(@(10,16), @(21,10), @(21,22)))
{
	Draw-Rectangle $multiball ([Drawing.Color]::White) ($center[0] - 3) ($center[1] - 3) 7 7 $true
	Draw-Rectangle $multiball ([Drawing.Color]::FromArgb(255, 150, 235, 255)) ($center[0] - 1) ($center[1] - 1) 3 3 $true
}
Save-Sprite $multiball 'power-multiball.png'

$rail = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
Draw-Rectangle $rail ([Drawing.Color]::FromArgb(255, 2, 8, 18)) 0 10 32 12 $true
Draw-Rectangle $rail ([Drawing.Color]::FromArgb(255, 40, 225, 255)) 0 10 32 12
Draw-Rectangle $rail ([Drawing.Color]::FromArgb(255, 15, 85, 110)) 2 12 28 8
Draw-Rectangle $rail ([Drawing.Color]::White) 4 12 8 2 $true
Save-Sprite $rail 'rail.png'

$wallHorizontal = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
Draw-Rectangle $wallHorizontal ([Drawing.Color]::FromArgb(255, 2, 8, 18)) 0 12 32 8 $true
Draw-Rectangle $wallHorizontal ([Drawing.Color]::FromArgb(255, 40, 225, 255)) 0 12 32 8
Draw-Rectangle $wallHorizontal ([Drawing.Color]::White) 5 14 10 1 $true
Save-Sprite $wallHorizontal 'wall-horizontal.png'

$wallVertical = New-Canvas $spriteSize $spriteSize ([Drawing.Color]::Transparent)
Draw-Rectangle $wallVertical ([Drawing.Color]::FromArgb(255, 2, 8, 18)) 12 0 8 32 $true
Draw-Rectangle $wallVertical ([Drawing.Color]::FromArgb(255, 40, 225, 255)) 12 0 8 32
Draw-Rectangle $wallVertical ([Drawing.Color]::White) 14 5 1 10 $true
Save-Sprite $wallVertical 'wall-vertical.png'

$atlas = New-Canvas ($spriteSize * 4) ($spriteSize * 3) ([Drawing.Color]::Transparent)
$atlasGraphics = [Drawing.Graphics]::FromImage($atlas)
$atlasItems = @(
	(Join-Path $assetRoot 'ball.png'), (Join-Path $assetRoot 'player.png'),
	(Join-Path $assetRoot 'tile-1.png'), (Join-Path $assetRoot 'tile-2.png'),
	(Join-Path $assetRoot 'tile-3.png'), (Join-Path $assetRoot 'tile-4.png'),
	(Join-Path $assetRoot 'tile-5.png'), (Join-Path $assetRoot 'particle.png'),
	(Join-Path $assetRoot 'bumper.png'), (Join-Path $assetRoot 'power-life.png'),
	(Join-Path $assetRoot 'power-multiball.png'), (Join-Path $assetRoot 'rail.png')
)
for ($index = 0; $index -lt $atlasItems.Count; $index++)
{
	$item = [Drawing.Bitmap]::FromFile($atlasItems[$index])
	$atlasGraphics.DrawImageUnscaled($item, ($index % 4) * $spriteSize, [Math]::Floor($index / 4) * $spriteSize)
	$item.Dispose()
}
$atlasGraphics.Dispose()
$atlas.Save((Join-Path $assetRoot 'brickout-spritesheet.png'), [Drawing.Imaging.ImageFormat]::Png)
$atlas.Dispose()
foreach ($sprite in $sprites) { $sprite.Dispose() }

$button = New-Canvas 64 32 ([Drawing.Color]::FromArgb(255, 3, 9, 19))
Draw-Rectangle $button ([Drawing.Color]::White) 0 0 64 32
Draw-Rectangle $button ([Drawing.Color]::FromArgb(255, 115, 150, 170)) 1 1 62 30
$button.Save((Join-Path $uiRoot 'button-panel.png'), [Drawing.Imaging.ImageFormat]::Png)
$button.Dispose()

$panel = New-Canvas 64 64 ([Drawing.Color]::FromArgb(245, 2, 8, 18))
Draw-Rectangle $panel ([Drawing.Color]::FromArgb(255, 40, 225, 255)) 0 0 64 64
$panel.Save((Join-Path $uiRoot 'neon-panel.png'), [Drawing.Imaging.ImageFormat]::Png)
$panel.Dispose()

$topBar = New-Canvas 16 16 ([Drawing.Color]::FromArgb(255, 1, 3, 8))
Draw-Rectangle $topBar ([Drawing.Color]::FromArgb(255, 40, 225, 255)) 0 14 16 2 $true
$topBar.Save((Join-Path $uiRoot 'top-bar.png'), [Drawing.Imaging.ImageFormat]::Png)
$topBar.Dispose()

$background = New-Canvas 320 180 ([Drawing.Color]::FromArgb(255, 2, 8, 24))
for ($x = 0; $x -lt 320; $x += 4)
{
	$color = if (($x % 20) -eq 0) { [Drawing.Color]::FromArgb(255, 9, 50, 88) }
		else { [Drawing.Color]::FromArgb(255, 4, 19, 39) }
	for ($y = 0; $y -lt 180; $y++) { Set-Pixel $background $x $y $color }
}
for ($y = 0; $y -lt 180; $y += 4)
{
	$color = if (($y % 20) -eq 0) { [Drawing.Color]::FromArgb(255, 9, 50, 88) }
		else { [Drawing.Color]::FromArgb(255, 4, 19, 39) }
	for ($x = 0; $x -lt 320; $x++) { Set-Pixel $background $x $y $color }
}
for ($x = 10; $x -lt 320; $x += 20)
{
	for ($y = 10; $y -lt 180; $y += 20)
	{
		Set-Pixel $background $x $y ([Drawing.Color]::FromArgb(255, 30, 100, 145))
	}
}
$background.Save((Join-Path $assetRoot 'background.png'), [Drawing.Imaging.ImageFormat]::Png)
$background.Dispose()

Write-Host 'Brickout neon blueprint art is ready: original 32x32 gameplay sprites, UI panels and a 320x180 arena.'
