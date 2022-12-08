#include <learnopengl/shader.h>
#include <learnopengl/model.h>

#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "particle.h"
#include "water.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "config.h"

#define POINT_LIGHT_NUM 3

// 世界坐标&模型
struct WorldModel {
    glm::vec3 position;
    glm::vec3 scale;
    float angle[2];
    glm::vec3 rotateAxis[2];
    Model model;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant, linear, quadratic;
};

class Scene {
private:
    const char* PBR_vs_path = "shaders/PBR_shaders/PBR_vs.glsl", * PBR_fs_path = "shaders/PBR_shaders/PBR_fs.glsl";
    const char* shadow_vs_path = "shaders/shadows_shaders/point_shadows_vs.glsl", * shadow_fs_path = "shaders/shadows_shaders/point_shadows_fs.glsl", * shadow_gs_path = "shaders/shadows_shaders/point_shadows_gs.glsl";
    const char* SSR_vs_path = "shaders/SSR_shaders/SSR_vs.glsl", * SSR_fs_path = "shaders/SSR_shaders/SSR_fs.glsl";
    Shader PBR_shader, depth_shader, SSR_shader;
    PointLight point_lights[POINT_LIGHT_NUM] = {
        glm::vec3(-3.0f, 2.0f, 0.0f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(1.5f, 2.0f, 2.5f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(-1.35f, 0.86f, -0.018f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f, 0.09f, 0.032f,
    };
    PointLight fire = { glm::vec3(-1.35f, 0.86f, -0.018f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f, 0.09f, 0.032f };
    void set_shaders_parameters();
    std::vector<WorldModel> models;
    const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    GLuint depthMapFBO[POINT_LIGHT_NUM], depthCubemap[POINT_LIGHT_NUM];
    unsigned int framebuffer, textureColorbuffer, depthBuffer;
    FireParticleGenerator FireParticle;
    SmokeParticleGenerator SmokeParticle;
    Water water;
    void load_models();

public:
	Scene();
    void render(glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, int shadow_mode, bool SSR_test, bool SSR_ON, bool scatter_ON, float delattime, float totalTime);
};