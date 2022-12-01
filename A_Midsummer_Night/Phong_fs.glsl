#version 330 core
out vec4 FragColor;

#define NR_POINT_LIGHTS 3

uniform samplerCube depthMap[NR_POINT_LIGHTS];

struct Material {
    sampler2D texture_diffuse1;
    vec3 specular;
    float shininess;
}; 

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// Shadow map related variables
#define NUM_SAMPLES 20
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10

#define EPS 2e-2
#define PI 3.141592653589793
#define PI2 6.283185307179586

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform vec3 viewPos;
uniform float far_plane;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;

vec2 poissonDisk[NUM_SAMPLES];
vec3 poissonDisk_3d[NUM_SAMPLES];

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 ambient = light.ambient * texture(material.texture_diffuse1, fs_in.TexCoords).xyz;
    vec3 diffuse = light.diffuse * diff * texture(material.texture_diffuse1, fs_in.TexCoords).xyz;
    vec3 specular = light.specular * spec * material.specular;
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float visibility)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * texture(material.texture_diffuse1, fs_in.TexCoords).xyz;
    vec3 diffuse = light.diffuse * diff * texture(material.texture_diffuse1, fs_in.TexCoords).xyz;
    vec3 specular = light.specular * spec * material.specular;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return ambient + visibility * diffuse;
}

highp float rand_2to1(vec2 uv) { 
  // 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

void poissonDiskSamples( const in vec2 randomSeed ) {

    float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
    float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

    float angle = rand_2to1( randomSeed ) * PI2;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for(int i = 0; i < NUM_SAMPLES; i++) {
        poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
        radius += radiusStep;
        angle += ANGLE_STEP;
    }
}

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
  float sign_ = sign(n.z);
  if (n.z == 0.0) {
    sign_ = 1.0;
  }
  float a = -1.0 / (sign_ + n.z);
  float b = n.x * n.y * a;
  b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
  b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

float PCSS(int index){
    float Stride = 50.;
    float shadowmapSize = 1024.;
    float visibility = 0.;
    int blockerNum = 0;
    float block_depth = 0.;

    vec3 fragToLight = fs_in.FragPos - pointLights[index].position;
    float cur_depth = length(fragToLight);
    vec3 n = normalize(fragToLight), b1, b2;
    LocalBasis(n, b1, b2);
    mat3 localToWorld = mat3(n, b1, b2);
    poissonDiskSamples(fs_in.FragPos.xy);
    for(int i = 0; i < NUM_SAMPLES; i++)
        poissonDisk_3d[i] = localToWorld * vec3(0.0, poissonDisk[i]);

    // STEP 1: avgblocker depth
    for(int i = 0; i < NUM_SAMPLES; i++) {
        float shadow_depth = texture(depthMap[index], fragToLight + poissonDisk_3d[i] * Stride / shadowmapSize).r;
        shadow_depth *= far_plane;
        if(cur_depth > shadow_depth + EPS) {
            blockerNum++;
            block_depth += shadow_depth;
        }
    }
    if(blockerNum != 0) {
        block_depth /= float(blockerNum);
    }
    // STEP 2: penumbra size
    float w_penumbra = 2.;
    if(blockerNum != 0)
        w_penumbra *= (cur_depth - block_depth) / block_depth;
    // STEP 3: filtering
    Stride = 10.;
    for(int i = 0; i < NUM_SAMPLES; i++) {
        float shadow_depth = texture(depthMap[index], fragToLight + poissonDisk_3d[i] * Stride * w_penumbra / shadowmapSize).r;
        shadow_depth *= far_plane;
        float res = float(cur_depth < shadow_depth + EPS);
        visibility += res;
    }
    return visibility / float(NUM_SAMPLES);
}

void main()
{    
    // properties
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    
    vec3 result = vec3(0.0);
    float visibility;
    // phase 1: directional lighting
    //vec3 result = CalcDirLight(dirLight, norm, viewDir);
    // phase 2: point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result +=  CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir, PCSS(i));    

    FragColor = vec4(result, 1.0);
}