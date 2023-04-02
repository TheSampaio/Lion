#version 460 core

in vec4 uColor;
out vec4 gl_Color;

void main()
{
    gl_Color = uColor;
}