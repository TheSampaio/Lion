#version 460 core

// Input
layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec4 iColour;

// Output
out vec4 uColour;

// Shader
void main()
{
    uColour = iColour;
    gl_Position = vec4(iPosition, 1.0);
}