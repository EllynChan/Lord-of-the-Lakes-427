#version 330

// Input attributes
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;

// Passed to fragment shader
out vec2 vTextureCoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform mat3 view;

void main()
{
	vTextureCoord = in_texcoord;
	vec3 pos = projection * view * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}