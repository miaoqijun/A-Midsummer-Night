#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 normal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

float _Amplitude1=0.04, _Speed1=0.05, _WaveLength1=0.2;
float _Amplitude2=0.05, _Speed2=0.1, _WaveLength2=0.3;
float _Amplitude3=0.034, _Speed3=0.3, _WaveLength3=0.1;
vec2 _Direction1=vec2(0.9,-0.53),_Direction2=vec2(0.26,0.03),_Direction3=vec2(0.59,-0.27);

float Wave(vec3 positionOS, float amplitude, float speed, float waveLength, vec2 direction, float k);
float DerivativesHeightX(vec3 positionOS, float amplitude, float speed, float waveLength, vec2 direction);
float DerivativesHeightY(vec3 positionOS, float amplitude, float speed, float waveLength, vec2 direction);

void main()
{
	float height = 0;

    float h1 = Wave(aPos, _Amplitude1, _Speed1, _WaveLength1, _Direction1, 1);
    float h2 = Wave(aPos, _Amplitude2, _Speed2, _WaveLength2, _Direction2, 1.5);
    float h3 = Wave(aPos, _Amplitude3, _Speed3, _WaveLength3, _Direction3, 2);
    height = (h1 + h2 + h3);

    float b1 = DerivativesHeightX(aPos, _Amplitude1, _Speed1, _WaveLength1, _Direction1);
    float b2 = DerivativesHeightX(aPos, _Amplitude2, _Speed2, _WaveLength2, _Direction2);
    float b3 = DerivativesHeightX(aPos, _Amplitude3, _Speed3, _WaveLength3, _Direction3);
    float b = b1 + b2 + b3;
    vec3 binormal = vec3(0, b, 1);

    float t1 = DerivativesHeightY(aPos, _Amplitude1, _Speed1, _WaveLength1, _Direction1);
    float t2 = DerivativesHeightY(aPos, _Amplitude2, _Speed2, _WaveLength2, _Direction2);
    float t3 = DerivativesHeightY(aPos, _Amplitude3, _Speed3, _WaveLength3, _Direction3);
    float t = t1 + t2 + t3;
    vec3 tangent = vec3(1, t, 0);

    normal = normalize(cross(binormal, tangent));

	gl_Position = projection * view * model * vec4(aPos.x,height*0.5,aPos.z,1.0);
    FragPos = vec3(model * vec4(aPos.x, height*0.5, aPos.z, 1.0));
	TexCoord = aTexCoord + vec2(time) / 5000.0f;
}

float Wave(vec3 positionOS, float amplitude, float speed, float waveLength, vec2 direction,float k)
{
    direction = normalize(direction);
    float t = time;
    float w = 2 / waveLength;
    float phase = speed * w;
    float H = 2 * amplitude * pow((sin(dot(positionOS.xz, direction) * w + t * phase) + 1) / 2, k);
    //float H = amplitude * sin(dot(positionOS.xz, direction) * w + t * phase);
    return H;
}


float DerivativesHeightY(vec3 positionOS, float amplitude, float speed, float waveLength, vec2 direction)
{
    float w = 2 / waveLength;
    float phase = speed * w;
    return w * direction.y * amplitude * cos(dot(direction, positionOS.xz) * w + time * phase);
}

float DerivativesHeightX(vec3 positionOS, float amplitude, float speed, float waveLength, vec2 direction)
{
    float w = 2 / waveLength;
    float phase = speed * w;
    return w * direction.x * amplitude * cos(dot(direction, positionOS.xz) * w + time * phase);
}


