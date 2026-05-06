#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D videoTexture;
uniform float brightness;

void main()
{
    vec3 color = texture(videoTexture, TexCoord).rgb;
    FragColor = vec4(color * brightness, 1.0);
}
