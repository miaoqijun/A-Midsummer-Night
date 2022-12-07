#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec4 Color;
layout (location = 2) in float factor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec4 v_color;
out float v_factor;

void main()
{
	float ageFactor = clamp(factor, 0.0f, 1.0f);
	float scale = ageFactor * 32.0f;
	gl_Position = projection * view * model * vec4(Position, 1.0);
	gl_PointSize = scale;
	v_color = Color;
	v_factor = ageFactor;
}
