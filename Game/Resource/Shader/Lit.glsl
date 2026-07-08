#shader vertex
#version 460 core

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iColor;
layout(location = 2) in vec2 iTexCoord;
layout(location = 3) in float iTexId;
layout(location = 4) in float iEntityId;

out vec4 vColor;
out vec2 vTexCoord;
out float vTexId;
flat out int vEntityId;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
	vColor = iColor;
	vTexCoord = iTexCoord;
	vTexId = iTexId;
	vEntityId = int(iEntityId);
	gl_Position = uProjection * uView * vec4(iPosition, 1.0);
}

#shader fragment
#version 460 core

#define MAX_TEXTURE_COUNT 32

in vec4 vColor;
in vec2 vTexCoord;
in float vTexId;
flat in int vEntityId;

layout(location = 0) out vec4 oFragment;
layout(location = 1) out int oEntityId;   // Editor picking id (ignored when no such attachment is bound).

uniform sampler2D uDiffuseTextureArray[MAX_TEXTURE_COUNT];

void main()
{
	int index = int(vTexId);
	vec4 color = texture(uDiffuseTextureArray[index], vTexCoord) * vColor;

	oFragment = color;

	// Pixel-perfect picking: transparent texels don't belong to the entity, so mark them empty.
	oEntityId = color.a < 0.1 ? -1 : vEntityId;
}
