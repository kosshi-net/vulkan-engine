#version 450

layout(set=0, binding = 0) uniform TextUBO {
	mat4 ortho;
} ubo;

layout(location = 0) out vec2 fragTexUV;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

void main() 
{
	gl_Position = ubo.ortho * vec4(inPosition, 1.0);
	fragTexUV = inColor.xy;
}
