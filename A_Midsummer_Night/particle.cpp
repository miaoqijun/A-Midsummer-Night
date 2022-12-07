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
FireParticleGenerator::FireParticleGenerator()
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
        particles[i].Velocity = (FIRE_MaxVeloc - FIRE_MinVeloc) * (float(rand()) / float(RAND_MAX)) + FIRE_MinVeloc;//在最大最小速度之间随机选择
        particles[i].Color = FIRE_INIT_COLOR;
        particles[i].size = FIRE_INIT_SIZE;//发射器粒子大小
        //在最短最长寿命之间随机选择
        particles[i].Life = (FIRE_MAX_LIFE - FIRE_MIN_LIFE) * (float(rand()) / float(RAND_MAX)) + FIRE_MIN_LIFE;
        float dist = sqrt(record.x * record.x + record.z * record.z);
        if (dist <= radius)
            particles[i].Life *= 1.3f;
        particles[i].Age = 0.0f;
        particles[i].Color.a = 1.0f / ((particles[i].Age - particles[i].Life / 2) * (particles[i].Age - particles[i].Life / 2) + 1.0f);
    }
}
void FireParticleGenerator::Update(GLfloat dt)
{
    for (GLuint i = 0; i < unsigned int(dt * 1000); i++)
    {
        int unusedParticle = firstUnusedParticle();
        respawnParticle(particles[unusedParticle]);
    }

    float factor;
    for (GLuint i = 0; i < amount; i++)
    {
        FireParticle& p = particles[i];
        factor = 1.0f / ((p.Age - p.Life / 2) * (p.Age - p.Life / 2) + 1.0f);
        p.Age += dt; // reduce life
        if (p.Age < p.Life)
        {
            p.Velocity += FIRE_ACC_SPEED;
            p.Position += p.Velocity * dt;
            p.Color.a = factor;
            p.size = factor * FIRE_INIT_SIZE * 0.1f;
        }
    }

    float sum_coord[3] = {};
    int n = 0;
    for (FireParticle particle : particles)
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
void FireParticleGenerator::Draw(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPos)
{
    glEnable(GL_BLEND);
    glDepthFunc(GL_ALWAYS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    vertices.clear();
    for (FireParticle particle : particles)
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
GLuint FireParticleGenerator::firstUnusedParticle()
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
void FireParticleGenerator::respawnParticle(FireParticle& particle)
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
    particle.Velocity = (FIRE_MaxVeloc - FIRE_MinVeloc) * (float(rand()) / float(RAND_MAX)) + FIRE_MinVeloc;//在最大最小速度之间随机选择
    //3. 颜色
    particle.Color = FIRE_INIT_COLOR;
    particle.size = FIRE_INIT_SIZE;//发射器粒子大小
    //在最短最长寿命之间随机选择
    particle.Life = (FIRE_MAX_LIFE - FIRE_MIN_LIFE) * (float(rand()) / float(RAND_MAX)) + FIRE_MIN_LIFE;
    float dist = sqrt(record.x * record.x + record.z * record.z);
    if (dist <= radius)
        particle.Life *= 1.3f;
    particle.Age = 0.0f;
    particle.Color.a = 1.0f / ((particle.Age - particle.Life / 2) * (particle.Age - particle.Life / 2) + 1.0f);
}
void FireParticleGenerator::init()
{
    for (GLuint i = 0; i < this->amount; i++)
        this->particles.push_back(FireParticle());
}

glm::vec3 FireParticleGenerator::get_light_position()
{
    if (vertices.size() > 0)
        return position + glm::vec3(avg_coord.x * 30.0f, avg_coord.y, avg_coord.z * 30.0f);// +glm::vec3(0.5f, 0.0f, 0.0f);
    else
        return position;
}

/*
* 下面是Smoke类的函数定义
*/
SmokeParticleGenerator::SmokeParticleGenerator()
    : lastUsedParticle(0)
{
    init();

}
void SmokeParticleGenerator::init()
{
    srand(unsigned(time(NULL)));
    for (GLuint i = 0; i < this->amount; ++i)
        this->particles.push_back(SmokeParticle());
    float radius = 0.5f;
    float random;
    float polar_angle;
    float azimuthAngle;
    shader = Shader(vs_path, fs_path);
    model = glm::mat4(1.0f);
    model = glm::translate(model, position); // translate it down so it's at the center of the scene
    model = glm::scale(model, scale);	// it's a bit too big for our scene, so scale it down
    for (int i = 0; i < particles.size(); i++)
    {
        random = (float)rand() / ((float)RAND_MAX);
        polar_angle = (float)rand() / ((float)RAND_MAX);
        polar_angle *= 2 * PI;
        particles[i].Position.x = random * radius * cos(polar_angle);
        particles[i].Position.z = random * radius * sin(polar_angle);
        particles[i].Position.y = 0.0f;
        azimuthAngle = (float)rand() / ((float)RAND_MAX);
        azimuthAngle *= MAX_EMISSION_ANGLE;
        particles[i].Velocity.x = sin(azimuthAngle) * cos(polar_angle) * 5.0f;
        particles[i].Velocity.y = cos(azimuthAngle) * 5.0f;
        particles[i].Velocity.z = sin(azimuthAngle) * sin(polar_angle) * 5.0f;
        particles[i].Life = (SMOKE_MAX_LIFE - SMOKE_MIN_LIFE) * (float)rand() / ((float)RAND_MAX) + SMOKE_MIN_LIFE;
        particles[i].Color = SMOKE_INIT_COLOR;
        particles[i].Delay = (float)rand() / ((float)RAND_MAX);
        particles[i].Age = 0.0f;
        particles[i].Factor = 1.0f;
    }
}
GLuint SmokeParticleGenerator::firstUnusedParticle()
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
void SmokeParticleGenerator::respawnParticle(SmokeParticle& particle)
{
    float radius = 0.5f;
    float random;
    float polar_angle;
    float azimuthAngle;

    random = (float)rand() / ((float)RAND_MAX);
    polar_angle = (float)rand() / ((float)RAND_MAX);
    polar_angle *= 2 * PI;
    particle.Position.x = random * radius * cos(polar_angle);
    particle.Position.z = random * radius * sin(polar_angle);
    particle.Position.y = 0.0f;
    azimuthAngle = (float)rand() / ((float)RAND_MAX);
    azimuthAngle *= MAX_EMISSION_ANGLE;
    particle.Velocity.x = sin(azimuthAngle) * cos(polar_angle) * 5.0f;
    particle.Velocity.y = cos(azimuthAngle) * 5.0f;
    particle.Velocity.z = sin(azimuthAngle) * sin(polar_angle) * 5.0f;

    particle.Life = (float)rand() / ((float)RAND_MAX);
    particle.Delay = (float)rand() / ((float)RAND_MAX);
    particle.Age = 0.0f;
    particle.Factor = 1.0f;
}
void SmokeParticleGenerator::Update(GLfloat dt)
{
    for (GLuint i = 0; i < SMOKE_NEW_PARTICLES; ++i)
    {
        int unusedParticle = firstUnusedParticle();
        respawnParticle(particles[unusedParticle]);
    }

    for (GLuint i = 0; i < amount; i++)
    {
        SmokeParticle& p = particles[i];
        p.Age += dt;
        if (p.Age < p.Life && p.Age > p.Delay)
        {
            p.Factor = 1.0f - ((p.Age - p.Delay) / p.Life);
            p.Position += p.Velocity * dt + U_GRAVITY * dt * dt * 10.0f;
        }
    }
}
void SmokeParticleGenerator::Draw(glm::mat4 projection, glm::mat4 view)
{
    glEnable(GL_BLEND);
    glDepthFunc(GL_ALWAYS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    std::vector<float> vertices;
    for (SmokeParticle particle : particles)
    {
        if (particle.Age < particle.Life && particle.Age > particle.Delay)
        {
            vertices.push_back(particle.Position.x);
            vertices.push_back(particle.Position.y);
            vertices.push_back(particle.Position.z);
            vertices.push_back(particle.Color.x);
            vertices.push_back(particle.Color.y);
            vertices.push_back(particle.Color.z);
            vertices.push_back(particle.Color.a);
            vertices.push_back(particle.Factor);
        }
    }

    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setMat4("model", model);
    //shader.setVec3("gCameraPos", viewPos);
    unsigned int texture;
    bindTexture(texture, "../resources/textures/smoke/smoke.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, GLsizei(vertices.size() / 8));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}