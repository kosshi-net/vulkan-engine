#version 450

layout(push_constant) uniform Uniform {
	mat4 proj;
	mat4 view;
} uni;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 color;

void main() 
{
	gl_Position = uni.proj * uni.view * vec4(inPosition, 1.0);
	gl_Position.z -= 0.0001;
	color = inColor;
}
