#pragma once

#ifndef PARTICLE_GENERATOR_H
#define PARTICLE_GENERATOR_H

#include <vector>


#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <glm/glm.hpp>


const float INIT_SIZE = 10.0f;
const glm::vec3 MaxVeloc(0.0f, 10.0f, 0.0f);
const glm::vec3 MinVeloc(0.0f, 5.0f, 0.0f);
const glm::vec3 ACC_SPEED(0.0, 0.08f, 0.0);
const glm::vec4 INIT_COLOR(0.5f, 0.3f, 0.1f, 1.0f);
const float MAX_LIFE = 0.4f;
const float MIN_LIFE = 0.3f;
const float ALPHA = 1.0f;
const int NEW_PARTICLES = 5;

struct Particle {
    glm::vec3 Position, Velocity;
    glm::vec4 Color;
    GLfloat Life; //粒子寿命
    GLfloat Age; //粒子年龄
    //GLfloat alpha;
    GLfloat size;//粒子点精灵大小
};
class ParticleGenerator
{
public:
    ParticleGenerator();
    void Update(GLfloat dt);
    void Draw(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPos);
    glm::vec3 avg_coord;
private:
    // State  
    const char* vs_path = "particle_vs.glsl";
    const char* gs_path = "particle_gs.glsl";
    const char* fs_path = "particle_fs.glsl";
    std::vector<Particle> particles;
    const GLuint amount = 70000;

    Shader shader;
    GLuint lastUsedParticle;
    GLuint VAO;
    GLuint VBO;
    const glm::vec3 scale = glm::vec3(0.03f, 0.03f, 0.03f);
    const glm::vec3 position = glm::vec3(-1.35f, 0.86f, -0.018f);
    glm::mat4 model;
    void init();
    GLuint firstUnusedParticle();
    void respawnParticle(Particle& particle);
};

#endif