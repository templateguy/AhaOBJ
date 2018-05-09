#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    vec4 c = texture(texture_diffuse1, TexCoords);
    if(c.x == 0.0 && c.y == 0.0 && c.z == 0.0)
    {
        FragColor = vec4(Color, 1.0);
    }
    else
    {
        FragColor = texture(texture_diffuse1, TexCoords) * vec4(Color, 1.0);
    }
}
