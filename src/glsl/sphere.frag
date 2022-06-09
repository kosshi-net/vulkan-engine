#version 450

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

void main()
{
	vec3 light_dir = normalize(vec3(0.0, 1.0, 0.0));
	float diffuse = dot(in_normal, light_dir) * 0.5 + 0.5;
	out_color = vec4(in_color.rgb * diffuse, in_color.a);
}
