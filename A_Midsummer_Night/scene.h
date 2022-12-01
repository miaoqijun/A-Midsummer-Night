#include <learnopengl/shader.h>
#include <learnopengl/model.h>

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define POINT_LIGHT_NUM 2

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// 世界坐标&模型
struct WorldModel {
    glm::vec3 position;
    glm::vec3 scale;
    float angle;
    glm::vec3 rotateAxis;
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
    const char* PBR_vs_path = "PBR_vs.glsl", * PBR_fs_path = "PBR_fs.glsl";
    const char* shadow_vs_path = "point_shadows_vs.glsl", * shadow_fs_path = "point_shadows_fs.glsl", * shadow_gs_path = "point_shadows_gs.glsl";
	Shader PBR_shader, depth_shader;
    PointLight point_lights[POINT_LIGHT_NUM] = {
        glm::vec3(-2.9f, 2.0f, -1.0f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(-2.9f, 2.0f, 1.0f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f, 0.09f, 0.032f
    };
    std::vector<WorldModel> models;
    const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    GLuint depthMapFBO[POINT_LIGHT_NUM], depthCubemap[POINT_LIGHT_NUM];
    void load_models();
public:
	Scene();
    void render(glm::mat4 view, glm::mat4 projection);
};