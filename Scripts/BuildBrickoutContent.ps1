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
		'Normal Color.x' = 0.12
		'Normal Color.y' = 0.82
		'Normal Color.z' = 1
		'Selected Color.x' = 1
		'Selected Color.y' = 0.22
		'Selected Color.z' = 0.72
		'Hovered Color.x' = 1
		'Hovered Color.y' = 0.22
		'Hovered Color.z' = 0.72
		'Pressed Color.x' = 1
		'Pressed Color.y' = 1
		'Pressed Color.z' = 1
		Order = 90
		Interactable = $true
		type = 'Button'
	}
}

function New-PanelEntity([string]$name, [float]$width, [float]$height, [float]$anchorX,
	[float]$anchorY, [float]$offsetX, [float]$offsetY, [int]$parent, [int]$order = 80,
	[bool]$visible = $true, [string]$texture = 'Sprites/UI/neon-panel.png')
{
	$panel = New-ButtonComponent $width $height
	$panel.Background = $texture
	$panel.'Normal Color.x' = 1
	$panel.'Normal Color.y' = 1
	$panel.'Normal Color.z' = 1
	$panel.'Selected Color.x' = 1
	$panel.'Selected Color.y' = 1
	$panel.'Selected Color.z' = 1
	$panel.'Hovered Color.x' = 1
	$panel.'Hovered Color.y' = 1
	$panel.'Hovered Color.z' = 1
	$panel.'Pressed Color.x' = 1
	$panel.'Pressed Color.y' = 1
	$panel.'Pressed Color.z' = 1
	$panel.Order = $order
	$panel.Interactable = $false

	$entity = [ordered]@{
		components = @($panel, (New-AnchorComponent $anchorX $anchorY $offsetX $offsetY))
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
	[float]$offsetX, [float]$offsetY, [int]$parent, [float]$width = 360, [float]$height = 56,
	[float]$textSize = 28, [bool]$visible = $true)
{
	$entity = [ordered]@{
		components = @(
			(New-TextComponent $label $textSize 100)
			(New-AnchorComponent $anchorX $anchorY $offsetX $offsetY)
			(New-ButtonComponent $width $height)
		)
		name = $name
		parent = $parent
		transform = New-Transform
	}
	if (!$visible) { $entity.visible = $false }
	return $entity
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
		'Bloom Strength' = 0.46
		'Bloom Threshold' = 0.5
		'Color Correction' = $true
		Brightness = -0.01
		Contrast = 1.05
		Saturation = 1.1
		Gamma = 1
		'Tint.x' = 1
		'Tint.y' = 1
		'Tint.z' = 1
		Vignette = $true
		'Vignette Strength' = 0.12
		'Chromatic Aberration' = $true
		'Chromatic Aberration Amount' = 0.001
		'Motion Blur' = $true
		'Motion Blur Strength' = 0.075
		'Color Vision Mode' = 0
		Fade = 0
		'Custom Shader' = ''
		type = 'PostProcessingComponent'
	}
}

function New-GroupEntity([string]$name, [int]$parent, [bool]$visible = $false)
{
	$entity = [ordered]@{
		components = @()
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

$gameRulesAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @(
				[ordered]@{
					'Lose Height' = -310
					'Shake Duration' = 0.06
					'Shake Strength' = 0.5
					type = 'GameRules'
				}
				[ordered]@{
					Texture = 'Sprites/Brickout/particle.png'
					'Max Particles' = 320
					'Emission Rate' = 0
					Lifetime = 0.34
					Speed = 155
					Direction = 90
					Spread = 360
					'Start Size' = 10
					'End Size' = 1
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

$screenTransitionAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @([ordered]@{ type = 'ScreenTransition' })
			name = 'Screen Transition'
			parent = -1
			transform = New-Transform
		}
	)
	root = 0
}
Write-SealedJson (Join-Path $assetRoot 'Assemblies\Screen Transition.lnassembly') $screenTransitionAssembly

$hudAssembly = [ordered]@{
	entities = @(
		[ordered]@{
			components = @()
			name = 'HUD'
			parent = -1
			transform = New-Transform
		}
		(New-PanelEntity 'HUD Bar' 1280 72 0.5 1 0 -36 0 85 $true 'Sprites/UI/top-bar.png')
		(New-TextEntity 'Score Text' 'SCORE 000000' 20 0 1 125 -34 0)
		(New-TextEntity 'Combo Text' 'COMBO X2' 17 0.5 1 -275 -34 0 $false)
		(New-TextEntity 'Level Text' 'LEVEL 01' 20 0.5 1 0 -34 0)
		(New-TextEntity 'Shockwave Text' 'SHOCKWAVE 0/8' 15 0.5 1 270 -34 0)
		(New-TextEntity 'Attempts Text' 'BALLS 3' 20 1 1 -88 -34 0)
		(New-TextEntity 'Power Text' 'MULTIBALL x3' 22 0.5 1 0 -92 0 $false)
	)
	root = 0
}
$hudPromptIndex = $hudAssembly.entities.Count
$hudAssembly.entities = @($hudAssembly.entities) + @(
	(New-GroupEntity 'HUD Controller Prompts' 0),
	(New-SpriteEntity 'HUD Move Icon' 'Sprites/UI/controller-stick.png' 0.5 0 -285 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Move Label' 'MOVE' 12 0.5 0 -235 27 $hudPromptIndex),
	(New-SpriteEntity 'HUD Launch Icon' 'Sprites/UI/controller-a.png' 0.5 0 -130 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Launch Label' 'LAUNCH' 12 0.5 0 -74 27 $hudPromptIndex),
	(New-SpriteEntity 'HUD Power Icon' 'Sprites/UI/controller-x.png' 0.5 0 50 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Power Label' 'WAVE' 12 0.5 0 100 27 $hudPromptIndex),
	(New-SpriteEntity 'HUD Pause Icon' 'Sprites/UI/controller-start.png' 0.5 0 215 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Pause Label' 'PAUSE' 12 0.5 0 280 27 $hudPromptIndex)
)
$hudKeyboardPromptIndex = $hudAssembly.entities.Count
$hudAssembly.entities = @($hudAssembly.entities) + @(
	(New-GroupEntity 'HUD Keyboard Prompts' 0),
	(New-SpriteEntity 'HUD Keyboard Move Icon' 'Sprites/UI/key-arrows.png' 0.5 0 -292 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Move Label' 'MOVE' 12 0.5 0 -220 27 $hudKeyboardPromptIndex),
	(New-SpriteEntity 'HUD Keyboard Launch Icon' 'Sprites/UI/key-space.png' 0.5 0 -105 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Launch Label' 'LAUNCH' 12 0.5 0 -34 27 $hudKeyboardPromptIndex),
	(New-SpriteEntity 'HUD Keyboard Power Icon' 'Sprites/UI/key-e.png' 0.5 0 82 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Power Label' 'WAVE' 12 0.5 0 130 27 $hudKeyboardPromptIndex),
	(New-SpriteEntity 'HUD Keyboard Pause Icon' 'Sprites/UI/key-esc.png' 0.5 0 230 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Pause Label' 'PAUSE' 12 0.5 0 285 27 $hudKeyboardPromptIndex)
)
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
	transform = New-Transform
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
		(New-PanelEntity 'Menu Panel' 760 690 0.5 0.5 0 0 0 70)
		(New-TextEntity 'Menu Title' 'BRICKOUT' 58 0.5 0.5 0 286 0)
		(New-TextEntity 'Menu Subtitle' 'NEON CIRCUIT' 19 0.5 0.5 0 232 0)
		(New-TextEntity 'Menu Prompt' 'PRESS TO START' 22 0.5 0.5 0 -20 0)
		[ordered]@{
			components = @()
			name = 'Menu Options'
			parent = 0
			transform = New-Transform
			visible = $false
		}
		(New-ButtonEntity 'Continue Button' 'CONTINUE' 0.5 0.5 0 132 6 500 46 22)
		(New-ButtonEntity 'Level Select Button' 'LEVEL SELECT' 0.5 0.5 0 78 6 500 46 22)
		(New-ButtonEntity 'Statistics Button' 'STATISTICS' 0.5 0.5 0 24 6 500 46 22)
		(New-ButtonEntity 'Credits Button' 'CREDITS' 0.5 0.5 0 -30 6 500 46 22)
		(New-ButtonEntity 'Settings Button' 'SETTINGS' 0.5 0.5 0 -84 6 500 46 22)
		(New-ButtonEntity 'Quit Button' 'QUIT' 0.5 0.5 0 -138 6 500 46 22)
		(New-TextEntity 'Menu Detail' 'CREDITS' 23 0.5 0.5 0 186 0 $false)
		(New-SpriteEntity 'Credits Logo' 'Images/sampaio-games-logo.png' 0.5 0.5 0 -35 0.1 0.1 0 100 $false)
		(New-GroupEntity 'Settings Options' 0)
		(New-ButtonEntity 'Sound Button' 'SOUND' 0.5 0.5 0 142 15 600 30 15)
		(New-ButtonEntity 'Resolution Button' 'RESOLUTION' 0.5 0.5 0 108 15 600 30 15)
		(New-ButtonEntity 'VSync Button' 'V-SYNC' 0.5 0.5 0 74 15 600 30 15)
		(New-ButtonEntity 'Quality Button' 'GRAPHICS' 0.5 0.5 0 40 15 600 30 15)
		(New-ButtonEntity 'Bloom Button' 'BLOOM' 0.5 0.5 0 6 15 600 30 15)
		(New-ButtonEntity 'Vignette Button' 'VIGNETTE' 0.5 0.5 0 -28 15 600 30 15)
		(New-ButtonEntity 'Motion Blur Button' 'MOTION BLUR' 0.5 0.5 0 -62 15 600 30 15)
		(New-ButtonEntity 'Camera Shake Button' 'CAMERA SHAKE' 0.5 0.5 0 -96 15 600 30 15)
		(New-ButtonEntity 'Color Mode Button' 'COLOR MODE' 0.5 0.5 0 -130 15 600 30 15)
		(New-ButtonEntity 'Language Button' 'LANGUAGE' 0.5 0.5 0 -164 15 600 30 15)
		(New-ButtonEntity 'Control Hints Button' 'CONTROL HINTS' 0.5 0.5 0 -198 15 600 30 15)
		(New-GroupEntity 'Level Options' 0)
		(New-ButtonEntity 'Level 1 Button' 'LEVEL 01' 0.5 0.5 0 126 27 560 46 18)
		(New-ButtonEntity 'Level 2 Button' 'LEVEL 02' 0.5 0.5 0 70 27 560 46 18)
		(New-ButtonEntity 'Level 3 Button' 'LEVEL 03' 0.5 0.5 0 14 27 560 46 18)
		(New-ButtonEntity 'Level 4 Button' 'LEVEL 04' 0.5 0.5 0 -42 27 560 46 18)
		(New-ButtonEntity 'Level 5 Button' 'LEVEL 05' 0.5 0.5 0 -98 27 560 46 18)
		(New-GroupEntity 'Statistics Panel' 0)
		(New-TextEntity 'Statistics Text' 'STATISTICS' 21 0.5 0.5 0 118 33)
		(New-ButtonEntity 'Back Button' 'BACK' 0.5 0.5 0 -252 0 600 36 17 $false)
	)
	root = 0
}
$attractPromptIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Attract Prompt' 0),
	(New-SpriteEntity 'Controller Start Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 -125 -20 1 1 $attractPromptIndex),
	(New-TextEntity 'Controller Start Label' 'PRESS TO START' 20 0.5 0.5 25 -21 $attractPromptIndex)
)
$keyboardAttractIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Attract Prompt' 0),
	(New-SpriteEntity 'Keyboard Start Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 -125 -20 1 1 $keyboardAttractIndex),
	(New-TextEntity 'Keyboard Start Label' 'PRESS TO START' 20 0.5 0.5 25 -21 $keyboardAttractIndex)
)
$menuPromptIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Menu Prompts' 0),
	(New-SpriteEntity 'Controller Navigate Icon' 'Sprites/UI/controller-dpad.png' 0.5 0.5 -180 -300 1 1 $menuPromptIndex),
	(New-TextEntity 'Controller Navigate Label' 'NAVIGATE' 13 0.5 0.5 -120 -301 $menuPromptIndex),
	(New-SpriteEntity 'Controller Select Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 45 -300 1 1 $menuPromptIndex),
	(New-TextEntity 'Controller Select Label' 'SELECT' 13 0.5 0.5 100 -301 $menuPromptIndex)
)
$keyboardMenuIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Menu Prompts' 0),
	(New-SpriteEntity 'Keyboard Navigate Icon' 'Sprites/UI/key-arrows.png' 0.5 0.5 -180 -300 1 1 $keyboardMenuIndex),
	(New-TextEntity 'Keyboard Navigate Label' 'NAVIGATE' 13 0.5 0.5 -108 -301 $keyboardMenuIndex),
	(New-SpriteEntity 'Keyboard Select Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 52 -300 1 1 $keyboardMenuIndex),
	(New-TextEntity 'Keyboard Select Label' 'SELECT' 13 0.5 0.5 110 -301 $keyboardMenuIndex)
)
$detailPromptIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Detail Prompts' 0),
	(New-SpriteEntity 'Controller Back Icon' 'Sprites/UI/controller-b.png' 0.5 0.5 70 -300 1 1 $detailPromptIndex),
	(New-TextEntity 'Controller Back Label' 'BACK' 13 0.5 0.5 118 -301 $detailPromptIndex)
)
$keyboardDetailIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Detail Prompts' 0),
	(New-SpriteEntity 'Keyboard Back Icon' 'Sprites/UI/key-esc.png' 0.5 0.5 62 -300 1 1 $keyboardDetailIndex),
	(New-TextEntity 'Keyboard Back Label' 'BACK' 13 0.5 0.5 116 -301 $keyboardDetailIndex)
)
$settingsPromptIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Settings Prompt' 0),
	(New-SpriteEntity 'Controller Toggle Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 -115 -300 1 1 $settingsPromptIndex),
	(New-TextEntity 'Controller Toggle Label' 'TOGGLE' 13 0.5 0.5 -60 -301 $settingsPromptIndex)
)
$keyboardSettingsIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Settings Prompt' 0),
	(New-SpriteEntity 'Keyboard Toggle Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 -125 -300 1 1 $keyboardSettingsIndex),
	(New-TextEntity 'Keyboard Toggle Label' 'TOGGLE' 13 0.5 0.5 -65 -301 $keyboardSettingsIndex)
)
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
		(New-PanelEntity 'Result Panel' 720 610 0.5 0.5 0 0 0 70)
		(New-TextEntity 'Result Title' 'CIRCUIT CLEARED' 45 0.5 0.5 0 198 0)
		(New-TextEntity 'Result Score' "TOTAL SCORE`n000000" 30 0.5 0.5 0 70 0)
		(New-ButtonEntity 'Play Again Button' 'PLAY AGAIN' 0.5 0.5 0 -66 0 460 52 25)
		(New-ButtonEntity 'Main Menu Button' 'MAIN MENU' 0.5 0.5 0 -132 0 460 52 25)
	)
	root = 0
}
$endPromptIndex = $endScreenAssembly.entities.Count
$endScreenAssembly.entities = @($endScreenAssembly.entities) + @(
	(New-GroupEntity 'End Controller Prompts' 0),
	(New-SpriteEntity 'End Navigate Icon' 'Sprites/UI/controller-dpad.png' 0.5 0.5 -180 -236 1 1 $endPromptIndex),
	(New-TextEntity 'End Navigate Label' 'NAVIGATE' 13 0.5 0.5 -120 -237 $endPromptIndex),
	(New-SpriteEntity 'End Select Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 35 -236 1 1 $endPromptIndex),
	(New-TextEntity 'End Select Label' 'SELECT' 13 0.5 0.5 88 -237 $endPromptIndex),
	(New-SpriteEntity 'End Back Icon' 'Sprites/UI/controller-b.png' 0.5 0.5 165 -236 1 1 $endPromptIndex),
	(New-TextEntity 'End Back Label' 'MENU' 13 0.5 0.5 210 -237 $endPromptIndex)
)
$endKeyboardPromptIndex = $endScreenAssembly.entities.Count
$endScreenAssembly.entities = @($endScreenAssembly.entities) + @(
	(New-GroupEntity 'End Keyboard Prompts' 0),
	(New-SpriteEntity 'End Keyboard Navigate Icon' 'Sprites/UI/key-arrows.png' 0.5 0.5 -180 -236 1 1 $endKeyboardPromptIndex),
	(New-TextEntity 'End Keyboard Navigate Label' 'NAVIGATE' 13 0.5 0.5 -108 -237 $endKeyboardPromptIndex),
	(New-SpriteEntity 'End Keyboard Select Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 55 -236 1 1 $endKeyboardPromptIndex),
	(New-TextEntity 'End Keyboard Select Label' 'SELECT' 13 0.5 0.5 112 -237 $endKeyboardPromptIndex),
	(New-SpriteEntity 'End Keyboard Back Icon' 'Sprites/UI/key-esc.png' 0.5 0.5 195 -236 1 1 $endKeyboardPromptIndex),
	(New-TextEntity 'End Keyboard Back Label' 'MENU' 13 0.5 0.5 242 -237 $endKeyboardPromptIndex)
)
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
		(New-PanelEntity 'Pause Panel' 660 460 0.5 0.5 0 0 1 85)
		(New-TextEntity 'Pause Title' 'PAUSED' 44 0.5 0.5 0 138 1)
		(New-TextEntity 'Pause Subtitle' 'CIRCUIT SUSPENDED' 19 0.5 0.5 0 84 1)
		(New-ButtonEntity 'Resume Button' 'RESUME' 0.5 0.5 0 -2 1 460 52 25)
		(New-ButtonEntity 'Pause Main Menu Button' 'MAIN MENU' 0.5 0.5 0 -68 1 460 52 25)
	)
	root = 0
}
$pausePromptIndex = $pauseAssembly.entities.Count
$pauseAssembly.entities = @($pauseAssembly.entities) + @(
	(New-GroupEntity 'Pause Controller Prompts' 1),
	(New-SpriteEntity 'Pause Navigate Icon' 'Sprites/UI/controller-dpad.png' 0.5 0.5 -165 -178 1 1 $pausePromptIndex),
	(New-TextEntity 'Pause Navigate Label' 'NAVIGATE' 13 0.5 0.5 -105 -179 $pausePromptIndex),
	(New-SpriteEntity 'Pause Select Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 60 -178 1 1 $pausePromptIndex),
	(New-TextEntity 'Pause Select Label' 'SELECT' 13 0.5 0.5 112 -179 $pausePromptIndex)
)
$pauseKeyboardPromptIndex = $pauseAssembly.entities.Count
$pauseAssembly.entities = @($pauseAssembly.entities) + @(
	(New-GroupEntity 'Pause Keyboard Prompts' 1),
	(New-SpriteEntity 'Pause Keyboard Navigate Icon' 'Sprites/UI/key-arrows.png' 0.5 0.5 -175 -178 1 1 $pauseKeyboardPromptIndex),
	(New-TextEntity 'Pause Keyboard Navigate Label' 'NAVIGATE' 13 0.5 0.5 -103 -179 $pauseKeyboardPromptIndex),
	(New-SpriteEntity 'Pause Keyboard Select Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 68 -178 1 1 $pauseKeyboardPromptIndex),
	(New-TextEntity 'Pause Keyboard Select Label' 'SELECT' 13 0.5 0.5 126 -179 $pauseKeyboardPromptIndex)
)
Write-SealedJson (Join-Path $assetRoot 'Assemblies\Pause Menu.lnassembly') $pauseAssembly

# Gameplay art is authored as detailed 32x32 neon sprites. Preserve the established world-space
# dimensions through entity scale and matching unscaled collider dimensions.
$paddlePath = Join-Path $assetRoot 'Assemblies\Paddle.lnassembly'
$paddleAssembly = Read-SealedJson $paddlePath
$paddleRoot = $paddleAssembly.entities[$paddleAssembly.root]
$paddleRoot.transform.scale = @(1, 1)
$paddleCollider = $paddleRoot.components | Where-Object { $_.type -eq 'BoxCollider2D' } | Select-Object -First 1

if ($paddleCollider)
{
	$paddleCollider.width = 100
	$paddleCollider.height = 20
}

$paddleBehavior = $paddleRoot.components | Where-Object { $_.type -eq 'Paddle' } | Select-Object -First 1
if ($paddleBehavior)
{
	$paddleBehavior.Speed = 550
	$paddleBehavior.'Horizontal Limit' = 350
}

Write-SealedJson $paddlePath $paddleAssembly

function Get-LevelLayout([int]$level, [int]$count)
{
	$layout = [Collections.Generic.List[object]]::new()

	if ($level -eq 1)
	{
		for ($index = 0; $index -lt $count; $index++)
		{
			$x = -280 + $index * 80
			$layout.Add([pscustomobject]@{ x=$x; y=(145 + [Math]::Abs($x) * 0.32); rotation=0 })
		}
	}
	elseif ($level -eq 2)
	{
		for ($index = 0; $index -lt $count; $index++)
		{
			$angle = -[Math]::PI * 0.5 + 2 * [Math]::PI * $index / $count
			$layout.Add([pscustomobject]@{
				x=[Math]::Round([Math]::Cos($angle) * 285, 1)
				y=[Math]::Round(145 + [Math]::Sin($angle) * 105, 1)
				rotation=0
			})
		}
	}
	elseif ($level -eq 3)
	{
		$heart = @(
			@(-210, 220), @(-140, 220), @(-70, 220), @(70, 220), @(140, 220), @(210, 220),
			@(-280, 175), @(-210, 175), @(-140, 175), @(-70, 175), @(0, 175),
			@(70, 175), @(140, 175), @(210, 175), @(280, 175),
			@(-245, 120), @(245, 120), @(-190, 65), @(190, 65),
			@(-130, 10), @(130, 10), @(-65, -45), @(65, -45), @(0, -100)
		)
		for ($index = 0; $index -lt $count; $index++)
		{
			$point = $heart[$index % $heart.Count]
			$layout.Add([pscustomobject]@{ x=$point[0]; y=$point[1]; rotation=0 })
		}
	}
	elseif ($level -eq 4)
	{
		$star = @(
			@(0, 235),
			@(-280, 170), @(-210, 170), @(-140, 170), @(-70, 170), @(0, 170),
			@(70, 170), @(140, 170), @(210, 170), @(280, 170),
			@(-210, 125), @(-140, 125), @(-70, 125), @(0, 125), @(70, 125), @(140, 125), @(210, 125),
			@(-140, 80), @(-70, 80), @(0, 80), @(70, 80), @(140, 80),
			@(-140, 35), @(-70, 35), @(70, 35), @(140, 35),
			@(-210, -10), @(-140, -10), @(140, -10), @(210, -10),
			@(-210, -55), @(210, -55)
		)
		for ($index = 0; $index -lt $count; $index++)
		{
			$point = $star[$index % $star.Count]
			$layout.Add([pscustomobject]@{ x=$point[0]; y=$point[1]; rotation=0 })
		}
	}
	else
	{
		for ($index = 0; $index -lt $count; $index++)
		{
			$row = [Math]::Floor($index / 10)
			$column = $index % 10
			$x = -315 + $column * 70
			$ridge = @(-15, 45, -5, 70, 5, 70, -5, 45, -15, 20)[$column]
			$y = 190 - $row * 42 + $ridge * (1.0 - $row / 4.0)
			$layout.Add([pscustomobject]@{ x=$x; y=[Math]::Round($y, 1); rotation=0 })
		}
	}

	return @($layout)
}

function New-WorldSprite([string]$name, [string]$texture, [float]$x, [float]$y,
	[float]$scaleX, [float]$scaleY, [float]$rotation, [int]$parent = -1, [int]$order = 5)
{
	return [ordered]@{
		components = @([ordered]@{ flipX=$false; flipY=$false; order=$order; texture=$texture; type='SpriteRenderer' })
		name = $name
		parent = $parent
		transform = [ordered]@{ position=@($x, $y); rotation=$rotation; scale=@($scaleX, $scaleY) }
	}
}

function New-Bumper([string]$name, [float]$x, [float]$y)
{
	$entity = New-WorldSprite $name 'Sprites/Brickout/bumper.png' $x $y 1 1 0 -1 4
	$entity.components = @($entity.components) + @(
		[ordered]@{ bodyType='Static'; fixedRotation=$false; type='RigidBody2D' },
		[ordered]@{ density=1; friction=0; radius=20; restitution=1; type='CircleCollider2D' }
	)
	return $entity
}

function New-Rail([string]$name, [float]$x, [float]$y, [float]$rotation)
{
	$entity = New-WorldSprite $name 'Sprites/Brickout/pinball-rail.png' $x $y 1 1 $rotation -1 3
	$entity.components = @($entity.components) + @(
		[ordered]@{ bodyType='Static'; fixedRotation=$false; type='RigidBody2D' },
		[ordered]@{ density=1; friction=0; width=120; height=12; restitution=1; type='BoxCollider2D' }
	)
	return $entity
}

function New-Post([string]$name, [float]$x, [float]$y)
{
	$entity = New-WorldSprite $name 'Sprites/Brickout/bumper.png' $x $y 0.65 0.65 0 -1 4
	$entity.components = @($entity.components) + @(
		[ordered]@{ bodyType='Static'; fixedRotation=$false; type='RigidBody2D' },
		[ordered]@{ density=1; friction=0; radius=20; restitution=1; type='CircleCollider2D' }
	)
	return $entity
}

for ($level = 1; $level -le 5; $level++)
{
	$scenePath = Join-Path $assetRoot ("Scenes\Level{0:D2}.lnscene" -f $level)
	$scene = Read-SealedJson $scenePath
	$scene.entities = @($scene.entities | Where-Object {
		$_.name -notlike 'Power Icon *' -and $_.name -notlike 'Arena *'
	})
	$systemsIndex = -1
	$hasHud = $false
	$hasPauseMenu = $false
	$hasScreenTransition = $false
	$brickIndices = [Collections.Generic.List[int]]::new()

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

		if ($entity.assembly -eq 'Assemblies/Screen Transition.lnassembly')
		{
			$hasScreenTransition = $true
		}

		if ($entity.name -eq 'Camera')
		{
			$entity.components = @($entity.components | Where-Object { $_.type -ne 'PostProcessingComponent' }) + @((New-PostProcessingComponent))
		}

		if ($sprite -and $sprite.texture -like 'Sprites/Brickout/background.*')
		{
			$sprite.texture = 'Sprites/Brickout/background.png'
			$entity.transform.scale = @(1, 1)
		}
		elseif ($sprite -and $sprite.texture -eq 'Sprites/Brickout/ball.png')
		{
			$entity.transform.scale = @(1, 1)
			$collider = $entity.components | Where-Object { $_.type -eq 'CircleCollider2D' } | Select-Object -First 1

			if ($collider)
			{
				$collider.radius = 7
			}

			$ballBehavior = $entity.components | Where-Object { $_.type -eq 'Ball' } | Select-Object -First 1
			if ($ballBehavior)
			{
				$ballBehavior.Speed = 390
				$ballBehavior.'Maximum Bounce Angle' = 55
			}

			if (!($entity.components | Where-Object { $_.type -eq 'ParticleComponent' }))
			{
				$entity.components = @($entity.components) + @([ordered]@{
					Texture='Sprites/Brickout/particle.png'; 'Max Particles'=96; 'Emission Rate'=48
					Lifetime=0.2; Speed=26; Direction=180; Spread=34; 'Start Size'=7; 'End Size'=1
					'Start Color.x'=1; 'Start Color.y'=1; 'Start Color.z'=1
					'End Color.x'=0.1; 'End Color.y'=0.75; 'End Color.z'=1; Order=12
					type='ParticleComponent'
				})
			}
		}
		elseif ($sprite -and $sprite.texture -like 'Sprites/Brickout/tile-*.png')
		{
			$brickIndices.Add($index)
			$entity.transform.scale = @(1, 1)
			$collider = $entity.components | Where-Object { $_.type -eq 'BoxCollider2D' } | Select-Object -First 1

			if ($collider)
			{
				$collider.width = 60
				$collider.height = 24
			}
		}
		elseif ($entity.name -eq 'Top Wall')
		{
			$entity.transform.scale = @(1, 1)
			$collider = $entity.components | Where-Object { $_.type -eq 'BoxCollider2D' } | Select-Object -First 1
			if ($collider) { $collider.width = 800; $collider.height = 24 }
			if (!$sprite)
			{
				$entity.components = @([ordered]@{ flipX=$false; flipY=$false; order=2; texture='Sprites/Brickout/wall-horizontal-v2.png'; type='SpriteRenderer' }) + @($entity.components)
			}
			else { $sprite.texture = 'Sprites/Brickout/wall-horizontal-v2.png' }
		}
		elseif ($entity.name -eq 'Left Wall' -or $entity.name -eq 'Right Wall')
		{
			$entity.transform.scale = @(1, 1)
			$collider = $entity.components | Where-Object { $_.type -eq 'BoxCollider2D' } | Select-Object -First 1
			if ($collider) { $collider.width = 24; $collider.height = 600 }
			if (!$sprite)
			{
				$entity.components = @([ordered]@{ flipX=$false; flipY=$false; order=2; texture='Sprites/Brickout/wall-vertical-v2.png'; type='SpriteRenderer' }) + @($entity.components)
			}
			else { $sprite.texture = 'Sprites/Brickout/wall-vertical-v2.png' }
		}
	}

	$layout = Get-LevelLayout $level $brickIndices.Count
	$powerPlan = switch ($level)
	{
		1 { @('Extra Life', 'Wide Paddle') }
		2 { @('Bomb', 'Multiball') }
		3 { @('Extra Life', 'Duplicate Paddle', 'Bomb') }
		4 { @('Multiball', 'Wide Paddle', 'Bomb') }
		default { @('Extra Life', 'Multiball', 'Bomb', 'Wide Paddle', 'Duplicate Paddle') }
	}
	for ($brickNumber = 0; $brickNumber -lt $brickIndices.Count; $brickNumber++)
	{
		$entityIndex = $brickIndices[$brickNumber]
		$brick = $scene.entities[$entityIndex]
		$point = $layout[$brickNumber]
		$brick.transform.position = @([float]$point.x, [float]$point.y)
		$brick.transform.rotation = 0
		$maximumDurability = switch ($level)
		{
			1 { 2 }
			2 { 2 }
			3 { 3 }
			4 { 3 }
			default { 4 }
		}
		$durability = 1 + (($brickNumber + $level - 1) % $maximumDurability)
		if ($level -eq 5 -and ($brickNumber % 11) -eq 4)
		{
			$durability = 5
		}
		$behavior = $brick.components | Where-Object { $_.type -eq 'Brick' } | Select-Object -First 1
		$behavior | Add-Member -NotePropertyName 'Hit Points' -NotePropertyValue $durability -Force
		$behavior | Add-Member -NotePropertyName 'Power' -NotePropertyValue '' -Force

		$power = ''
		for ($powerIndex = 0; $powerIndex -lt $powerPlan.Count; $powerIndex++)
		{
			$slot = [Math]::Floor(($powerIndex + 1) * $brickIndices.Count / ($powerPlan.Count + 1))
			if ($brickNumber -eq $slot) { $power = $powerPlan[$powerIndex]; break }
		}

		if ($power)
		{
			$behavior.Power = $power
			$iconTexture = switch ($power)
			{
				'Extra Life' { 'Sprites/Brickout/power-life.png' }
				'Multiball' { 'Sprites/Brickout/power-multiball.png' }
				'Bomb' { 'Sprites/Brickout/power-bomb.png' }
				'Wide Paddle' { 'Sprites/Brickout/power-wide.png' }
				'Duplicate Paddle' { 'Sprites/Brickout/power-duplicate.png' }
			}
			$icon = New-WorldSprite ("Power Icon {0:D2}" -f ($brickNumber + 1)) $iconTexture 0 0 1 1 0 $entityIndex 8
			$scene.entities = @($scene.entities) + @($icon)
		}
	}

	$arenaElements = switch ($level)
	{
		1 { @(
			(New-Rail 'Arena Rail Left' -250 -80 22),
			(New-Rail 'Arena Rail Right' 250 -80 -22)
		) }
		2 { @(
			(New-Bumper 'Arena Bumper Center' 0 -35),
			(New-Post 'Arena Post Left' -245 -105),
			(New-Post 'Arena Post Right' 245 -105)
		) }
		3 { @(
			(New-Bumper 'Arena Bumper Left' -275 -30),
			(New-Bumper 'Arena Bumper Right' 275 -30),
			(New-Rail 'Arena Rail Center' 0 -135 0)
		) }
		4 { @(
			(New-Rail 'Arena Slingshot Left' -245 -105 32),
			(New-Rail 'Arena Slingshot Right' 245 -105 -32),
			(New-Post 'Arena Post Center Left' -90 -10),
			(New-Post 'Arena Post Center Right' 90 -10)
		) }
		default { @(
			(New-Bumper 'Arena Bumper Left' -300 -15),
			(New-Bumper 'Arena Bumper Center' 0 -75),
			(New-Bumper 'Arena Bumper Right' 300 -15),
			(New-Rail 'Arena Rail Left' -245 -155 -24),
			(New-Rail 'Arena Rail Right' 245 -155 24)
		) }
	}
	$scene.entities = @($scene.entities) + $arenaElements

	if (!$hasHud)
	{
		$scene.entities = @($scene.entities) + @((New-AssemblyInstance 'Assemblies/HUD.lnassembly' $systemsIndex))
	}

	if (!$hasPauseMenu)
	{
		$scene.entities = @($scene.entities) + @((New-AssemblyInstance 'Assemblies/Pause Menu.lnassembly' $systemsIndex))
	}

	if (!$hasScreenTransition)
	{
		$scene.entities = @($scene.entities) + @((New-AssemblyInstance 'Assemblies/Screen Transition.lnassembly' $systemsIndex))
	}

	Write-SealedJson $scenePath $scene
}

$mainMenuPath = Join-Path $assetRoot 'Scenes\MainMenu.lnscene'
$mainMenuScene = Read-SealedJson $mainMenuPath
$hasGameRules = $mainMenuScene.entities | Where-Object { $_.assembly -eq 'Assemblies/Game Rules.lnassembly' }
$hasScreenTransition = $mainMenuScene.entities | Where-Object { $_.assembly -eq 'Assemblies/Screen Transition.lnassembly' }

if (!$hasGameRules)
{
	$mainMenuScene.entities = @($mainMenuScene.entities) + @((New-AssemblyInstance 'Assemblies/Game Rules.lnassembly'))
}

if (!$hasScreenTransition)
{
	$mainMenuScene.entities = @($mainMenuScene.entities) + @((New-AssemblyInstance 'Assemblies/Screen Transition.lnassembly'))
}

foreach ($entity in $mainMenuScene.entities)
{
	if ($entity.name -eq 'Camera')
	{
		$entity.components = @($entity.components | Where-Object { $_.type -ne 'PostProcessingComponent' }) + @((New-PostProcessingComponent))
	}
}

Write-SealedJson $mainMenuPath $mainMenuScene

function New-EndScene
{
	return [ordered]@{
		entities = @(
			(New-AssemblyInstance 'Assemblies/End Screen.lnassembly')
			(New-AssemblyInstance 'Assemblies/Game Rules.lnassembly')
			(New-AssemblyInstance 'Assemblies/Screen Transition.lnassembly')
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

$splashScene = [ordered]@{
	entities = @(
		[ordered]@{
			components = @([ordered]@{ type = 'SplashScreen' })
			name = 'Splash'
			parent = -1
			transform = New-Transform
		}
		(New-SpriteEntity 'Lion Engine Logo' 'Images/lion-engine-banner.png' 0.5 0.5 0 0 0.62 0.62 -1 20)
		(New-AssemblyInstance 'Assemblies/Screen Transition.lnassembly')
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
Write-SealedJson (Join-Path $assetRoot 'Scenes\Splash.lnscene') $splashScene

$inputPath = Join-Path $assetRoot 'Config\Input.lninput'
$inputMap = Read-SealedJson $inputPath
$menuActions = @(
	[ordered]@{ name='menu_up'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=265; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=11; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_axis'; code=1; scale=-1; gamepad=-1 }) },
	[ordered]@{ name='menu_down'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=264; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=13; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_axis'; code=1; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_left'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=263; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=14; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_axis'; code=0; scale=-1; gamepad=-1 }) },
	[ordered]@{ name='menu_right'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=262; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=12; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_axis'; code=0; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_confirm'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=257; scale=1; gamepad=-1 },
		[ordered]@{ device='keyboard'; code=32; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=0; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_back'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=256; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=1; scale=1; gamepad=-1 }) },
	[ordered]@{ name='player_launch'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=32; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=0; scale=1; gamepad=-1 }) },
	[ordered]@{ name='player_pause'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=256; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=7; scale=1; gamepad=-1 }) },
	[ordered]@{ name='player_power'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=69; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=2; scale=1; gamepad=-1 }) }
)

$menuNames = @($menuActions | ForEach-Object { $_.name })
$inputMap.actions = @($inputMap.actions | Where-Object { $_.name -notin $menuNames }) + $menuActions
Write-SealedJson $inputPath $inputMap

Write-Host 'Brickout scenes, assemblies and menu input actions are up to date.'
