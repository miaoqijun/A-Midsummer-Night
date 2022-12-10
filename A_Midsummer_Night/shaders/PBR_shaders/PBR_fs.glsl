#version 330 core
/* parameters (change before compiling according to config.h) */
#define SHADOWS_SAMPLES 30

out vec4 FragColor;

#define NR_POINT_LIGHTS 3

uniform samplerCube depthMap[NR_POINT_LIGHTS];
uniform bool SSR_test;
uniform int shadow_mode;

struct Material {
    sampler2D texture_albedo1;
    sampler2D texture_metalness1;
    sampler2D texture_roughness1;
    sampler2D texture_ao1;
    sampler2D texture_normal1;
    sampler2D texture_emissive1;
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

#define BLOCKER_SEARCH_NUM_SAMPLES SHADOWS_SAMPLES
#define PCF_NUM_SAMPLES SHADOWS_SAMPLES
#define NUM_RINGS 10

#define EPS 2e-2
#define PI 3.141592653589793
#define PI2 6.283185307179586

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos[NR_POINT_LIGHTS];
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform vec3 viewPos;
uniform float far_plane;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;

vec2 poissonDisk[SHADOWS_SAMPLES];
vec3 poissonDisk_3d[SHADOWS_SAMPLES];

highp float rand_2to1(vec2 uv) { 
  // 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

void poissonDiskSamples( const in vec2 randomSeed ) {

    float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( SHADOWS_SAMPLES );
    float INV_NUM_SAMPLES = 1.0 / float( SHADOWS_SAMPLES );

    float angle = rand_2to1( randomSeed ) * PI2;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for(int i = 0; i < SHADOWS_SAMPLES; i++) {
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

float hard_shadow(int index, vec3 pos) {
    vec3 fragToLight = pos - pointLights[index].position;
    float cur_depth = length(fragToLight);
    float shadow_depth = texture(depthMap[index], fragToLight).r;
    shadow_depth *= far_plane;
    return float(cur_depth < shadow_depth + EPS);
}

float PCF(int index, vec3 pos) {
    float Stride = 10.;
    float shadowmapSize = 1024.;
    float visibility = 0.;

    vec3 fragToLight = pos - pointLights[index].position;
    float cur_depth = length(fragToLight);
    vec3 n = normalize(fragToLight), b1, b2;
    LocalBasis(n, b1, b2);
    mat3 localToWorld = mat3(n, b1, b2);
    poissonDiskSamples(pos.xy);
    for(int i = 0; i < SHADOWS_SAMPLES; i++)
        poissonDisk_3d[i] = localToWorld * vec3(0.0, poissonDisk[i]);
    for(int i = 0; i < SHADOWS_SAMPLES; i++) {
        float shadow_depth = texture(depthMap[index], fragToLight + poissonDisk_3d[i] * Stride / shadowmapSize).r;
        shadow_depth *= far_plane;
        float res = float(cur_depth < shadow_depth + EPS);
        visibility += res;
    }
    return visibility / float(SHADOWS_SAMPLES);
}

float PCSS(int index, vec3 pos){
    float Stride = 50.;
    float shadowmapSize = 1024.;
    float visibility = 0.;
    int blockerNum = 0;
    float block_depth = 0.;

    vec3 fragToLight = pos - pointLights[index].position;
    float cur_depth = length(fragToLight);
    vec3 n = normalize(fragToLight), b1, b2;
    LocalBasis(n, b1, b2);
    mat3 localToWorld = mat3(n, b1, b2);
    poissonDiskSamples(pos.xy);
    for(int i = 0; i < SHADOWS_SAMPLES; i++)
        poissonDisk_3d[i] = localToWorld * vec3(0.0, poissonDisk[i]);

    // STEP 1: avgblocker depth
    for(int i = 0; i < SHADOWS_SAMPLES; i++) {
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
    for(int i = 0; i < SHADOWS_SAMPLES; i++) {
        float shadow_depth = texture(depthMap[index], fragToLight + poissonDisk_3d[i] * Stride * w_penumbra / shadowmapSize).r;
        shadow_depth *= far_plane;
        float res = float(cur_depth < shadow_depth + EPS);
        visibility += res;
    }
    return visibility / float(SHADOWS_SAMPLES);
}

float visibility(int index, vec3 pos) {
    if(shadow_mode == 0)
        return hard_shadow(index, pos);
    else if(shadow_mode == 1)
        return PCF(index, pos);
    else
        return PCSS(index, pos);
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------
void main()
{		
    vec3 N = texture(material.texture_normal1, fs_in.TexCoords).rgb;
    N = normalize(N * 2.0 - 1.0);
    vec3 V = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);

    vec3 albedo = pow(texture(material.texture_albedo1, fs_in.TexCoords).rgb, vec3(2.2));
    float metalness = texture(material.texture_metalness1, fs_in.TexCoords).x;
    float roughness = texture(material.texture_roughness1, fs_in.TexCoords).x;
    float ao = texture(material.texture_ao1, fs_in.TexCoords).x;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metalness);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < NR_POINT_LIGHTS; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(fs_in.TangentLightPos[i] - fs_in.TangentFragPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - fs_in.FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));
        vec3 radiance = pointLights[i].diffuse * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalness;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo

        Lo += (kD * albedo / PI + specular) * radiance * NdotL * visibility(i, fs_in.FragPos);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.01) * albedo * ao;
    float emissive = texture(material.texture_emissive1, fs_in.TexCoords).r;
    vec3 color = Lo + vec3(emissive);
    if(!SSR_test)
        color += ambient;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}









































































































