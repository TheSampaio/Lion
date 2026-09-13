$ErrorActionPreference = 'Stop'

$assetRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Sandbox\Assets'))
$alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_'

function Unseal-Content([string]$content)
{
	$stripped = $content -replace '[\s]', ''
	$bytes = [Collections.Generic.List[byte]]::new()

	for ($index = 0; $index -lt $stripped.Length; $index += 4)
	{
		$count = [Math]::Min(4, $stripped.Length - $index)
		[uint32]$group = 0

		for ($offset = 0; $offset -lt $count; $offset++)
		{
			$group = $group -bor ([uint32]$alphabet.IndexOf($stripped[$index + $offset]) -shl (18 - 6 * $offset))
		}

		for ($offset = 0; $offset + 1 -lt $count; $offset++)
		{
			$value = ($group -shr (16 - 8 * $offset)) -band 255
			$key = if (($bytes.Count % 2) -eq 0) { 7 } else { 210 }
			$bytes.Add([byte]($value -bxor $key))
		}
	}

	return [Text.Encoding]::UTF8.GetString($bytes.ToArray())
}

function Seal-Content([string]$content)
{
	$bytes = [Text.Encoding]::UTF8.GetBytes($content)
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

	$lines = for ($index = 0; $index -lt $encoded.Length; $index += 76)
	{
		$encoded.ToString($index, [Math]::Min(76, $encoded.Length - $index))
	}

	return ($lines -join "`n") + "`n"
}

function Read-SealedJson([string]$path)
{
	return (Unseal-Content ([IO.File]::ReadAllText($path))) | ConvertFrom-Json
}

function Write-SealedJson([string]$path, $value)
{
	$json = $value | ConvertTo-Json -Depth 100
	[IO.File]::WriteAllText($path, (Seal-Content $json))
}

function New-Transform([float]$x = 0, [float]$y = 0, [float]$scaleX = 1, [float]$scaleY = 1)
{
	return [ordered]@{
		position = @($x, $y)
		rotation = 0
		scale = @($scaleX, $scaleY)
	}
}

function New-TextComponent([string]$text, [float]$size, [int]$order = 100)
{
	return [ordered]@{
		Text = $text
		Font = 'Fonts/Arcade.lnfont'
		Size = $size
		Spacing = 0
		Centered = $true
		Order = $order
		'Color.x' = 1
		'Color.y' = 1
		'Color.z' = 1
		type = 'TextRenderer'
	}
}

function New-AnchorComponent([float]$anchorX, [float]$anchorY, [float]$offsetX = 0, [float]$offsetY = 0)
{
	return [ordered]@{
		'Anchor.x' = $anchorX
		'Anchor.y' = $anchorY
		'Anchor.z' = 0
		'Offset.x' = $offsetX
		'Offset.y' = $offsetY
		'Offset.z' = 0
		type = 'WidgetAnchor'
	}
}

function New-ButtonComponent([float]$width = 360, [float]$height = 56)
{
	return [ordered]@{
		Background = 'Sprites/UI/button-panel.png'
		'Size.x' = $width
		'Size.y' = $height
		'Size.z' = 0
		'Normal Color.x' = 0.86
		'Normal Color.y' = 0.86
		'Normal Color.z' = 0.86
		'Selected Color.x' = 1
		'Selected Color.y' = 1
		'Selected Color.z' = 1
		'Hovered Color.x' = 1
		'Hovered Color.y' = 1
		'Hovered Color.z' = 1
		'Pressed Color.x' = 0.65
		'Pressed Color.y' = 0.65
		'Pressed Color.z' = 0.65
		Order = 90
		Interactable = $true
		type = 'Button'
	}
}

function New-TextEntity([string]$name, [string]$text, [float]$size, [float]$anchorX,
	[float]$anchorY, [float]$offsetX, [float]$offsetY, [int]$parent, [bool]$visible = $true)
{
	$entity = [ordered]@{
		components = @(
			(New-TextComponent $text $size)
			(New-AnchorComponent $anchorX $anchorY $offsetX $offsetY)
		)
		name = $name
		parent = $parent
		transform = New-Transform
	}

	if (!$visible)
	{
		$entity.visible = $false
	}

	return $entity
}

function New-ButtonEntity([string]$name, [string]$label, [float]$anchorX, [float]$anchorY,
	[float]$offsetX, [float]$offsetY, [int]$parent, [float]$width = 360, [float]$height = 56)
{
	return [ordered]@{
		components = @(
			(New-TextComponent $label 28 100)
			(New-AnchorComponent $anchorX $anchorY $offsetX $offsetY)
			(New-ButtonComponent $width $height)
		)
		name = $name
		parent = $parent
		transform = New-Transform
	}
}

function New-AssemblyInstance([string]$path, [int]$parent = -1)
{
	return [ordered]@{
		assembly = $path
		parent = $parent
		placement = New-Transform
		visible = $true
	}
}

function New-SpriteEntity([string]$name, [string]$texture, [float]$anchorX, [float]$anchorY,
	[float]$offsetX, [float]$offsetY, [float]$scaleX, [float]$scaleY, [int]$parent,
	[int]$order = 100, [bool]$visible = $true)
{
	$entity = [ordered]@{
		components = @(
			[ordered]@{
				flipX = $false
				flipY = $false
				order = $order
				texture = $texture
				type = 'SpriteRenderer'
			}
			(New-AnchorComponent $anchorX $anchorY $offsetX $offsetY)
		)
		name = $name
		parent = $parent
		transform = New-Transform 0 0 $scaleX $scaleY
	}

	if (!$visible)
	{
		$entity.visible = $false
	}

	return $entity
}

function New-PostProcessingComponent
{
	return [ordered]@{
		Bloom = $true
		'Bloom Strength' = 0.18
		'Bloom Threshold' = 0.72
		'Color Correction' = $true
		Brightness = 0
		Contrast = 1.04
		Saturation = 1.08
		Gamma = 1
		'Tint.x' = 1
		'Tint.y' = 1
		'Tint.z' = 1
		Vignette = $true
		'Vignette Strength' = 0.14
		'Chromatic Aberration' = $false
		'Chromatic Aberration Amount' = 0.0015
		'Custom Shader' = ''
		type = 'PostProcessingComponent'
	}
}

$gameRulesAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @(
				[ordered]@{
					'Lose Height' = -310
					'Shake Duration' = 0.06
					'Shake Strength' = 0.72
					type = 'GameRules'
				}
				[ordered]@{
					Texture = 'Sprites/Brickout/particle.png'
					'Max Particles' = 160
					'Emission Rate' = 0
					Lifetime = 0.28
					Speed = 125
					Direction = 90
					Spread = 360
					'Start Size' = 15
					'End Size' = 2
					'Start Color.x' = 1
					'Start Color.y' = 1
					'Start Color.z' = 1
					'End Color.x' = 0.25
					'End Color.y' = 0.75
					'End Color.z' = 1
					Order = 70
					type = 'ParticleComponent'
				}
			)
			name = 'Game Rules'
			parent = -1
			transform = New-Transform
		}
	)
	root = 0
}
Write-SealedJson (Join-Path $assetRoot 'Assemblies\Game Rules.lnassembly') $gameRulesAssembly

$hudAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @()
			name = 'HUD'
			parent = -1
			transform = New-Transform
		}
		(New-TextEntity 'Score Text' 'SCORE 000000' 24 0 1 150 -32 0)
		(New-TextEntity 'Attempts Text' 'BALLS 3' 24 1 1 -110 -32 0)
	)
	root = 0
}
Write-SealedJson (Join-Path $assetRoot 'Assemblies\HUD.lnassembly') $hudAssembly

$background = [ordered]@{
	components = @(
		[ordered]@{
			flipX = $false
			flipY = $false
			order = -10
			texture = 'Sprites/Brickout/background.png'
			type = 'SpriteRenderer'
		}
		(New-AnchorComponent 0.5 0.5)
	)
	name = 'Background'
	parent = 0
	transform = New-Transform 0 0 4 4
}

$mainMenuAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @([ordered]@{ type = 'MainMenu' })
			name = 'Main Menu'
			parent = -1
			transform = New-Transform
		}
		$background
		(New-TextEntity 'Menu Title' 'BRICKOUT' 72 0.5 0.5 0 190 0)
		(New-TextEntity 'Menu Prompt' 'PRESS ANY KEY TO START' 28 0.5 0.5 0 -100 0)
		[ordered]@{
			components = @()
			name = 'Menu Options'
			parent = 0
			transform = New-Transform
			visible = $false
		}
		(New-ButtonEntity 'Play Button' 'PLAY' 0.5 0.5 0 70 4)
		(New-ButtonEntity 'Credits Button' 'CREDITS' 0.5 0.5 0 0 4)
		(New-ButtonEntity 'Settings Button' 'SETTINGS' 0.5 0.5 0 -70 4)
		(New-ButtonEntity 'Quit Button' 'QUIT' 0.5 0.5 0 -140 4)
		(New-TextEntity 'Menu Detail' 'CREDITS' 24 0.5 0.5 0 105 0 $false)
		(New-SpriteEntity 'Credits Logo' 'Images/sampaio-games-logo.png' 0.5 0.5 0 -50 0.105 0.105 0 100 $false)
		(New-ButtonEntity 'Sound Button' 'SOUND ON' 0.5 0.5 0 -20 9)
		(New-ButtonEntity 'Back Button' 'BACK' 0.5 0.5 0 -190 9)
	)
	root = 0
}
Write-SealedJson (Join-Path $assetRoot 'Assemblies\Main Menu.lnassembly') $mainMenuAssembly

$endBackground = [ordered]@{}
foreach ($property in $background.GetEnumerator())
{
	$endBackground[$property.Key] = $property.Value
}
$endBackground.parent = 0

$endScreenAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @([ordered]@{ type = 'EndScreen' })
			name = 'End Screen'
			parent = -1
			transform = New-Transform
		}
		$endBackground
		(New-TextEntity 'Result Title' 'YOU WIN!' 58 0.5 0.5 0 170 0)
		(New-TextEntity 'Result Score' "TOTAL SCORE`n000000" 38 0.5 0.5 0 40 0)
		(New-ButtonEntity 'Play Again Button' 'PLAY AGAIN' 0.5 0.5 0 -105 0)
		(New-ButtonEntity 'Main Menu Button' 'MAIN MENU' 0.5 0.5 0 -180 0)
	)
	root = 0
}
Write-SealedJson (Join-Path $assetRoot 'Assemblies\End Screen.lnassembly') $endScreenAssembly

$pauseAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @([ordered]@{ type = 'PauseMenu' })
			name = 'Pause Menu'
			parent = -1
			transform = New-Transform
		}
		[ordered]@{
			components = @()
			name = 'Pause Overlay'
			parent = 0
			transform = New-Transform
			visible = $false
		}
		[ordered]@{
			components = @(
				[ordered]@{
					Background = 'Sprites/UI/button-panel.png'
					'Size.x' = 1280
					'Size.y' = 720
					'Size.z' = 0
					'Normal Color.x' = 0.12
					'Normal Color.y' = 0.12
					'Normal Color.z' = 0.14
					'Selected Color.x' = 0.12
					'Selected Color.y' = 0.12
					'Selected Color.z' = 0.14
					'Hovered Color.x' = 0.12
					'Hovered Color.y' = 0.12
					'Hovered Color.z' = 0.14
					'Pressed Color.x' = 0.12
					'Pressed Color.y' = 0.12
					'Pressed Color.z' = 0.14
					Order = 80
					Interactable = $false
					type = 'Button'
				}
				(New-AnchorComponent 0.5 0.5)
			)
			name = 'Pause Dim'
			parent = 1
			transform = New-Transform
		}
		(New-TextEntity 'Pause Title' 'PAUSE' 58 0.5 0.5 0 115 1)
		(New-ButtonEntity 'Resume Button' 'RESUME' 0.5 0.5 0 10 1)
		(New-ButtonEntity 'Pause Main Menu Button' 'MAIN MENU' 0.5 0.5 0 -70 1)
	)
	root = 0
}
Write-SealedJson (Join-Path $assetRoot 'Assemblies\Pause Menu.lnassembly') $pauseAssembly

# Gameplay art is authored as compact 16x16 pixel sprites. Preserve the established world-space
# dimensions through entity scale and matching unscaled collider dimensions.
$paddlePath = Join-Path $assetRoot 'Assemblies\Paddle.lnassembly'
$paddleAssembly = Read-SealedJson $paddlePath
$paddleRoot = $paddleAssembly.entities[$paddleAssembly.root]
$paddleRoot.transform.scale = @(6.25, 1.25)
$paddleCollider = $paddleRoot.components | Where-Object { $_.type -eq 'BoxCollider2D' } | Select-Object -First 1

if ($paddleCollider)
{
	$paddleCollider.width = 16
	$paddleCollider.height = 16
}

Write-SealedJson $paddlePath $paddleAssembly

for ($level = 1; $level -le 5; $level++)
{
	$scenePath = Join-Path $assetRoot ("Scenes\Level{0:D2}.lnscene" -f $level)
	$scene = Read-SealedJson $scenePath
	$systemsIndex = -1
	$hasHud = $false
	$hasPauseMenu = $false

	for ($index = 0; $index -lt $scene.entities.Count; $index++)
	{
		$entity = $scene.entities[$index]
		$sprite = $entity.components | Where-Object { $_.type -eq 'SpriteRenderer' } | Select-Object -First 1

		if ($entity.name -eq 'Systems')
		{
			$systemsIndex = $index
		}

		if ($entity.assembly -like 'Assemblies/Game Rules Level *.lnassembly')
		{
			$entity.assembly = 'Assemblies/Game Rules.lnassembly'
		}

		if ($entity.assembly -eq 'Assemblies/HUD.lnassembly')
		{
			$hasHud = $true
		}

		if ($entity.assembly -eq 'Assemblies/Pause Menu.lnassembly')
		{
			$hasPauseMenu = $true
		}

		if ($entity.name -eq 'Camera' -and !($entity.components | Where-Object { $_.type -eq 'PostProcessingComponent' }))
		{
			$entity.components = @($entity.components) + @((New-PostProcessingComponent))
		}

		if ($sprite -and $sprite.texture -eq 'Sprites/Brickout/background.jpg')
		{
			$sprite.texture = 'Sprites/Brickout/background.png'
			$entity.transform.scale = @(4, 4)
		}
		elseif ($sprite -and $sprite.texture -eq 'Sprites/Brickout/ball.png')
		{
			$entity.transform.scale = @(0.75, 0.75)
			$collider = $entity.components | Where-Object { $_.type -eq 'CircleCollider2D' } | Select-Object -First 1

			if ($collider)
			{
				$collider.radius = 8
			}
		}
		elseif ($sprite -and $sprite.texture -like 'Sprites/Brickout/tile-*.png')
		{
			$entity.transform.scale = @(3.75, 1.5)
			$collider = $entity.components | Where-Object { $_.type -eq 'BoxCollider2D' } | Select-Object -First 1

			if ($collider)
			{
				$collider.width = 16
				$collider.height = 16
			}
		}
	}

	if (!$hasHud)
	{
		$scene.entities = @($scene.entities) + @((New-AssemblyInstance 'Assemblies/HUD.lnassembly' $systemsIndex))
	}

	if (!$hasPauseMenu)
	{
		$scene.entities = @($scene.entities) + @((New-AssemblyInstance 'Assemblies/Pause Menu.lnassembly' $systemsIndex))
	}

	Write-SealedJson $scenePath $scene
}

$mainMenuPath = Join-Path $assetRoot 'Scenes\MainMenu.lnscene'
$mainMenuScene = Read-SealedJson $mainMenuPath
$hasGameRules = $mainMenuScene.entities | Where-Object { $_.assembly -eq 'Assemblies/Game Rules.lnassembly' }

if (!$hasGameRules)
{
	$mainMenuScene.entities = @($mainMenuScene.entities) + @((New-AssemblyInstance 'Assemblies/Game Rules.lnassembly'))
}

foreach ($entity in $mainMenuScene.entities)
{
	if ($entity.name -eq 'Camera' -and !($entity.components | Where-Object { $_.type -eq 'PostProcessingComponent' }))
	{
		$entity.components = @($entity.components) + @((New-PostProcessingComponent))
	}
}

Write-SealedJson $mainMenuPath $mainMenuScene

function New-EndScene
{
	return [ordered]@{
		entities = @(
			(New-AssemblyInstance 'Assemblies/End Screen.lnassembly')
			(New-AssemblyInstance 'Assemblies/Game Rules.lnassembly')
			[ordered]@{
				components = @(
					[ordered]@{
						limit = $false
						limitBottom = -360
						limitLeft = -640
						limitRight = 640
						limitTop = 360
						offsetX = 0
						offsetY = 0
						positionSmoothing = 5
						rotationSmoothing = 5
						smooth = $false
						type = 'Camera2D'
						zoom = 1
					}
					(New-PostProcessingComponent)
				)
				name = 'Camera'
				parent = -1
				transform = New-Transform
			}
		)
		gravity = @(0, 0)
	}
}

Write-SealedJson (Join-Path $assetRoot 'Scenes\Victory.lnscene') (New-EndScene)
Write-SealedJson (Join-Path $assetRoot 'Scenes\Defeat.lnscene') (New-EndScene)

$inputPath = Join-Path $assetRoot 'Config\Input.lninput'
$inputMap = Read-SealedJson $inputPath
$menuActions = @(
	[ordered]@{ name='menu_up'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=265; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=11; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_down'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=264; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=13; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_left'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=263; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=14; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_right'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=262; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=12; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_confirm'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=257; scale=1; gamepad=-1 },
		[ordered]@{ device='keyboard'; code=32; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=0; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_back'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=256; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=1; scale=1; gamepad=-1 }) }
)

$menuNames = @($menuActions | ForEach-Object { $_.name })
$inputMap.actions = @($inputMap.actions | Where-Object { $_.name -notin $menuNames }) + $menuActions
Write-SealedJson $inputPath $inputMap

Write-Host 'Brickout scenes, assemblies and menu input actions are up to date.'
