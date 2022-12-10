#include <learnopengl/shader.h>
#include <learnopengl/model.h>

#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "fft.h"

#define VERTEX_NUM 100

typedef glm::vec2 vector2;
typedef glm::vec3 vector3;

struct vertex_ocean {
    GLfloat   x, y, z; // vertex
    GLfloat  nx, ny, nz; // normal
    GLfloat  tx, ty; //texcoords 
    GLfloat   a, b, c; // htilde0
    GLfloat  _a, _b, _c; // htilde0mk conjugate
    GLfloat  ox, oy, oz; // original position
};

struct complex_vector_normal {  // structure used with discrete fourier transform
    std::complex<float> h;      // wave height
    glm::vec2 D;      // displacement
    glm::vec3 n;      // normal
};

class cOcean {
private:
    bool geometry;                          // flag to render geometry or surface

    float g;                                // gravity constant
    int N, Nplus1;                          // dimension -- N should be a power of 2
    float A;                                // phillips spectrum parameter -- affects heights of waves
    glm::vec2 w;                              // wind parameter
    float length;                           // length parameter
    std::complex<float>* h_tilde,                       // for fast fourier transform
        * h_tilde_slopex, * h_tilde_slopez,
        * h_tilde_dx, * h_tilde_dz;
    cFFT* fft;                              // fast fourier transform
    float stride = 1.0f;

    vertex_ocean* vertices;                 // vertices for vertex buffer object
    unsigned int* indices;                  // indicies for vertex buffer object
    unsigned int indices_count;             // number of indices to render
    GLuint vao, vbo_vertices, vbo_indices;       // vertex buffer objects

    GLuint glProgram; // shaders

    string water_path = "../resources/textures/water/water_textures_2k.png";
    string normal_path = "../resources/textures/water/water_textures_2k_normal.png";
    unsigned int watermapTexture, normalTexture;
    unsigned int loadmap(string path, unsigned int mode);
    const glm::vec3 position = glm::vec3(-30.0f, 0.0f, -25.0f);
    glm::mat4 model;
    //GLint vertex, normal, texture, light_position, projection, view, model; // attributes and uniforms

protected:
public:
    Shader ocean_shader;
    cOcean(const int N = 128, const float A = 0.005f, const glm::vec2 w = glm::vec2(10, 0), const float length = 25.0f, bool geometry = false);
    ~cOcean();
    void release();

    float dispersion(int n_prime, int m_prime);     // deep water
    float phillips(int n_prime, int m_prime);       // phillips spectrum
    std::complex<float> hTilde_0(int n_prime, int m_prime);
    std::complex<float> hTilde(float t, int n_prime, int m_prime);
    complex_vector_normal h_D_and_n(vector2 x, float t);
    void evaluateWaves(float t);
    void evaluateWavesFFT(float t);
    void render(float t, glm::vec3 light_pos, glm::mat4 Projection, glm::mat4 View, glm::mat4 Model, bool use_fft);
};
