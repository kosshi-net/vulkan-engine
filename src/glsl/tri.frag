#version 450

layout(location = 0) in vec2 fragTexUV;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
	//outColor = vec4(fragTexUV, 0.0, 1.0);
	outColor = texture(texSampler, fragTexUV);
}
