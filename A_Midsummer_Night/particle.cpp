#include "particle.h"
#include <time.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

static void bindTexture(unsigned int& texture, const char* name)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    unsigned char* data;

    data = stbi_load(name, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}
ParticleGenerator::ParticleGenerator()
    : lastUsedParticle(0)
{
    this->init();
    shader = Shader(vs_path, fs_path, gs_path);
    model = glm::mat4(1.0f);
    model = glm::translate(model, position); // translate it down so it's at the center of the scene
    model = glm::scale(model, scale);	// it's a bit too big for our scene, so scale it down
    int n = 10;
    float Adj_value = 0.05f;
    float radius = 0.7f;//火焰地区半径
    for (unsigned int i = 0; i < amount; i++)
    {
        glm::vec3 record(0.0f);
        for (int j = 0; j < n; j++) //模拟高斯分布计算粒子位置
        {
            record.x += (2.0f * float(rand()) / float(RAND_MAX) - 1.0f);
            record.z += (2.0f * float(rand()) / float(RAND_MAX) - 1.0f);
        }
        record.x *= radius;
        record.z *= radius;
        record.y = 0.0f;
        particles[i].Position = record;
        particles[i].Velocity = (MaxVeloc - MinVeloc) * (float(rand()) / float(RAND_MAX)) + MinVeloc;//在最大最小速度之间随机选择
        particles[i].Color = INIT_COLOR;
        particles[i].size = INIT_SIZE;//发射器粒子大小
        //在最短最长寿命之间随机选择
        particles[i].Life = (MAX_LIFE - MIN_LIFE) * (float(rand()) / float(RAND_MAX)) + MIN_LIFE;
        float dist = sqrt(record.x * record.x + record.z * record.z);
        if (dist <= radius)
            particles[i].Life *= 1.3f;
        particles[i].Age = 0.0f;
        particles[i].Color.a = 1.0f / ((particles[i].Age - particles[i].Life / 2) * (particles[i].Age - particles[i].Life / 2) + 1.0f);
    }
}
void ParticleGenerator::Update(GLfloat dt)
{
    for (GLuint i = 0; i < unsigned int(dt * 1000); i++)
    {
        int unusedParticle = firstUnusedParticle();
        respawnParticle(particles[unusedParticle]);
    }

    float factor;
    for (GLuint i = 0; i < amount; i++)
    {
        Particle& p = particles[i];
        factor = 1.0f / ((p.Age - p.Life / 2) * (p.Age - p.Life / 2) + 1.0f);
        p.Age += dt; // reduce life
        if (p.Age < p.Life)
        {
            p.Velocity += ACC_SPEED;
            p.Position += p.Velocity * dt;
            p.Color.a = factor;
            p.size = factor * INIT_SIZE * 0.1f;
        }
    }

    float sum_coord[3] = {};
    int n = 0;
    for (Particle particle : particles)
    {
        if (particle.Age < particle.Life)
        {
            sum_coord[0] += particle.Position.x;
            sum_coord[1] += particle.Position.y;
            sum_coord[2] += particle.Position.z;
            n++;
        }
    }
    sum_coord[0] /= n;
    sum_coord[1] /= n;
    sum_coord[2] /= n;
    avg_coord = glm::vec3(sum_coord[0] / n, sum_coord[1] / n, sum_coord[2] / n);
}
void ParticleGenerator::Draw(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPos)
{
    glEnable(GL_BLEND);
    glDepthFunc(GL_ALWAYS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    vertices.clear();
    for (Particle particle : particles)
    {
        if (particle.Age < particle.Life)
        {
            vertices.push_back(particle.Position.x);
            vertices.push_back(particle.Position.y);
            vertices.push_back(particle.Position.z);
            vertices.push_back(particle.Color.x);
            vertices.push_back(particle.Color.y);
            vertices.push_back(particle.Color.z);
            vertices.push_back(particle.Color.a);
            vertices.push_back(particle.size);
            if (particle.Age / particle.Life < 0.6)
                vertices.push_back(0.5);
            else
                vertices.push_back(1.5);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setMat4("model", model);
    shader.setVec3("gCameraPos", viewPos);
    shader.setInt("flame", 0);
    shader.setInt("Round", 1);
    unsigned int texture, texture1;
    bindTexture(texture, "../resources/textures/fire/particle.bmp");
    bindTexture(texture1, "../resources/textures/fire/flame.bmp");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, GLsizei(vertices.size()) / 8);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
GLuint ParticleGenerator::firstUnusedParticle()
{
    for (GLuint i = lastUsedParticle; i < this->amount; i++) {
        if (particles[i].Age >= particles[i].Life) {
            lastUsedParticle = i;
            return i;
        }
    }
    for (GLuint i = 0; i < lastUsedParticle; i++) {
        if (particles[i].Age >= particles[i].Life) {
            lastUsedParticle = i;
            return i;
        }
    }

    lastUsedParticle = 0;
    return 0;
}
void ParticleGenerator::respawnParticle(Particle& particle)
{
    glm::vec3 record(0.0f);
    float radius = 0.7f;//火焰地区半径
    //1. 先计算位置
    for (int j = 0; j < 10; j++) //模拟高斯分布计算粒子位置
    {
        record.x += (2.0f * float(rand()) / float(RAND_MAX) - 1.0f);
        record.z += (2.0f * float(rand()) / float(RAND_MAX) - 1.0f);
    }
    record.x *= radius;
    record.z *= radius;
    record.y = 0.0f;
    particle.Position = record;
    //2. 再更新速度
    particle.Velocity = (MaxVeloc - MinVeloc) * (float(rand()) / float(RAND_MAX)) + MinVeloc;//在最大最小速度之间随机选择
    //3. 颜色
    particle.Color = INIT_COLOR;
    particle.size = INIT_SIZE;//发射器粒子大小
    //在最短最长寿命之间随机选择
    particle.Life = (MAX_LIFE - MIN_LIFE) * (float(rand()) / float(RAND_MAX)) + MIN_LIFE;
    float dist = sqrt(record.x * record.x + record.z * record.z);
    if (dist <= radius)
        particle.Life *= 1.3f;
    particle.Age = 0.0f;
    particle.Color.a = 1.0f / ((particle.Age - particle.Life / 2) * (particle.Age - particle.Life / 2) + 1.0f);
}
void ParticleGenerator::init()
{
    for (GLuint i = 0; i < this->amount; i++)
        this->particles.push_back(Particle());
}

glm::vec3 ParticleGenerator::get_light_position()
{
    if (vertices.size() > 0)
        return position + glm::vec3(avg_coord.x * 200.0f, avg_coord.y, avg_coord.z * 200.0f);
    else
        return position;
}