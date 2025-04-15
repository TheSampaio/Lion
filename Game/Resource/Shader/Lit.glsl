#shader vertex
#version 460 core

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iColor;
layout(location = 2) in vec2 iTexCoord;
layout(location = 3) in float iTexId;

out vec4 vColor;
out vec2 vTexCoord;
out float vTexId;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
	vColor = iColor;
	vTexCoord = iTexCoord;
	vTexId = iTexId;
	gl_Position = uProjection * uView * vec4(iPosition, 1.0);
}

#shader fragment
#version 460 core

#define MAX_TEXTURE_COUNT 32

in vec4 vColor;
in vec2 vTexCoord;
in float vTexId;

out vec4 oFragment;

uniform sampler2D uDiffuseTextureArray[MAX_TEXTURE_COUNT];

void main()
{
	int index = int(vTexId);
	oFragment = texture(uDiffuseTextureArray[index], vTexCoord) * vColor;
}
