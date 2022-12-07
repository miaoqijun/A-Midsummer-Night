#include "water.h"

Water::Water() {
    load_models();

    stbi_set_flip_vertically_on_load(true);
    watermapTexture = loadmap(water_path, GL_MIRRORED_REPEAT);
    normalTexture = loadmap(normal_path, GL_MIRRORED_REPEAT);
    stbi_set_flip_vertically_on_load(false);
    water_shader.use();
    glUniform1i(glGetUniformLocation(water_shader.ID, "texture_albedo"), 5);
    glUniform1i(glGetUniformLocation(water_shader.ID, "texture_normal"), 6);
}

void Water::load_models()
{
    //grass
    water_shader = Shader(water_vs_path, water_fs_path, water_gs_path);
    int count = VERTEX_COUNT * VERTEX_COUNT;
    float* vertices = new float[count * 5];
    unsigned int* indices = new unsigned int[6 * (VERTEX_COUNT - 1) * (VERTEX_COUNT - 1)];

    int pointer = 0;
    for (int i = 0; i < VERTEX_COUNT; i++) {
        for (int j = 0; j < VERTEX_COUNT; j++) {
            // 生成顶点数据
            vertices[pointer * 5] = (float)j / ((float)VERTEX_COUNT - 1) * SIZE - SIZE / 2;
            vertices[pointer * 5 + 1] = 0;
            vertices[pointer * 5 + 2] = (float)i / ((float)VERTEX_COUNT - 1) * SIZE - SIZE / 2;
            //std::cout << vertices[pointer * 3] << " " << vertices[pointer * 3 + 1] << " " << vertices[pointer * 3 + 2] << " ";

            // 纹理坐标
            vertices[pointer * 5 + 3] = (float)j / 10000;
            vertices[pointer * 5 + 4] = (float)i / 10000;

            pointer++;
        }
    }

    pointer = 0;
    for (int gz = 0; gz < VERTEX_COUNT - 1; gz++) {
        for (int gx = 0; gx < VERTEX_COUNT - 1; gx++) {
            int topLeft = (gz * VERTEX_COUNT) + gx;
            int topRight = topLeft + 1;
            int bottomLeft = ((gz + 1) * VERTEX_COUNT) + gx;
            int bottomRight = bottomLeft + 1;
            indices[pointer++] = topLeft;
            indices[pointer++] = bottomLeft;
            indices[pointer++] = topRight;

            indices[pointer++] = topRight;
            indices[pointer++] = bottomLeft;
            indices[pointer++] = bottomRight;
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * count, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6 * (VERTEX_COUNT - 1) * (VERTEX_COUNT - 1), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Water::render(glm::mat4 view, glm::mat4 projection,double time)
{
    // reset viewport
    //glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // render grass
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, watermapTexture);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    water_shader.setMat4("model", model);
    water_shader.setMat4("view", view);
    water_shader.setMat4("projection", projection);
    water_shader.setFloat("time", (float)time);
    glBindVertexArray(VAO);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, 6 * (VERTEX_COUNT - 1) * (VERTEX_COUNT - 1), GL_UNSIGNED_INT, 0);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

unsigned int Water::loadmap(string path, unsigned int mode)
{
    string filename = string(path);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}