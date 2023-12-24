#version 330

// From Vertex Shader
in vec4 vcolor;
in vec2 vpos; // Distance from local origin

// Application data
uniform vec4 fcolor;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	color = fcolor * vcolor;
}