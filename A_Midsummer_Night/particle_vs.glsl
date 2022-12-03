#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec4 Color;
layout (location = 2) in float scale;
layout (location = 3) in float choice;
uniform mat4 projection; 
uniform mat4 view; 
uniform mat4 model; 
out GS_OUT
{
    vec4 ourColor;
    float ourScale;
    float ourChoice;
} gs_out;

void main()
{
    gl_Position = vec4(Position, 1.0);
    gs_out.ourColor = Color;
    gs_out.ourScale = scale;
    gs_out.ourChoice = choice;
}

