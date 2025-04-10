#shader vertex
#version 460 core

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;
layout(location = 2) in vec2 iTex;

out vec3 vColor;
out vec2 vTex;

void main()
{
	vColor = iColor;
	vTex = iTex;
	gl_Position = vec4(iPosition, 1.0);
}

#shader fragment
#version 460 core

in vec3 vColor;
in vec2 vTex;

out vec4 oFragment;

uniform sampler2D uDiffuseSampler;

void main()
{
	oFragment = texture(uDiffuseSampler, vTex) * vec4(vColor, 1.0);
}
