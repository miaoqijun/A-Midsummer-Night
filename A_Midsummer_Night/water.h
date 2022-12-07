#include <learnopengl/shader.h>
#include <learnopengl/model.h>

#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Water {
private:
    void load_models();
    const char* water_vs_path = "shaders/water_shaders/water_vs.glsl", * water_fs_path = "shaders/water_shaders/water_fs.glsl", * water_gs_path = "shaders/water_shaders/water_gs.glsl";
    unsigned int VBO, VAO, EBO;
    const int VERTEX_COUNT = 512, SIZE = 100;
    string water_path = "../resources/textures/water/water_textures_2k.png";
    string normal_path = "../resources/textures/water/water_textures_2k_normal.png";
    unsigned int watermapTexture, normalTexture;
    unsigned int loadmap(string path, unsigned int mode);
    const glm::vec3 position = glm::vec3(0.0f, -0.05f, 0.0f);
    glm::mat4 model;
public:
    Shader water_shader;
    Water();
    void render(glm::mat4 view, glm::mat4 projection,double time);
};
