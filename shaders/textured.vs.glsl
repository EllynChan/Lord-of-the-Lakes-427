#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;
out vec2 vpos;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform mat3 view; // add view matrix uniform

void main()
{
	texcoord = in_texcoord;
	vpos = in_position.xy; // local coordinated before transform
	vec3 pos = projection * view * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}