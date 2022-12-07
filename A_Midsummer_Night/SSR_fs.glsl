#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D texture_albedo1;
    sampler2D texture_metalness1;
    sampler2D texture_roughness1;
    sampler2D texture_ao1;
    sampler2D texture_normal1;
    sampler2D texture_emissive1;
}; 

uniform Material material;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform bool SSR_ON;
uniform bool SSR_test;
uniform bool scatter_ON;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    mat4 worldToScreen;
} fs_in;

uniform sampler2D colorMap;
uniform sampler2D depthMap;
uniform samplerCube lightDepthMap;
uniform float far_plane;

#define PI 3.141592653589793
#define TWO_PI 6.283185307
#define INV_PI 0.31830988618
#define INV_TWO_PI 0.15915494309

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float Rand1(inout float p) {
  p = fract(p * .1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}

vec2 Rand2(inout float p) {
  return vec2(Rand1(p), Rand1(p));
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = uv.x;
  float phi = uv.y * TWO_PI;
  float sinTheta = sqrt(1.0 - z*z);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = INV_TWO_PI;
  return dir;
}

vec3 SampleHemisphereCos(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = sqrt(1.0 - uv.x);
  float phi = uv.y * TWO_PI;
  float sinTheta = sqrt(uv.x);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = z * INV_PI;
  return dir;
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

vec4 Project(vec4 a) {
  return a / a.w;
}

vec2 GetScreenCoordinate(vec3 posWorld) {
  vec2 uv = Project(fs_in.worldToScreen * vec4(posWorld, 1.0)).xy * 0.5 + 0.5;
  return uv;
}

float GetDepth(vec3 posWorld) {
    vec4 screenSpaceCoord = fs_in.worldToScreen * vec4(posWorld, 1.0);
    vec3 projCoords = screenSpaceCoord.xyz / screenSpaceCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    float depth = projCoords.z;
    return depth;
}

float GetGBufferDepth(vec2 uv) {
    float depth = texture(depthMap, uv).x;
    if (depth < 1e-2) {
        depth = 1000.0;
    }
    return depth;
}

#define MAX_DIST 1.0
#define STEP_LONG 0.05

bool outScreen(vec3 pos){
    vec2 uv = GetScreenCoordinate(pos);
    return any(bvec4(lessThan(uv, vec2(0.0)), greaterThan(uv, vec2(1.0))));
}

bool RayMarch(vec3 ori, vec3 dir, out vec3 hitPos) {
    int level = 0;
    vec3 delta = normalize(dir) * STEP_LONG;
    vec3 nowPos = ori;
    float total_dist = 0.0;

    while(true) {
        if(outScreen(nowPos))
            return false;
        vec3 nextPos = nowPos + delta;
        float curDepth = GetDepth(nextPos);
        float depth = GetGBufferDepth(GetScreenCoordinate(nextPos));
        if(depth - curDepth >= 1e-5) {
            total_dist += sqrt(dot(delta, delta));
            if(total_dist - MAX_DIST >= 1e-5)
                return false;
            nowPos = nextPos;
            level++;
            delta *= 2.0;
        }
        else {
            level--;
            delta /= 2.0;
            if(level < 0)
                break;
        }
    }
    hitPos = nowPos;
    return true;
}

// PBR
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

vec3 PBR_shading(vec3 hitPos, vec3 hitColor)
{		
    vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(viewPos - fs_in.FragPos);

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

    // calculate per-light radiance
    vec3 L = normalize(hitPos - fs_in.FragPos);
    vec3 H = normalize(V + L);
    float distance = length(hitPos - fs_in.FragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
    vec3 radiance = hitColor * attenuation;

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

    Lo = (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    
    // HDR tonemapping
    vec3 color = Lo / (Lo + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    return color;
}

//scatter

#define EPS 2e-2
#define SCATTER_SAMPLES 50
#define HOUSE_MIN_X -1.35294
#define HOUSE_MAX_X 1.35294
#define HOUSE_MIN_Y 0.68627
#define HOUSE_MAX_Y 1.66667
#define HOUSE_MIN_Z -1.39216
#define HOUSE_MAX_Z 1.43137

highp float rand_2to1(vec2 uv) { 
  // 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

float phaseFunction(vec3 viewPos, vec3 lightPos, vec3 nowPos)
{
    const float g = 0.9;
    vec3 lightDr = normalize(nowPos - lightPos);
    vec3 rd = normalize(nowPos - viewPos);
    float cosTheta = dot(lightDr, -rd);
    return 1 / (4 * PI) * (1 - g * g)/ pow(1 + g * g - 2 * g * cosTheta, 1.5);
}

float evaluateDensity(vec3 nowPos)
{
    if(nowPos.x < HOUSE_MIN_X || nowPos.x > HOUSE_MAX_X ||
       nowPos.y < HOUSE_MIN_Y || nowPos.y > HOUSE_MAX_Y ||
       nowPos.z < HOUSE_MIN_Z || nowPos.z > HOUSE_MAX_Z) //not in house
        return 0.0;
    return 0.3;
}

float visibility(vec3 pos) {
    vec3 fragToLight = pos - lightPos;
    float cur_depth = length(fragToLight);
    float shadow_depth = texture(lightDepthMap, fragToLight).r;
    shadow_depth *= far_plane;
    return float(cur_depth < shadow_depth + EPS);
}

vec4 volumeScattering(vec3 rO, vec3 finalPos)
{
    const float lightIntense = 30.;

    float transmittance = 1.0;
    vec3 scatteredLight = vec3(0.0, 0.0, 0.0);
    float step_long = distance(finalPos, rO) / SCATTER_SAMPLES;

    vec3 rD = normalize(finalPos - rO) * step_long;
    vec3 nowPos = rO + rand_2to1(finalPos.xy) * rD;
    while(true) {
        if(dot(nowPos - rO, nowPos - rO) - dot(finalPos - rO, finalPos - rO) > EPS)
            break;
        float vLight = lightIntense / dot(nowPos - lightPos, nowPos - lightPos);
        float D = evaluateDensity(nowPos);
        scatteredLight += D * vLight * phaseFunction(rO, lightPos, nowPos) * transmittance * step_long * visibility(nowPos);
        transmittance *= exp(-D * step_long);
        nowPos += rD;
    }
    return vec4(scatteredLight, transmittance);
}

#define SAMPLE_NUM 20

void main() {
    vec3 albedo = pow(texture(material.texture_albedo1, fs_in.TexCoords).rgb, vec3(2.2));
    float ao = texture(material.texture_ao1, fs_in.TexCoords).x;

    float s = InitRand(gl_FragCoord.xy);

    vec3 L = vec3(0.0);
    vec2 uv = GetScreenCoordinate(fs_in.FragPos);
    L = texture(colorMap, uv).rgb;

    if(SSR_test && SSR_ON) {
        vec3 L_in = vec3(0.0);
        for(int i = 0; i < SAMPLE_NUM; i++) {    
            vec3 normal = normalize(fs_in.Normal);
            vec3 b1, b2;
            LocalBasis(normal, b1, b2);
            mat3 localToWorld = mat3(b1, b2, normal);
            float pdf;
            vec3 dir = normalize(localToWorld * SampleHemisphereCos(s, pdf));
            vec3 hitPos;
            bool hit = RayMarch(fs_in.FragPos, dir, hitPos);
            if(dot(dir, normal) > 0.0 && hit) {
                vec2 uv_hit = GetScreenCoordinate(hitPos);
                L_in += PBR_shading(hitPos, texture(colorMap, uv_hit).rgb);
            }
        }
        L_in /= float(SAMPLE_NUM);
  
        L += L_in;
    }

    if(scatter_ON) {
        vec4 scatTrans = volumeScattering(viewPos, fs_in.FragPos);
        // HDR tonemapping
        scatTrans.xyz = scatTrans.xyz / (scatTrans.xyz + vec3(1.0));
        // gamma correct
        scatTrans.xyz = pow(scatTrans.xyz, vec3(1.0/2.2)); 
        L = L * scatTrans.w + scatTrans.xyz;
    }

    FragColor = vec4(L, 1.0);
}