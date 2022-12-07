#version 330 core 

uniform sampler2D texture;
in vec4 v_color;
in float v_factor;
out vec4 FragColor;

void main()
{
	vec4 texColor;
	vec2 texCoords;

	texCoords = vec2(gl_PointCoord.x, gl_PointCoord.y);
	texColor = texture2D(texture, texCoords);
	gl_FragColor = vec4(texColor.r, texColor.r, texColor.r, 0.08) * v_color;
}