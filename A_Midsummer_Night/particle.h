#pragma once
#define PI 3.14159f
#include <vector>


#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <glm/glm.hpp>


const float FIRE_INIT_SIZE = 10.0f;
const glm::vec3 FIRE_MaxVeloc(0.0f, 10.0f, 0.0f);
const glm::vec3 FIRE_MinVeloc(0.0f, 5.0f, 0.0f);
const glm::vec3 FIRE_ACC_SPEED(0.0, 0.08f, 0.0);
const glm::vec4 FIRE_INIT_COLOR(0.5f, 0.3f, 0.1f, 1.0f);
const float FIRE_MAX_LIFE = 0.7f;
const float FIRE_MIN_LIFE = 0.6f;
const float FIRE_ALPHA = 1.0f;
const float SMOKE_INIT_SIZE = 7.0f;
const glm::vec3 SMOKE_MaxVeloc(0.0f, 0.03f, 0.0f);
const glm::vec3 SMOKE_MinVeloc(0.0f, 0.02f, 0.0f);
const glm::vec3 SMOKE_ACC_SPEED(0.0, 0.008f, 0.0);
const glm::vec4 SMOKE_INIT_COLOR(0.1f, 0.1f, 0.1f, 1.0f);
const float SMOKE_MAX_LIFE = 1.0f;
const float SMOKE_MIN_LIFE = 0.9f;
const int SMOKE_NEW_PARTICLES = 5;
const float MAX_EMISSION_ANGLE = PI / 12.0f;
const glm::vec3 U_GRAVITY(0.0f, 500.0f, 0.0f);
const int NEW_PARTICLES = 5;

struct FireParticle {
    glm::vec3 Position, Velocity;
    glm::vec4 Color;
    GLfloat Life; //粒子寿命
    GLfloat Age; //粒子年龄
    //GLfloat alpha;
    GLfloat size;//粒子点精灵大小
};
struct SmokeParticle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 Color;
    GLfloat Life;
    GLfloat Age;
    GLfloat Delay;
    GLfloat Factor;
};
class FireParticleGenerator
{
public:
    FireParticleGenerator();
    void Update(GLfloat dt);
    void Draw(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPos);
    glm::vec3 get_light_position();
private:
    // State  
    const char* vs_path = "shaders/particle_shaders/particle_vs.glsl";
    const char* gs_path = "shaders/particle_shaders/particle_gs.glsl";
    const char* fs_path = "shaders/particle_shaders/particle_fs.glsl";
    std::vector<FireParticle> particles;
    const GLuint amount = 70000;
    glm::vec3 avg_coord;
    std::vector<float> vertices;
    Shader shader;
    GLuint lastUsedParticle;
    GLuint VAO;
    GLuint VBO;
    const glm::vec3 scale = glm::vec3(0.03f, 0.03f, 0.03f);
    const glm::vec3 position = glm::vec3(-1.35f, 0.86f, -0.018f);
    glm::mat4 model;
    void init();
    GLuint firstUnusedParticle();
    void respawnParticle(FireParticle& particle);
};
class SmokeParticleGenerator
{
public:
    // Constructor
    SmokeParticleGenerator();
    // Update all particles
    void Update(GLfloat dt);
    // Render all particles
    void Draw(glm::mat4 projection, glm::mat4 view);
private:
    // State
    const char* vs_path = "shaders/particle_shaders/smoke_vs.glsl";
    const char* gs_path = "shaders/particle_shaders/smoke_gs.glsl";
    const char* fs_path = "shaders/particle_shaders/smoke_fs.glsl";
    std::vector<SmokeParticle> particles;
    const GLuint amount = 50000;
    // Render state
    Shader shader;
    GLuint VAO;
    GLuint VBO;
    GLuint lastUsedParticle;
    const glm::vec3 scale = glm::vec3(0.20f, 0.20f, 0.20f);
    const glm::vec3 position = glm::vec3(-1.57f, 2.8f, 0.0f);
    glm::mat4 model;
    // Initializes buffer and vertex attributes
    void init();
    // Returns the first Particle index that's currently unused e.g. Life <= 0.0f or 0 if no particle is currently inactive
    GLuint firstUnusedParticle();
    // Respawns particle
    void respawnParticle(SmokeParticle& particle);
};
