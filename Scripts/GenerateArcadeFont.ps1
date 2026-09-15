param(
	[string]$OutputDirectory = (Join-Path $PSScriptRoot '..\Sandbox\Assets\Fonts')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$latin = -join (@(0x00C1,0x00C0,0x00C2,0x00C3,0x00C4,0x00C9,0x00C8,0x00CA,0x00CB,
	0x00CD,0x00CC,0x00CE,0x00CF,0x00D3,0x00D2,0x00D4,0x00D5,0x00D6,0x00DA,0x00D9,
	0x00DB,0x00DC,0x00C7,0x00D1,0x0178) | ForEach-Object { [char]$_ })
$cyrillic = [string][char]0x0401 + (-join (0x0410..0x042F | ForEach-Object { [char]$_ }))
$greek = -join (@(0x0391..0x03A1; 0x03A3..0x03A9) | ForEach-Object { [char]$_ })
$characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:!?-+.</>'* " + $latin + $cyrillic + $greek
$patterns = @{
	'A'='01110','10001','10001','11111','10001','10001','10001'
	'B'='11110','10001','10001','11110','10001','10001','11110'
	'C'='01111','10000','10000','10000','10000','10000','01111'
	'D'='11110','10001','10001','10001','10001','10001','11110'
	'E'='11111','10000','10000','11110','10000','10000','11111'
	'F'='11111','10000','10000','11110','10000','10000','10000'
	'G'='01111','10000','10000','10111','10001','10001','01111'
	'H'='10001','10001','10001','11111','10001','10001','10001'
	'I'='11111','00100','00100','00100','00100','00100','11111'
	'J'='00111','00010','00010','00010','10010','10010','01100'
	'K'='10001','10010','10100','11000','10100','10010','10001'
	'L'='10000','10000','10000','10000','10000','10000','11111'
	'M'='10001','11011','10101','10101','10001','10001','10001'
	'N'='10001','11001','10101','10011','10001','10001','10001'
	'O'='01110','10001','10001','10001','10001','10001','01110'
	'P'='11110','10001','10001','11110','10000','10000','10000'
	'Q'='01110','10001','10001','10001','10101','10010','01101'
	'R'='11110','10001','10001','11110','10100','10010','10001'
	'S'='01111','10000','10000','01110','00001','00001','11110'
	'T'='11111','00100','00100','00100','00100','00100','00100'
	'U'='10001','10001','10001','10001','10001','10001','01110'
	'V'='10001','10001','10001','10001','10001','01010','00100'
	'W'='10001','10001','10001','10101','10101','11011','10001'
	'X'='10001','10001','01010','00100','01010','10001','10001'
	'Y'='10001','10001','01010','00100','00100','00100','00100'
	'Z'='11111','00001','00010','00100','01000','10000','11111'
	'0'='01110','10001','10011','10101','11001','10001','01110'
	'1'='00100','01100','00100','00100','00100','00100','01110'
	'2'='01110','10001','00001','00010','00100','01000','11111'
	'3'='11110','00001','00001','01110','00001','00001','11110'
	'4'='00010','00110','01010','10010','11111','00010','00010'
	'5'='11111','10000','10000','11110','00001','00001','11110'
	'6'='01110','10000','10000','11110','10001','10001','01110'
	'7'='11111','00001','00010','00100','01000','01000','01000'
	'8'='01110','10001','10001','01110','10001','10001','01110'
	'9'='01110','10001','10001','01111','00001','00001','01110'
	':'='00000','00100','00100','00000','00100','00100','00000'
	'!'='00100','00100','00100','00100','00100','00000','00100'
	'?'='01110','10001','00001','00010','00100','00000','00100'
	'-'='00000','00000','00000','11111','00000','00000','00000'
	'+'='00000','00100','00100','11111','00100','00100','00000'
	'.'='00000','00000','00000','00000','00000','00100','00100'
	'<'='00010','00100','01000','10000','01000','00100','00010'
	'/'='00001','00010','00100','00100','01000','10000','10000'
	'>'='01000','00100','00010','00001','00010','00100','01000'
	"'"='00100','00100','00010','00000','00000','00000','00000'
	'*'='00000','10101','01110','11111','01110','10101','00000'
	' '='00000','00000','00000','00000','00000','00000','00000'
}

$columns = 16
$rows = [int][Math]::Ceiling($characters.Length / [float]$columns)
$cellSize = 16
$bitmap = [System.Drawing.Bitmap]::new($columns * $cellSize, $rows * $cellSize,
	[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$white = [System.Drawing.Color]::FromArgb(255, 255, 255, 255)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.TextRenderingHint = [Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
$fallbackFont = [Drawing.Font]::new('Consolas', 22, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
$fallbackBrush = [Drawing.SolidBrush]::new($white)
$fallbackFormat = [Drawing.StringFormat]::GenericTypographic.Clone()
$fallbackFormat.Alignment = [Drawing.StringAlignment]::Center
$fallbackFormat.LineAlignment = [Drawing.StringAlignment]::Center
$fallbackFormat.FormatFlags = [Drawing.StringFormatFlags]::NoWrap -bor [Drawing.StringFormatFlags]::NoClip

for ($index = 0; $index -lt $characters.Length; $index++)
{
	$character = [string]$characters[$index]
	$cellX = ($index % $columns) * $cellSize
	$cellY = [Math]::Floor($index / $columns) * $cellSize
	$rowsPattern = $patterns[$character]
	if (!$rowsPattern)
	{
		$graphics.DrawString($character, $fallbackFont, $fallbackBrush,
			[Drawing.RectangleF]::new($cellX, $cellY - 5, $cellSize, $cellSize + 8), $fallbackFormat)
		continue
	}

	for ($y = 0; $y -lt 7; $y++)
	{
		for ($x = 0; $x -lt 5; $x++)
		{
			if ($rowsPattern[$y][$x] -eq '1')
			{
				for ($pixelY = 0; $pixelY -lt 2; $pixelY++)
				{
					for ($pixelX = 0; $pixelX -lt 2; $pixelX++)
					{
						$bitmap.SetPixel($cellX + $x * 2 + $pixelX + 3,
							$cellY + $y * 2 + $pixelY + 1, $white)
					}
				}
			}
		}
	}
}

$fallbackFormat.Dispose()
$fallbackBrush.Dispose()
$fallbackFont.Dispose()
$graphics.Dispose()

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$atlasPath = Join-Path $OutputDirectory 'Arcade.png'
$bitmap.Save($atlasPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bitmap.Dispose()

$descriptor = [ordered]@{
	texture = 'Fonts/Arcade.png'
	characters = $characters
	columns = $columns
	rows = $rows
	glyphWidth = $cellSize
	glyphHeight = $cellSize
	advance = 16
	lineHeight = 18
} | ConvertTo-Json

$alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_'
$bytes = [Text.Encoding]::UTF8.GetBytes($descriptor)
$encoded = [Text.StringBuilder]::new()

for ($index = 0; $index -lt $bytes.Length; $index += 3)
{
	$remaining = $bytes.Length - $index
	[uint32]$group = 0

	for ($offset = 0; $offset -lt 3; $offset++)
	{
		$value = 0
		if ($offset -lt $remaining)
		{
			$key = if ((($index + $offset) % 2) -eq 0) { 7 } else { 210 }
			$value = $bytes[$index + $offset] -bxor $key
		}

		$group = $group -bor ([uint32]$value -shl (16 - 8 * $offset))
	}

	$count = [Math]::Min($remaining + 1, 4)
	for ($offset = 0; $offset -lt $count; $offset++)
	{
		[void]$encoded.Append($alphabet[($group -shr (18 - 6 * $offset)) -band 63])
	}
}

$wrapped = for ($index = 0; $index -lt $encoded.Length; $index += 76)
{
	$encoded.ToString($index, [Math]::Min(76, $encoded.Length - $index))
}

[IO.File]::WriteAllText((Join-Path $OutputDirectory 'Arcade.lnfont'), ($wrapped -join "`n") + "`n")
Write-Host "Generated $atlasPath"
