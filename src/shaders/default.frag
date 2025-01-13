#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec2 pos;

uniform sampler2D texture1;

void main()
{
    //FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    FragColor = vec4(texture(texture1, TexCoord).rgb,1.0f);
    //FragColor = vec4(TexCoord.x, TexCoord.y,0.0f,1.0f);
} 