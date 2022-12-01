#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

#define NR_POINT_LIGHTS 2

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos[NR_POINT_LIGHTS];
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 viewPos;
uniform PointLight pointLights[NR_POINT_LIGHTS];

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    vs_out.FragPos = vec3(model * vec4(position, 1.0));   
    vs_out.TexCoords = texCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);    
    
    mat3 TBN = transpose(mat3(T, B, N));  
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        vs_out.TangentLightPos[i] = TBN * pointLights[i].position;
    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;
}