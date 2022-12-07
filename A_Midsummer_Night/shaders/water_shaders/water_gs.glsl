#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 Normal[];
in vec3 FragPos[];
in vec2 TexCoords[];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

#define NR_POINT_LIGHTS 3

out GS_OUT
{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 TangentLightPos[NR_POINT_LIGHTS];
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat4 worldToScreen;
} gs_out;

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform PointLight pointLights[NR_POINT_LIGHTS];

void main() {    
    // positions
    vec3 pos1 = FragPos[0];
    vec3 pos2 = FragPos[1];
    vec3 pos3 = FragPos[2];
    // texture coordinates
    vec2 uv1 = TexCoords[0];
    vec2 uv2 = TexCoords[1];
    vec2 uv3 = TexCoords[2];

    // calculate tangent/bitangent vectors of both triangles
    vec3 tangent, bitangent;
    // - triangle 1
    vec3 edge1 = pos2 - pos1;
    vec3 edge2 = pos3 - pos1;
    vec2 deltaUV1 = uv2 - uv1;
    vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent = normalize(tangent);

    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    bitangent = normalize(bitangent);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T, B, N;
    mat3 TBN;

    T = normalize(normalMatrix * tangent);
    N = normalize(normalMatrix * Normal[0]);    
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    B = cross(T, N);
    TBN = transpose(mat3(T, B, N));  
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        gs_out.TangentLightPos[i] = TBN * pointLights[i].position;
    gs_out.TangentViewPos  = TBN * viewPos;
    gs_out.TangentFragPos  = TBN * FragPos[0];
    gs_out.FragPos = FragPos[0];
    gs_out.TexCoords = TexCoords[0];
    gs_out.Normal = Normal[0];
    gs_out.worldToScreen = projection * view;
    gl_Position = gl_in[0].gl_Position; 
    EmitVertex();

    T = normalize(normalMatrix * tangent);
    N = normalize(normalMatrix * Normal[1]);    
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    B = cross(T, N);
    TBN = transpose(mat3(T, B, N));  
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        gs_out.TangentLightPos[i] = TBN * pointLights[i].position;
    gs_out.TangentViewPos  = TBN * viewPos;
    gs_out.TangentFragPos  = TBN * FragPos[1];
    gs_out.FragPos = FragPos[1];
    gs_out.TexCoords = TexCoords[1];
    gs_out.Normal = Normal[1];
    gs_out.worldToScreen = projection * view;
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    T = normalize(normalMatrix * tangent);
    N = normalize(normalMatrix * Normal[2]);    
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    B = cross(T, N);
    TBN = transpose(mat3(T, B, N));  
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        gs_out.TangentLightPos[i] = TBN * pointLights[i].position;
    gs_out.TangentViewPos  = TBN * viewPos;
    gs_out.TangentFragPos  = TBN * FragPos[2];
    gs_out.FragPos = FragPos[2];
    gs_out.TexCoords = TexCoords[2];
    gs_out.Normal = Normal[2];
    gs_out.worldToScreen = projection * view;
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}