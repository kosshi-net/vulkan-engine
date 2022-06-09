#version 450

layout(set=0, binding=0) uniform Scene {
	mat4 proj;
	mat4 view;
} scene;

layout(location = 0) in vec3  in_vertex;
layout(location = 1) in vec4  in_pos;
layout(location = 2) in vec4  in_color;

layout(location = 0) out vec4 color;
layout(location = 1) out vec3 normal;

void main() 
{
	normal = in_vertex;
	vec3 vertex = in_vertex * in_pos.w + in_pos.xyz;
	gl_Position = scene.proj * scene.view * vec4(vertex, 1.0);
	color = in_color;
}
