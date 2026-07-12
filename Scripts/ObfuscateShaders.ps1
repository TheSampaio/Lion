# Obfuscates shipped shader files so they are not human-readable on disk.
#
# Called as a post-build step for the Shipping configuration. It XOR-encodes every ".glsl" file
# under <OutputDir>/Shaders with a fixed key; the engine XOR-decodes them at load time (see
# Engine/Source/Lion/Render/OpenGL/OpenGLShader.cpp). The build copies fresh plaintext shaders
# before this runs, so it is safe to run on every build (never double-encodes).

param([Parameter(Mandatory = $true)][string]$OutputDir)

$key = 0x5A
$shaderDir = Join-Path $OutputDir 'Shaders'

if (-not (Test-Path $shaderDir)) {
    exit 0
}

Get-ChildItem -Path $shaderDir -Filter '*.glsl' -Recurse | ForEach-Object {
    $bytes = [System.IO.File]::ReadAllBytes($_.FullName)

    for ($i = 0; $i -lt $bytes.Length; $i++) {
        $bytes[$i] = $bytes[$i] -bxor $key
    }

    [System.IO.File]::WriteAllBytes($_.FullName, $bytes)
}
