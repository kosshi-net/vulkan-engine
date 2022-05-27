#version 450

layout(location = 0) in vec2 fragTexUV;

layout(location = 1) in vec4 color;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
	vec4 tex = texture(texSampler, fragTexUV);
	float alpha = tex.r;
	outColor = vec4(color.rgb, color.a * alpha);
}
