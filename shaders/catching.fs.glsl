#version 330

// From vertex shader
in vec2 vTextureCoord;

// Application data
uniform float uFilled; // between 0.0 and 1.0
uniform sampler2D sampler0;

// Output color
layout(location = 0) out vec4 FragColor;

void main() {
    vec2 texCoord;
        if (vTextureCoord.y <= (1.0 - uFilled)) {
            texCoord = vec2(vTextureCoord.x / 2, vTextureCoord.y);
        } else {
            texCoord = vec2(vTextureCoord.x / 2 + 0.5, vTextureCoord.y);
        }
    FragColor = texture(sampler0, texCoord);
}