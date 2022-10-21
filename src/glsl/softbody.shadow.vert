#version 450

layout(set=0, binding=0) uniform Scene {
	mat4 matrix;
	vec3 dir;
} light;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;

void main() 
{
	vec4 pos = vec4(in_pos, 1.0);
	gl_Position = light.matrix * pos;
}
