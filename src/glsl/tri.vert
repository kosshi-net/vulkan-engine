#version 450

layout(set=0, binding = 0) uniform SceneUBO {
	mat4 view;
	mat4 proj;
} scene_ubo;

layout(set=1, binding = 0) uniform ObjectUBO {
	mat4 model;
} object_ubo;

layout(location = 0) out vec2 fragTexUV;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 frag_pos;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

void main() 
{
	gl_Position = scene_ubo.proj * scene_ubo.view * object_ubo.model * vec4(inPosition, 1.0);
	fragTexUV = inColor.xy;
	frag_pos = gl_Position.xyz;
	normal = ( transpose(inverse(object_ubo.model)) * vec4(inNormal, 1.0)).xyz;
}
