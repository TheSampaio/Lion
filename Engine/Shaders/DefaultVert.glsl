#version 460 core

layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec4 iColor;

out vec4 uColor;

void main()
{
    uColor = iColor;
    gl_Position = vec4(iPosition, 1.0);
}