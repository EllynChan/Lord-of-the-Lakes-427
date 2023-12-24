#version 330

// Input attributes
in vec3 in_position;
in vec4 in_color;

out vec4 vcolor;
out vec2 vpos;

// Application data
uniform vec2 offsets[100];
uniform mat3 transform;
uniform mat3 projection;
uniform mat3 view; // add view matrix uniform

void main()
{
	vpos = in_position.xy; // local coordinated before transform
	vcolor = in_color;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0); // why not simply *in_position.xyz ?
	vec2 offset = offsets[gl_InstanceID];
    gl_Position = vec4(pos.xy + offset, in_position.z, 1.0);
	//gl_Position = vec4(pos.xy, in_position.z, 1.0);
}