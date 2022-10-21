#version 450


layout(location = 0) in vec3 normal;
layout(location = 1) in vec4 frag_pos;
layout(location = 2) in vec4 frag_pos_light;
layout(location = 3) in vec3 light_dir;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D shadowmap;

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

float compute_shadow(vec4 pos)
{

	//vec3 proj = pos.xyz / pos.w;

	float bias = 0.0005;

	float map_depth = texture(shadowmap, pos.xy * 0.5 + 0.5 ).r;
	float cur_depth = pos.z;

	//return pos.z;
	//return map_depth;

	float shadow = cur_depth-bias < map_depth  ? 1.0 : 0.0;  
	return shadow;
/*
	float cur_depth = proj.z;


	float shadow = cur_depth > map_depth  ? 1.0 : 0.0;  

	return map_depth;

	return shadow; 
*/
}

void main()
{
	vec3 color = gl_FrontFacing ? vec3(1.0, 0.5, 1.0) : vec3(0.3, 0.3, 0.3);

	if (frag_pos.y < 0.0) color = vec3(0.05);

	float light_dot = dot(normal, light_dir);

	if(gl_FrontFacing)
		light_dot = -light_dot;
	
	float ambient = 0.25;


	float shadow;
	if (light_dot <= 0.0){
		shadow = 0.0;
	}else{
		shadow  = compute_shadow(frag_pos_light);
	}

	if (shadow < 0.5)
		light_dot = -light_dot;

	shadow = max(shadow, 0.1);

	float diffuse = light_dot * 0.5 + 0.5;
	diffuse = pow(diffuse, 2.0);
	diffuse *= 1.0 - ambient;

	outColor = vec4(color* (diffuse*shadow+ambient), 1.0);
	
}
