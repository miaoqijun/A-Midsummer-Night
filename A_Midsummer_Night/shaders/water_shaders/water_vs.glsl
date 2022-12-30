#version 330 core
 
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 atexture;
 
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
 
void main() {
    gl_Position = projection * view * model * vec4(vertex.x, vertex.y * 0.01, vertex.z, 1.0);

    Normal = normalize(mat3(transpose(inverse(model))) * normal);
    FragPos = vec3(model * vec4(vertex.x, vertex.y * 0.01, vertex.z, 1.0));
    TexCoords = atexture + vec2(time) / 5000.0f;
}