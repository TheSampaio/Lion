#version 460 core

layout (location = 0) in vec4 iPosition;
layout (location = 1) in vec4 iColor;

out vec4 uColor;

void main()
{
    uColor = iColor;
    gl_Position = iPosition;
}