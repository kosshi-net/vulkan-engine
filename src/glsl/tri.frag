#version 450

layout(location = 0) in vec2 fragTexUV;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 frag_pos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
	vec3 light_dir  = normalize(vec3(1.0, 1.0, 0.0));
	vec3 light2_dir = normalize(vec3(0.0, 0.0, 1.0));

	vec3 view_dir = normalize(frag_pos);
	vec3 _normal = normalize(normal);

	float diffuse = max( dot(_normal, light_dir)*0.7+0.3, 0.0 ) + 0.1;
	
	vec3 reflection  = reflect(-light_dir,  _normal);
	vec3 reflection2 = reflect(-light2_dir, _normal);

	float specular   = pow( max( dot( view_dir, reflection ), 0.0), 32.0) * 2.0;
	float specular2  = pow( max( dot( view_dir, reflection2), 0.0), 16.0) * 0.25;

	vec4 tex = texture(texSampler, fragTexUV);
	outColor =  tex * (diffuse+specular2+specular);
}
