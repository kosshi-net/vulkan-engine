#version 450

layout(push_constant) uniform SoftbodyUniform {
	mat4 proj;
	mat4 view;
} uni;

layout(set=0, binding=0) uniform Scene {
	mat4 matrix;
	vec3 dir;
} light;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec4 frag_pos;
layout(location = 2) out vec4 frag_pos_light;
layout(location = 3) out vec3 light_dir;

void main() 
{

	normal = in_normal;

	vec4 pos = vec4(in_pos, 1.0);

	gl_Position = uni.proj * uni.view * pos;

	frag_pos_light = light.matrix * pos;

	frag_pos = pos;

	light_dir = light.dir;
}
