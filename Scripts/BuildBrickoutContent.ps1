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

function New-TextComponent([string]$text, [float]$size, [int]$order = 100,
	[bool]$centered = $true, [float]$offsetX = 0, [float]$offsetY = 0)
{
	return [ordered]@{
		Text = $text
		Font = 'Fonts/Arcade.lnfont'
		Size = $size
		Spacing = 0
		Centered = $centered
		Order = $order
		'Offset.x' = $offsetX
		'Offset.y' = $offsetY
		'Offset.z' = 0
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

function New-CheckBoxComponent([bool]$checked = $false)
{
	return [ordered]@{
		Background = 'Sprites/UI/checkbox-frame.png'
		Checkmark = 'Sprites/UI/checkbox-check.png'
		'Size.x' = 30; 'Size.y' = 30; 'Size.z' = 0
		'Normal Color.x' = 0.12; 'Normal Color.y' = 0.82; 'Normal Color.z' = 1
		'Hovered Color.x' = 1; 'Hovered Color.y' = 0.22; 'Hovered Color.z' = 0.72
		'Checkmark Color.x' = 1; 'Checkmark Color.y' = 1; 'Checkmark Color.z' = 1
		Order = 105; Checked = $checked; Interactable = $true; type = 'CheckBox'
	}
}

function New-ProgressBarComponent([float]$value)
{
	return [ordered]@{
		Background = 'Sprites/UI/progress-track.png'; Fill = 'Sprites/UI/progress-fill.png'
		'Size.x' = 300; 'Size.y' = 22; 'Size.z' = 0
		'Background Color.x' = 0.08; 'Background Color.y' = 0.28; 'Background Color.z' = 0.42
		'Fill Color.x' = 0.12; 'Fill Color.y' = 0.82; 'Fill Color.z' = 1
		Minimum = 0; Maximum = 10; Value = $value; Order = 105; Interactable = $true
		type = 'ProgressBar'
	}
}

function New-ComboBoxComponent([string]$prefix, [string]$options, [int]$selected = 0)
{
	return [ordered]@{
		Options = $options; Prefix = $prefix; Font = 'Fonts/Arcade.lnfont'
		'Option Height' = 48; 'Selected Index' = $selected; 'Popup Order' = 180
		type = 'ComboBox'
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
	[float]$anchorY, [float]$offsetX, [float]$offsetY, [int]$parent, [bool]$visible = $true,
	[bool]$centered = $true)
{
	$entity = [ordered]@{
		components = @(
			(New-TextComponent $text $size 100 $centered)
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
					'Shake Strength' = 0.42
					type = 'GameRules'
				}
				[ordered]@{
					Texture = 'Sprites/Brickout/particle.png'
					'Max Particles' = 720
					'Emission Rate' = 0
					Lifetime = 0.52
					Speed = 215
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
		[ordered]@{
			components = @([ordered]@{
				Texture = 'Sprites/Brickout/particle.png'
				'Max Particles' = 920
				'Emission Rate' = 0
				Lifetime = 0.68
				Speed = 165
				Direction = 90
				Spread = 360
				'Start Size' = 20
				'End Size' = 1
				'Start Color.x' = 1
				'Start Color.y' = 0.2
				'Start Color.z' = 0.82
				'End Color.x' = 0.05
				'End Color.y' = 0.92
				'End Color.z' = 1
				Order = 72
				type = 'ParticleComponent'
			})
			name = 'Shockwave Overdrive Particles'
			parent = 0
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
		(New-TextEntity 'Shockwave Text' 'SHOCKWAVE 0/16' 15 0.5 1 248 -34 0)
		(New-TextEntity 'Attempts Text' 'BALLS 3' 20 1 1 -88 -34 0)
		(New-TextEntity 'Power Text' 'MULTIBALL x3' 22 0.5 1 0 -92 0 $false)
		(New-GroupEntity 'Power Timer' 0)
		(New-SpriteEntity 'Power Timer Ring' 'Sprites/Brickout/power-timer-11.png' 1 1 -72 -104 0.72 0.72 8 96)
		(New-SpriteEntity 'Power Timer Icon' 'Sprites/Brickout/power-wide.png' 1 1 -72 -104 0.46 0.46 8 97)
		(New-TextEntity 'Power Timer Text' '12.0s' 11 1 1 -72 -151 8)
	)
	root = 0
}
$hudPromptIndex = $hudAssembly.entities.Count
$hudAssembly.entities = @($hudAssembly.entities) + @(
	(New-GroupEntity 'HUD Controller Prompts' 0),
	(New-SpriteEntity 'HUD Move Icon' 'Sprites/UI/controller-stick.png' 0.5 0 -285 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Move Label' 'MOVE' 12 0.5 0 -214 27 $hudPromptIndex),
	(New-SpriteEntity 'HUD Launch Icon' 'Sprites/UI/controller-a.png' 0.5 0 -130 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Launch Label' 'LAUNCH' 12 0.5 0 -54 27 $hudPromptIndex),
	(New-SpriteEntity 'HUD Power Icon' 'Sprites/UI/controller-x.png' 0.5 0 50 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Power Label' 'WAVE' 12 0.5 0 118 27 $hudPromptIndex),
	(New-SpriteEntity 'HUD Pause Icon' 'Sprites/UI/controller-start.png' 0.5 0 215 28 1 1 $hudPromptIndex),
	(New-TextEntity 'HUD Pause Label' 'PAUSE' 12 0.5 0 302 27 $hudPromptIndex)
)
$hudKeyboardPromptIndex = $hudAssembly.entities.Count
$hudAssembly.entities = @($hudAssembly.entities) + @(
	(New-GroupEntity 'HUD Keyboard Prompts' 0),
	(New-SpriteEntity 'HUD Keyboard Move Icon' 'Sprites/UI/key-arrows.png' 0.5 0 -292 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Move Label' 'MOVE' 12 0.5 0 -198 27 $hudKeyboardPromptIndex),
	(New-SpriteEntity 'HUD Keyboard Launch Icon' 'Sprites/UI/key-space.png' 0.5 0 -105 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Launch Label' 'LAUNCH' 12 0.5 0 -12 27 $hudKeyboardPromptIndex),
	(New-SpriteEntity 'HUD Keyboard Power Icon' 'Sprites/UI/key-e.png' 0.5 0 82 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Power Label' 'WAVE' 12 0.5 0 150 27 $hudKeyboardPromptIndex),
	(New-SpriteEntity 'HUD Keyboard Pause Icon' 'Sprites/UI/key-esc.png' 0.5 0 230 28 1 1 $hudKeyboardPromptIndex),
	(New-TextEntity 'HUD Keyboard Pause Label' 'PAUSE' 12 0.5 0 306 27 $hudKeyboardPromptIndex)
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
		(New-PanelEntity 'Menu Panel' 860 690 0.5 0.5 0 0 0 70)
		(New-SpriteEntity 'Menu Title' 'Sprites/UI/brickout-title.png' 0.5 0.5 0 280 0.8 0.8 0 100)
		(New-TextEntity 'Menu Subtitle' 'NEON CIRCUIT' 18 0.5 0.5 0 222 0)
		(New-TextEntity 'Menu Prompt' 'PRESS TO START' 22 0.5 0.5 0 -20 0)
		(New-GroupEntity 'Menu Options' 0)
		(New-ButtonEntity 'Continue Button' 'CONTINUE' 0.5 0.5 0 145 6 540 42 20)
		(New-ButtonEntity 'Level Select Button' 'LEVEL SELECT' 0.5 0.5 0 97 6 540 42 20)
		(New-ButtonEntity 'Achievements Button' 'ACHIEVEMENTS' 0.5 0.5 0 49 6 540 42 20)
		(New-ButtonEntity 'Statistics Button' 'STATISTICS' 0.5 0.5 0 1 6 540 42 20)
		(New-ButtonEntity 'Credits Button' 'CREDITS' 0.5 0.5 0 -47 6 540 42 20)
		(New-ButtonEntity 'Settings Button' 'SETTINGS' 0.5 0.5 0 -95 6 540 42 20)
		(New-ButtonEntity 'Quit Button' 'QUIT' 0.5 0.5 0 -143 6 540 42 20)
		(New-TextEntity 'Menu Detail' 'CREDITS' 23 0.5 0.5 0 166 0 $false)
		(New-GroupEntity 'Settings Options' 0)
		(New-GroupEntity 'Level Options' 0)
		(New-GroupEntity 'Statistics Panel' 0)
		(New-GroupEntity 'Achievements Panel' 0)
		(New-GroupEntity 'Credits Panel' 0)
		(New-ButtonEntity 'Back Button' 'BACK' 0.5 0.5 0 -270 0 660 42 18 $false)
	)
	root = 0
}
$settingsOptionsIndex = 15
$levelOptionsIndex = 16
$statisticsIndex = 17
$achievementsIndex = 18
$creditsIndex = 19

$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-TextEntity 'Settings Tab 1' 'GRAPHICS' 13 0.5 0.5 -270 126 $settingsOptionsIndex),
	(New-TextEntity 'Settings Tab 2' 'SOUND' 13 0.5 0.5 -90 126 $settingsOptionsIndex),
	(New-TextEntity 'Settings Tab 3' 'ACCESSIBILITY' 13 0.5 0.5 105 126 $settingsOptionsIndex),
	(New-TextEntity 'Settings Tab 4' 'CONTROLS' 13 0.5 0.5 285 126 $settingsOptionsIndex),
	(New-TextEntity 'Settings Page' 'PAGE 1/4' 11 0.5 0.5 0 94 $settingsOptionsIndex)
)

$graphicsIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @((New-GroupEntity 'Graphics Settings' $settingsOptionsIndex))
$graphicsRows = @(
	@{ Name='Resolution Button'; Label='RESOLUTION'; Y=55; Combo='960 X 540|1280 X 720|1600 X 900|1920 X 1080'; Selected=1 },
	@{ Name='VSync Button'; Label='V-SYNC'; Y=7; Check='VSync CheckBox' },
	@{ Name='Quality Button'; Label='GRAPHICS'; Y=-41; Combo='LOW|MEDIUM|HIGH'; Selected=2 },
	@{ Name='Bloom Button'; Label='BLOOM'; Y=-89; Check='Bloom CheckBox' },
	@{ Name='Vignette Button'; Label='VIGNETTE'; Y=-137; Check='Vignette CheckBox' },
	@{ Name='Motion Blur Button'; Label='MOTION BLUR'; Y=-185; Check='Motion Blur CheckBox' }
)
foreach ($row in $graphicsRows)
{
	$button = New-ButtonEntity $row.Name $row.Label 0.5 0.5 0 $row.Y $graphicsIndex 660 40 15
	$text = $button.components | Where-Object { $_.type -eq 'TextRenderer' } | Select-Object -First 1
	$text.Centered = $false; $text.'Offset.x' = -292
	if ($row.Combo) { $button.components = @($button.components) + @((New-ComboBoxComponent $row.Label $row.Combo $row.Selected)) }
	$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @($button)
	if ($row.Check)
	{
		$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @([ordered]@{
			components = @((New-CheckBoxComponent $true), (New-AnchorComponent 0.5 0.5 282 $row.Y))
			name = $row.Check; parent = $graphicsIndex; transform = New-Transform
		})
	}
}
$soundIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Sound Settings' $settingsOptionsIndex),
	(New-ButtonEntity 'SFX Volume Button' 'SFX 80%' 0.5 0.5 0 40 $soundIndex 660 56 16),
	(New-ButtonEntity 'Music Volume Button' 'MUSIC 60%' 0.5 0.5 0 -36 $soundIndex 660 56 16),
	[ordered]@{ components=@((New-ProgressBarComponent 8), (New-AnchorComponent 0.5 0.5 120 40)); name='SFX Progress'; parent=$soundIndex; transform=New-Transform },
	[ordered]@{ components=@((New-ProgressBarComponent 6), (New-AnchorComponent 0.5 0.5 120 -36)); name='Music Progress'; parent=$soundIndex; transform=New-Transform }
)
foreach ($name in @('SFX Volume Button', 'Music Volume Button'))
{
	$button = $mainMenuAssembly.entities | Where-Object { $_.name -eq $name } | Select-Object -First 1
	$text = $button.components | Where-Object { $_.type -eq 'TextRenderer' } | Select-Object -First 1
	$text.Centered = $false; $text.'Offset.x' = -292
}
$accessibilityIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @((New-GroupEntity 'Accessibility Settings' $settingsOptionsIndex))
$cameraButton = New-ButtonEntity 'Camera Shake Button' 'CAMERA SHAKE' 0.5 0.5 0 48 $accessibilityIndex 660 46 16
$cameraText = $cameraButton.components | Where-Object { $_.type -eq 'TextRenderer' } | Select-Object -First 1
$cameraText.Centered = $false; $cameraText.'Offset.x' = -292
$colorButton = New-ButtonEntity 'Color Mode Button' 'COLOR MODE' 0.5 0.5 0 -10 $accessibilityIndex 660 46 15
$colorText = $colorButton.components | Where-Object { $_.type -eq 'TextRenderer' } | Select-Object -First 1
$colorText.Centered = $false; $colorText.'Offset.x' = -292
$colorButton.components = @($colorButton.components) + @((New-ComboBoxComponent 'COLOR MODE' 'NONE|PROTANOPIA|DEUTERANOPIA|TRITANOPIA' 0))
$languageButton = New-ButtonEntity 'Language Button' 'LANGUAGE' 0.5 0.5 0 -68 $accessibilityIndex 660 46 15
$languageText = $languageButton.components | Where-Object { $_.type -eq 'TextRenderer' } | Select-Object -First 1
$languageText.Centered = $false; $languageText.'Offset.x' = -292
$languageButton.components = @($languageButton.components) + @((New-ComboBoxComponent 'LANGUAGE' 'ENGLISH|PORTUGUES|ESPANOL|ITALIANO|FRANCAIS|DEUTSCH|RUSSIAN|GREEK' 0))
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	$cameraButton, $colorButton, $languageButton,
	[ordered]@{ components=@((New-CheckBoxComponent $true), (New-AnchorComponent 0.5 0.5 282 48)); name='Camera Shake CheckBox'; parent=$accessibilityIndex; transform=New-Transform }
)
$controlsIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @((New-GroupEntity 'Controls Settings' $settingsOptionsIndex))
$controlButton = New-ButtonEntity 'Control Hints Button' 'CONTROL HINTS' 0.5 0.5 0 45 $controlsIndex 660 48 16
$controlText = $controlButton.components | Where-Object { $_.type -eq 'TextRenderer' } | Select-Object -First 1
$controlText.Centered = $false; $controlText.'Offset.x' = -292
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	$controlButton,
	[ordered]@{ components=@((New-CheckBoxComponent $true), (New-AnchorComponent 0.5 0.5 282 45)); name='Control Hints CheckBox'; parent=$controlsIndex; transform=New-Transform },
	(New-TextEntity 'Controls Help' "ARROWS / LEFT STICK    MOVE`nSPACE / A              LAUNCH`nE / X                  SHOCKWAVE`nESC / START            PAUSE" 14 0.5 0.5 -285 -35 $controlsIndex $true $false)
)

for ($slot = 0; $slot -lt 10; $slot++)
{
	$column = $slot % 2
	$row = [Math]::Floor($slot / 2)
	$x = if ($column -eq 0) { -178 } else { 178 }
	$y = 82 - $row * 58
	$slotName = "Level Slot {0} Button" -f ($slot + 1)
	$slotLabel = "LEVEL {0:D3}" -f ($slot + 1)
	$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
		(New-ButtonEntity $slotName $slotLabel 0.5 0.5 $x $y $levelOptionsIndex 330 48 12)
	)
}
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-TextEntity 'Level Page' 'PAGE 01/10' 13 0.5 0.5 0 -210 $levelOptionsIndex),
	(New-TextEntity 'Statistics Text' 'STATISTICS' 18 0.5 0.5 -270 95 $statisticsIndex $true $false),
	(New-TextEntity 'Achievement Progress' '0/10 UNLOCKED' 13 0.5 0.5 0 -202 $achievementsIndex),
	(New-SpriteEntity 'Credits Logo' 'Images/sampaio-games-logo.png' 0.5 0.5 0 78 0.085 0.085 $creditsIndex 100),
	(New-TextEntity 'Credits Creator' "CREATED BY`nKELLVYN SAMPAIO" 21 0.5 0.5 0 -50 $creditsIndex),
	(New-TextEntity 'Credits Technology' "POWERED BY LION ENGINE`nA SAMPAIO GAMES PRODUCTION" 14 0.5 0.5 0 -142 $creditsIndex)
)
for ($row = 0; $row -lt 5; $row++)
{
	$rowIndex = $mainMenuAssembly.entities.Count
	$y = 92 - $row * 58
	$rowName = "Achievement Row {0}" -f ($row + 1)
	$iconName = "Achievement Icon {0}" -f ($row + 1)
	$textName = "Achievement Text {0}" -f ($row + 1)
	$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
		(New-GroupEntity $rowName $achievementsIndex $true),
		(New-SpriteEntity $iconName 'Sprites/Brickout/achievement-first.png' 0.5 0.5 -285 $y 0.58 0.58 $rowIndex 100),
		(New-TextEntity $textName 'ACHIEVEMENT' 13 0.5 0.5 -238 ($y + 6) $rowIndex $true $false)
	)
}
$attractPromptIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Attract Prompt' 0),
	(New-SpriteEntity 'Controller Start Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 -125 -20 1 1 $attractPromptIndex),
	(New-TextEntity 'Controller Start Label' 'PRESS TO START' 20 0.5 0.5 50 -21 $attractPromptIndex)
)
$keyboardAttractIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Attract Prompt' 0),
	(New-SpriteEntity 'Keyboard Start Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 -125 -20 1 1 $keyboardAttractIndex),
	(New-TextEntity 'Keyboard Start Label' 'PRESS TO START' 20 0.5 0.5 50 -21 $keyboardAttractIndex)
)
$menuPromptIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Menu Prompts' 0),
	(New-SpriteEntity 'Controller Navigate Icon' 'Sprites/UI/controller-dpad.png' 0.5 0.5 -180 -300 1 1 $menuPromptIndex),
	(New-TextEntity 'Controller Navigate Label' 'NAVIGATE' 13 0.5 0.5 -98 -301 $menuPromptIndex),
	(New-SpriteEntity 'Controller Select Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 45 -300 1 1 $menuPromptIndex),
	(New-TextEntity 'Controller Select Label' 'SELECT' 13 0.5 0.5 122 -301 $menuPromptIndex)
)
$keyboardMenuIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Menu Prompts' 0),
	(New-SpriteEntity 'Keyboard Navigate Icon' 'Sprites/UI/key-arrows.png' 0.5 0.5 -180 -300 1 1 $keyboardMenuIndex),
	(New-TextEntity 'Keyboard Navigate Label' 'NAVIGATE' 13 0.5 0.5 -88 -301 $keyboardMenuIndex),
	(New-SpriteEntity 'Keyboard Select Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 52 -300 1 1 $keyboardMenuIndex),
	(New-TextEntity 'Keyboard Select Label' 'SELECT' 13 0.5 0.5 132 -301 $keyboardMenuIndex)
)
$keyboardDetailIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Detail Prompts' 0),
	(New-SpriteEntity 'Keyboard Back Icon' 'Sprites/UI/key-esc.png' 0.5 0.5 -278 -270 1 1 $keyboardDetailIndex)
)
$controllerDetailIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Detail Prompts' 0),
	(New-SpriteEntity 'Controller Back Icon' 'Sprites/UI/controller-b.png' 0.5 0.5 -278 -270 1 1 $controllerDetailIndex)
)
$keyboardPageIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Settings Prompt' 0),
	(New-SpriteEntity 'Keyboard Previous Tab Icon' 'Sprites/UI/key-q.png' 0.5 0.5 -355 94 1 1 $keyboardPageIndex),
	(New-SpriteEntity 'Keyboard Next Tab Icon' 'Sprites/UI/key-e.png' 0.5 0.5 355 94 1 1 $keyboardPageIndex)
)
$controllerPageIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Settings Prompt' 0),
	(New-SpriteEntity 'Controller Previous Tab Icon' 'Sprites/UI/controller-lb.png' 0.5 0.5 -355 94 1 1 $controllerPageIndex),
	(New-SpriteEntity 'Controller Next Tab Icon' 'Sprites/UI/controller-rb.png' 0.5 0.5 355 94 1 1 $controllerPageIndex)
)
$keyboardPagingIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Keyboard Page Prompt' 0),
	(New-SpriteEntity 'Keyboard Previous Page Icon' 'Sprites/UI/key-q.png' 0.5 0.5 -355 -214 1 1 $keyboardPagingIndex),
	(New-SpriteEntity 'Keyboard Next Page Icon' 'Sprites/UI/key-e.png' 0.5 0.5 355 -214 1 1 $keyboardPagingIndex)
)
$controllerPagingIndex = $mainMenuAssembly.entities.Count
$mainMenuAssembly.entities = @($mainMenuAssembly.entities) + @(
	(New-GroupEntity 'Controller Page Prompt' 0),
	(New-SpriteEntity 'Controller Previous Page Icon' 'Sprites/UI/controller-lb.png' 0.5 0.5 -355 -214 1 1 $controllerPagingIndex),
	(New-SpriteEntity 'Controller Next Page Icon' 'Sprites/UI/controller-rb.png' 0.5 0.5 355 -214 1 1 $controllerPagingIndex)
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
	(New-TextEntity 'End Navigate Label' 'NAVIGATE' 13 0.5 0.5 -98 -237 $endPromptIndex),
	(New-SpriteEntity 'End Select Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 35 -236 1 1 $endPromptIndex),
	(New-TextEntity 'End Select Label' 'SELECT' 13 0.5 0.5 110 -237 $endPromptIndex),
	(New-SpriteEntity 'End Back Icon' 'Sprites/UI/controller-b.png' 0.5 0.5 165 -236 1 1 $endPromptIndex),
	(New-TextEntity 'End Back Label' 'MENU' 13 0.5 0.5 232 -237 $endPromptIndex)
)
$endKeyboardPromptIndex = $endScreenAssembly.entities.Count
$endScreenAssembly.entities = @($endScreenAssembly.entities) + @(
	(New-GroupEntity 'End Keyboard Prompts' 0),
	(New-SpriteEntity 'End Keyboard Navigate Icon' 'Sprites/UI/key-arrows.png' 0.5 0.5 -180 -236 1 1 $endKeyboardPromptIndex),
	(New-TextEntity 'End Keyboard Navigate Label' 'NAVIGATE' 13 0.5 0.5 -86 -237 $endKeyboardPromptIndex),
	(New-SpriteEntity 'End Keyboard Select Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 55 -236 1 1 $endKeyboardPromptIndex),
	(New-TextEntity 'End Keyboard Select Label' 'SELECT' 13 0.5 0.5 134 -237 $endKeyboardPromptIndex),
	(New-SpriteEntity 'End Keyboard Back Icon' 'Sprites/UI/key-esc.png' 0.5 0.5 195 -236 1 1 $endKeyboardPromptIndex),
	(New-TextEntity 'End Keyboard Back Label' 'MENU' 13 0.5 0.5 264 -237 $endKeyboardPromptIndex)
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
	(New-TextEntity 'Pause Navigate Label' 'NAVIGATE' 13 0.5 0.5 -83 -179 $pausePromptIndex),
	(New-SpriteEntity 'Pause Select Icon' 'Sprites/UI/controller-a.png' 0.5 0.5 60 -178 1 1 $pausePromptIndex),
	(New-TextEntity 'Pause Select Label' 'SELECT' 13 0.5 0.5 134 -179 $pausePromptIndex)
)
$pauseKeyboardPromptIndex = $pauseAssembly.entities.Count
$pauseAssembly.entities = @($pauseAssembly.entities) + @(
	(New-GroupEntity 'Pause Keyboard Prompts' 1),
	(New-SpriteEntity 'Pause Keyboard Navigate Icon' 'Sprites/UI/key-arrows.png' 0.5 0.5 -175 -178 1 1 $pauseKeyboardPromptIndex),
	(New-TextEntity 'Pause Keyboard Navigate Label' 'NAVIGATE' 13 0.5 0.5 -81 -179 $pauseKeyboardPromptIndex),
	(New-SpriteEntity 'Pause Keyboard Select Icon' 'Sprites/UI/key-enter.png' 0.5 0.5 68 -178 1 1 $pauseKeyboardPromptIndex),
	(New-TextEntity 'Pause Keyboard Select Label' 'SELECT' 13 0.5 0.5 148 -179 $pauseKeyboardPromptIndex)
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
	$paddleBehavior.'Horizontal Limit' = 338
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
		$family = ($level - 1) % 10
		for ($index = 0; $index -lt $count; $index++)
		{
			$row = [Math]::Floor($index / 10)
			$column = $index % 10
			$x = -333 + $column * 74
			$y = 220 - $row * 58
			switch ($family)
			{
				0 { $y += [Math]::Sin(($column + $level * 0.3) * 0.85) * 42 }
				1 { $y -= [Math]::Abs($column - 4.5) * 14; $x += ($row % 2) * 18 }
				2 { $y += [Math]::Abs($column - 4.5) * 13 - 38 }
				3 { $x += [Math]::Sin($row * 1.4) * 34; $y += [Math]::Cos($column * 0.8) * 22 }
				4 { $y += (($column + $row) % 2) * 25 - 12 }
				5 { if (($row % 2) -eq 0) { $x -= 24 } else { $x += 24 }; $y += ($column % 3) * 10 }
				6 { $x += ($row % 2) * 20 - 10; $y += [Math]::Abs($column - 4.5) * 5 }
				7 { $y += [Math]::Sin($column * 1.2) * 25; $x += [Math]::Cos($row * 1.8) * 22 }
				8 { $y += [Math]::Sin(($column + $row) * 0.72) * 36 }
				9 { $x += ($row - 1.5) * 12; $y -= [Math]::Abs($column - 4.5) * 4 }
			}
			$layout.Add([pscustomobject]@{ x=$x; y=[Math]::Round($y, 1); rotation=0 })
		}
	}

	return @($layout)
}

function Get-LevelPowerPlan([int]$level)
{
	switch (($level - 1) % 5)
	{
		0 { @('Extra Life', 'Wide Paddle') }
		1 { @('Bomb', 'Multiball') }
		2 { @('Extra Life', 'Piercing Ball', 'Bomb') }
		3 { @('Multiball', 'Wide Paddle', 'Bomb') }
		default { @('Extra Life', 'Multiball', 'Bomb', 'Wide Paddle', 'Piercing Ball') }
	}
}

function ConvertTo-SymmetricLayout([object[]]$layout, [int]$count)
{
	$pairCount = [Math]::Floor($count / 2)
	$candidates = [Collections.Generic.List[object]]::new()
	$centerCandidates = [Collections.Generic.List[object]]::new()
	$seen = [Collections.Generic.HashSet[string]]::new()

	foreach ($point in $layout)
	{
		$absoluteX = [Math]::Round([Math]::Abs([double]$point.x), 1)
		if ($absoluteX -lt 35)
		{
			$centerCandidates.Add($point)
			continue
		}

		$key = '{0:F1}|{1:F1}' -f $absoluteX, [double]$point.y
		$overlaps = $candidates | Where-Object {
			[Math]::Abs([double]$_.x - $absoluteX) -lt 66 -and
			[Math]::Abs([double]$_.y - [double]$point.y) -lt 28
		} | Select-Object -First 1
		if (!$seen.Contains($key) -and !$overlaps)
		{
			[void]$seen.Add($key)
			$candidates.Add([pscustomobject]@{
				x=$absoluteX; y=[double]$point.y; rotation=[Math]::Abs([double]$point.rotation)
			})
		}
	}

	$fallback = 0
	while ($candidates.Count -lt $pairCount)
	{
		$column = $fallback % 5
		$row = [Math]::Floor($fallback / 5)
		$fallback++
		$x = 70 + $column * 66
		$y = 220 - $row * 54
		$overlaps = $candidates | Where-Object {
			[Math]::Abs([double]$_.x - $x) -lt 66 -and [Math]::Abs([double]$_.y - $y) -lt 28
		} | Select-Object -First 1
		if (!$overlaps)
		{
			$candidates.Add([pscustomobject]@{ x=$x; y=$y; rotation=0 })
		}
	}

	$symmetric = [Collections.Generic.List[object]]::new()
	for ($index = 0; $index -lt $pairCount; $index++)
	{
		$point = $candidates[$index]
		$symmetric.Add([pscustomobject]@{ x=-[double]$point.x; y=[double]$point.y; rotation=-[double]$point.rotation })
		$symmetric.Add([pscustomobject]@{ x= [double]$point.x; y=[double]$point.y; rotation= [double]$point.rotation })
	}

	if (($count % 2) -ne 0)
	{
		$centerY = if ($centerCandidates.Count -gt 0) { [double]$centerCandidates[0].y } else { 235.0 }
		$symmetric.Add([pscustomobject]@{ x=0; y=$centerY; rotation=0 })
	}

	return @($symmetric)
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

$baseScenes = @(1..5 | ForEach-Object {
	Read-SealedJson (Join-Path $assetRoot ("Scenes\Level{0:D2}.lnscene" -f $_))
})

$arenaSignatures = [Collections.Generic.HashSet[string]]::new()
for ($level = 1; $level -le 100; $level++)
{
	$theme = [int]([Math]::Floor(($level - 1) / 5) + 1)
	$scenePath = Join-Path $assetRoot ("Scenes\Level{0:D2}.lnscene" -f $level)
	if ($level -le 5)
	{
		$scene = $baseScenes[$level - 1]
	}
	else
	{
		$template = $baseScenes[($level - 1) % $baseScenes.Count]
		$scene = ($template | ConvertTo-Json -Depth 100) | ConvertFrom-Json
	}
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

		if ($entity.name -eq 'Background' -and $sprite)
		{
			$sprite.texture = "Sprites/Brickout/background-theme-{0:D2}.png" -f $theme
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
				$ballBehavior | Add-Member -NotePropertyName 'Minimum Horizontal Ratio' -NotePropertyValue 0.2 -Force
			}

			if (!($entity.components | Where-Object { $_.type -eq 'ParticleComponent' }))
			{
				$entity.components = @($entity.components) + @([ordered]@{
					Texture='Sprites/Brickout/particle.png'; 'Max Particles'=180; 'Emission Rate'=72
					Lifetime=0.28; Speed=38; Direction=180; Spread=42; 'Start Size'=9; 'End Size'=2
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

	$layout = ConvertTo-SymmetricLayout (Get-LevelLayout $level $brickIndices.Count) $brickIndices.Count
	$powerPlan = @(Get-LevelPowerPlan $level)
	$powerAssignments = @{}
	$mirroredPairCount = [Math]::Floor($brickIndices.Count / 2)
	for ($powerIndex = 0; $powerIndex -lt $powerPlan.Count; $powerIndex++)
	{
		$slot = [Math]::Floor(($powerIndex + 1) * $mirroredPairCount / ($powerPlan.Count + 1))
		$side = ($level + $powerIndex) % 2
		$powerAssignments[[int]($slot * 2 + $side)] = $powerPlan[$powerIndex]
	}
	for ($brickNumber = 0; $brickNumber -lt $brickIndices.Count; $brickNumber++)
	{
		$entityIndex = $brickIndices[$brickNumber]
		$brick = $scene.entities[$entityIndex]
		$point = $layout[$brickNumber]
		$brick.transform.position = @([float]$point.x, [float]$point.y)
		$brick.transform.rotation = 0
		$symmetricSlot = [Math]::Floor($brickNumber / 2)
		$durability = 1 + (($symmetricSlot + $level) % 2)
		if ($level -ge 2 -and (($symmetricSlot + $level) % 7) -eq 0) { $durability = 3 }
		if ($level -ge 4 -and (($symmetricSlot * 3 + $level) % 11) -eq 0) { $durability = 4 }
		if ($level -ge 6 -and (($symmetricSlot * 5 + $level) % 17) -eq 0) { $durability = 5 }
		if ($level -ge 25 -and (($symmetricSlot + $level) % 5) -eq 0) { $durability = [Math]::Max($durability, 3) }
		if ($level -ge 50 -and (($symmetricSlot + $level) % 4) -eq 0) { $durability = [Math]::Max($durability, 4) }
		$behavior = $brick.components | Where-Object { $_.type -eq 'Brick' } | Select-Object -First 1
		$behavior | Add-Member -NotePropertyName 'Hit Points' -NotePropertyValue $durability -Force
		$behavior | Add-Member -NotePropertyName 'Power' -NotePropertyValue '' -Force

		$power = if ($powerAssignments.ContainsKey($brickNumber)) { $powerAssignments[$brickNumber] } else { '' }

		if ($power)
		{
			$behavior.Power = $power
			$iconTexture = switch ($power)
			{
				'Extra Life' { 'Sprites/Brickout/power-life.png' }
				'Multiball' { 'Sprites/Brickout/power-multiball.png' }
				'Bomb' { 'Sprites/Brickout/power-bomb.png' }
				'Wide Paddle' { 'Sprites/Brickout/power-wide.png' }
				'Piercing Ball' { 'Sprites/Brickout/power-piercing.png' }
			}
			$icon = New-WorldSprite ("Power Icon {0:D2}" -f ($brickNumber + 1)) $iconTexture 0 0 0.42 0.42 0 $entityIndex 8
			$scene.entities = @($scene.entities) + @($icon)
		}
	}

	$arenaElements = switch (($level - 1) % 20)
	{
		0 { @(
			(New-Rail 'Arena Rail Left' -285 25 28),
			(New-Rail 'Arena Rail Right' 285 25 -28)
		) }
		1 { @(
			(New-Bumper 'Arena Bumper Left' -250 40),
			(New-Bumper 'Arena Bumper Right' 250 40)
		) }
		2 { @(
			(New-Post 'Arena Post Left' -300 85),
			(New-Post 'Arena Post Right' 300 85),
			(New-Bumper 'Arena Bumper Center' 0 55)
		) }
		3 { @(
			(New-Rail 'Arena Slingshot Left' -260 50 34),
			(New-Rail 'Arena Slingshot Right' 260 50 -34)
		) }
		4 { @(
			(New-Bumper 'Arena Bumper Left' -285 95),
			(New-Bumper 'Arena Bumper Center' 0 35),
			(New-Bumper 'Arena Bumper Right' 285 95)
		) }
		5 { @(
			(New-Post 'Arena Post Left' -220 35),
			(New-Post 'Arena Post Right' 220 35)
		) }
		6 { @(
			(New-Rail 'Arena Rail Left' -285 90 -24),
			(New-Rail 'Arena Rail Right' 285 90 24),
			(New-Bumper 'Arena Bumper Center' 0 25)
		) }
		7 { @(
			(New-Bumper 'Arena Bumper Left' -310 35),
			(New-Bumper 'Arena Bumper Right' 310 35),
			(New-Post 'Arena Post Center' 0 105)
		) }
		8 { @(
			(New-Rail 'Arena Rail Left' -230 25 18),
			(New-Rail 'Arena Rail Right' 230 25 -18),
			(New-Post 'Arena Post Left' -80 85),
			(New-Post 'Arena Post Right' 80 85)
		) }
		9 { @(
			(New-Bumper 'Arena Bumper Left' -300 75),
			(New-Bumper 'Arena Bumper Right' 300 75)
		) }
		10 { @(
			(New-Rail 'Arena Chevron Left' -175 55 -24),
			(New-Rail 'Arena Chevron Right' 175 55 24),
			(New-Post 'Arena Post Center' 0 125)
		) }
		11 { @(
			(New-Bumper 'Arena Bumper Inner Left' -145 45),
			(New-Bumper 'Arena Bumper Inner Right' 145 45),
			(New-Post 'Arena Post Outer Left' -315 125),
			(New-Post 'Arena Post Outer Right' 315 125)
		) }
		12 { @(
			(New-Rail 'Arena Funnel Left' -250 110 -32),
			(New-Rail 'Arena Funnel Right' 250 110 32),
			(New-Bumper 'Arena Bumper Center' 0 45)
		) }
		13 { @(
			(New-Post 'Arena Post Left' -275 45),
			(New-Post 'Arena Post Center' 0 115),
			(New-Post 'Arena Post Right' 275 45)
		) }
		14 { @(
			(New-Rail 'Arena Zig Left' -305 45 42),
			(New-Rail 'Arena Zig Center' 0 110 0),
			(New-Rail 'Arena Zig Right' 305 45 -42)
		) }
		15 { @(
			(New-Bumper 'Arena Diamond Top' 0 145),
			(New-Post 'Arena Diamond Left' -125 55),
			(New-Post 'Arena Diamond Right' 125 55)
		) }
		16 { @(
			(New-Rail 'Arena Gate Left' -315 105 -18),
			(New-Rail 'Arena Gate Right' 315 105 18),
			(New-Bumper 'Arena Gate Left Bumper' -105 35),
			(New-Bumper 'Arena Gate Right Bumper' 105 35)
		) }
		17 { @(
			(New-Post 'Arena Orbit Left' -325 55),
			(New-Bumper 'Arena Orbit Center' 0 95),
			(New-Post 'Arena Orbit Right' 325 55)
		) }
		18 { @(
			(New-Rail 'Arena Crown Left' -205 65 20),
			(New-Rail 'Arena Crown Center' 0 125 0),
			(New-Rail 'Arena Crown Right' 205 65 -20)
		) }
		default { @(
			(New-Bumper 'Arena Cascade Left' -250 125),
			(New-Post 'Arena Cascade Center' 0 80),
			(New-Bumper 'Arena Cascade Right' 250 125)
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
	[ordered]@{ name='menu_tab_left'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=81; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=4; scale=1; gamepad=-1 }) },
	[ordered]@{ name='menu_tab_right'; deadzone=0.2; bindings=@(
		[ordered]@{ device='keyboard'; code=69; scale=1; gamepad=-1 },
		[ordered]@{ device='gamepad_button'; code=5; scale=1; gamepad=-1 }) },
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

for ($level = 1; $level -le 100; $level++)
{
	$scenePath = Join-Path $assetRoot ("Scenes\Level{0:D2}.lnscene" -f $level)
	if (!(Test-Path -LiteralPath $scenePath))
	{
		throw "Missing generated Brickout level: $scenePath"
	}

	$scene = Read-SealedJson $scenePath
	$expectedTheme = "Sprites/Brickout/background-theme-{0:D2}.png" -f ([int]([Math]::Floor(($level - 1) / 5) + 1))
	$background = $scene.entities | Where-Object { $_.name -eq 'Background' } | Select-Object -First 1
	$backgroundSprite = $background.components | Where-Object { $_.type -eq 'SpriteRenderer' } | Select-Object -First 1
	if (!$backgroundSprite -or $backgroundSprite.texture -ne $expectedTheme)
	{
		throw "Brickout level $level does not use its expected five-level theme."
	}
	$bricks = @($scene.entities | Where-Object { $_.components | Where-Object { $_.type -eq 'Brick' } })
	if ($bricks.Count -eq 0)
	{
		throw "Brickout level $level has no gameplay bricks."
	}
	for ($left = 0; $left -lt $bricks.Count; $left++)
	{
		for ($right = $left + 1; $right -lt $bricks.Count; $right++)
		{
			$deltaX = [Math]::Abs($bricks[$left].transform.position[0] - $bricks[$right].transform.position[0])
			$deltaY = [Math]::Abs($bricks[$left].transform.position[1] - $bricks[$right].transform.position[1])
			if ($deltaX -lt 66 -and $deltaY -lt 28)
			{
				throw "Brickout level $level overlaps bricks $left and $right."
			}
		}
	}
	foreach ($brick in $bricks)
	{
		$x = [double]$brick.transform.position[0]
		$y = [double]$brick.transform.position[1]
		$behavior = $brick.components | Where-Object { $_.type -eq 'Brick' } | Select-Object -First 1
		$mirror = $bricks | Where-Object {
			[Math]::Abs([double]$_.transform.position[0] + $x) -lt 0.2 -and
			[Math]::Abs([double]$_.transform.position[1] - $y) -lt 0.2
		} | Select-Object -First 1
		$mirrorBehavior = if ($mirror) {
			$mirror.components | Where-Object { $_.type -eq 'Brick' } | Select-Object -First 1
		} else { $null }
		if (!$mirror -or $mirrorBehavior.'Hit Points' -ne $behavior.'Hit Points')
		{
			throw "Brickout level $level has a brick without a mirrored counterpart of equal durability."
		}
		if ($x -ne 0 -and $behavior.Power -and $mirrorBehavior.Power)
		{
			throw "Brickout level $level mirrors a power across both sides of the arena."
		}
	}
	$expectedPowerPlan = @(Get-LevelPowerPlan $level)
	$placedPowers = @($bricks | ForEach-Object {
		($_.components | Where-Object { $_.type -eq 'Brick' } | Select-Object -First 1).Power
	} | Where-Object { $_ })
	if ((($placedPowers | Sort-Object) -join '|') -ne (($expectedPowerPlan | Sort-Object) -join '|'))
	{
		throw "Brickout level $level has powers '$($placedPowers -join '|')', expected '$($expectedPowerPlan -join '|')'."
	}

	$unsafeObstacles = @($scene.entities | Where-Object {
		$_.name -like 'Arena *' -and $_.transform.position[1] -lt 0
	})
	if ($unsafeObstacles.Count -gt 0)
	{
		throw "Brickout level $level places a pinball obstacle in the paddle approach lane."
	}
	$arenaElements = @($scene.entities | Where-Object { $_.name -like 'Arena *' })
	foreach ($element in $arenaElements)
	{
		$x = [double]$element.transform.position[0]
		$y = [double]$element.transform.position[1]
		$rotation = [double]$element.transform.rotation
		$sprite = $element.components | Where-Object { $_.type -eq 'SpriteRenderer' } | Select-Object -First 1
		$mirror = $arenaElements | Where-Object {
			[Math]::Abs([double]$_.transform.position[0] + $x) -lt 0.2 -and
			[Math]::Abs([double]$_.transform.position[1] - $y) -lt 0.2 -and
			[Math]::Abs([double]$_.transform.rotation + $rotation) -lt 0.2 -and
			(($_.components | Where-Object { $_.type -eq 'SpriteRenderer' } | Select-Object -First 1).texture -eq $sprite.texture)
		} | Select-Object -First 1
		if (!$mirror)
		{
			throw "Brickout level $level has a pinball obstacle without a mirrored counterpart."
		}
	}
	$arenaSignature = (($scene.entities | Where-Object { $_.name -like 'Arena *' } | Sort-Object name |
		ForEach-Object { "{0}:{1}:{2}:{3}" -f $_.name, $_.transform.position[0], $_.transform.position[1], $_.transform.rotation }) -join '|')
	[void]$arenaSignatures.Add($arenaSignature)
}

if ($arenaSignatures.Count -ne 20)
{
	throw "Expected 20 distinct pinball layouts, found $($arenaSignatures.Count)."
}

Write-Host 'Brickout scenes, assemblies and menu input actions are up to date.'
