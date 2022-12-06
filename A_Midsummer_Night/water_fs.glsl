#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
	//FragColor = texture(texture1, TexCoord);
	FragColor = vec4(95f/255,202f/255,248f/255,1.0f);
}