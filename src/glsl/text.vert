#version 450

layout(set=0, binding = 0) uniform TextUBO {
	mat4 ortho;
} ubo;

layout(location = 0) out vec2 fragTexUV;
layout(location = 1) out vec4 color;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

void main() 
{
	gl_Position = ubo.ortho * vec4(inPosition, 0.0, 1.0);
	fragTexUV = inTexCoord.xy;
	color = inColor;
}
