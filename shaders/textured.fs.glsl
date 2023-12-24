#version 330

// From vertex shader
in vec2 texcoord;
in vec2 vpos; // Distance from local origin

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float darken_factor;
uniform int light_up;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	if (darken_factor > 0)
		color -= darken_factor * vec4(0.8, 0.8, 0.8, 0);
	float radius = distance(vec2(0.0), vpos);
	if (light_up == 1 && radius < 0.25)
	{
		// 0.8 is just to make it not too strong
		color += (0.25 - radius) * vec4(0.6, 0.7, 1.0, 0);
	}
}