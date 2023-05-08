#version 460 core

// Input
in vec4 uColour;

// Output
out vec4 gl_Color;

// Shader
void main()
{
    if (uColour.a > 1.0)
    {
        gl_Color = vec4(uColour.r, uColour.g, uColour.b, 1.0);
    }

    else
    {
        gl_Color = uColour;
    }
}