#include "scene.h"

Scene::Scene()
{
    Phong_shader = Shader(Phong_vs_path, Phong_fs_path);
    Phong_shader.use();
    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        Phong_shader.setVec3("pointLights[" + to_string(i) + "].position", point_lights[i].position);
        Phong_shader.setVec3("pointLights[" + to_string(i) + "].ambient", point_lights[i].ambient);
        Phong_shader.setVec3("pointLights[" + to_string(i) + "].diffuse", point_lights[i].diffuse);
        Phong_shader.setVec3("pointLights[" + to_string(i) + "].specular", point_lights[i].specular);
        Phong_shader.setFloat("pointLights[" + to_string(i) + "].constant", point_lights[i].constant);
        Phong_shader.setFloat("pointLights[" + to_string(i) + "].linear", point_lights[i].linear);
        Phong_shader.setFloat("pointLights[" + to_string(i) + "].quadratic", point_lights[i].quadratic);
    }

    PBR_shader = Shader(PBR_vs_path, PBR_fs_path);
    PBR_shader.use();
    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        PBR_shader.setVec3("pointLights[" + to_string(i) + "].position", point_lights[i].position);
        PBR_shader.setVec3("pointLights[" + to_string(i) + "].ambient", point_lights[i].ambient);
        PBR_shader.setVec3("pointLights[" + to_string(i) + "].diffuse", point_lights[i].diffuse);
        PBR_shader.setVec3("pointLights[" + to_string(i) + "].specular", point_lights[i].specular);
        PBR_shader.setFloat("pointLights[" + to_string(i) + "].constant", point_lights[i].constant);
        PBR_shader.setFloat("pointLights[" + to_string(i) + "].linear", point_lights[i].linear);
        PBR_shader.setFloat("pointLights[" + to_string(i) + "].quadratic", point_lights[i].quadratic);
    }

    load_models();

    depth_shader = Shader(shadow_vs_path, shadow_fs_path, shadow_gs_path);

    // Configure depth map FBO
    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        glGenFramebuffers(1, &depthMapFBO[i]);
        // Create depth cubemap texture
        glGenTextures(1, &depthCubemap[i]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap[i]);
        for (GLuint i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // Attach cubemap as depth map FBO's color buffer
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Phong_shader.use();
        Phong_shader.setInt("depthMap[" + to_string(i) + "]", i);
        PBR_shader.use();
        PBR_shader.setInt("depthMap[" + to_string(i) + "]", i);
    }
}

void Scene::load_models()
{
    WorldModel house = {
        glm::vec3(0.0f, 0.1f, 0.0f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::radians(-90.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        Model("../resources/objects/House/highpoly_town_house_01.obj"),
        true
    };
    models.push_back(house);

    WorldModel grass = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.01f, 0.01f, 0.01f),
        glm::radians(-90.0f),
        glm::vec3(0.1f, 0.0f, 0.0f),
        Model("../resources/objects/Grass_Patch/10450_Rectangular_Grass_Patch_v1_iterations-2.obj"),
        true
    };
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            grass.position = glm::vec3(i * 1.0f, j * 1.0f, 0.0f);
            models.push_back(grass);
        }
    }

    WorldModel bonfire = {
        glm::vec3(0.5f, 0.15f, 3.5f),
        glm::vec3(0.15f, 0.15f, 0.15f),
        glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Bonfire/Bonfire model 1.obj"),
        false
    };
    models.push_back(bonfire);

    WorldModel tree = {
        glm::vec3(3.0f, 0.0f, 0.2f),
        glm::vec3(0.02f, 0.02f, 0.02f),
        glm::radians(-90.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Tree1/OrnamentalPlant_001.obj"),
        false
    };
    for (int i = -2; i <= 2; i++) {
        tree.position = glm::vec3(3.0f, i * 1.0f, 0.2f);
        models.push_back(tree);
    }

    WorldModel blender = {
        glm::vec3(-1.5f, 0.1f, 0.35f),
        glm::vec3(0.02f, 0.02f, 0.02f),
        glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/blender/klupa117.obj"),
        true
    };
    models.push_back(blender);

    WorldModel table = {
        glm::vec3(-0.3f, 0.1f, 3.5f),
        glm::vec3(0.02f, 0.02f, 0.02f),
        glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/dinner_table/3dstylish-fdb001.obj"),
        false
    };
    models.push_back(table);

    WorldModel roadLamp = {
        glm::vec3(-3.0f, 0.1f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/roadLamp/Street_lamp_1.obj"),
        true
    };
    roadLamp.position = glm::vec3(-3.0f, 0.1f, -1.0f);
    models.push_back(roadLamp);
    roadLamp.position = glm::vec3(-2.5f, 0.1f, 0.0f);
    models.push_back(roadLamp);
    roadLamp.position = glm::vec3(-3.0f, 0.1f, 1.0f);
    models.push_back(roadLamp);
}

void Scene::render(glm::mat4 view, glm::mat4 projection)
{
    // 0. Create depth cubemap transformation matrices
    GLfloat aspect = (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT;
    GLfloat near = 1.0f;
    GLfloat far = 25.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
    std::vector<glm::mat4> shadowTransforms[3];

    for (int i = 0; i < 3; i++) {
        shadowTransforms[i].push_back(shadowProj * glm::lookAt(point_lights[i].position, point_lights[i].position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms[i].push_back(shadowProj * glm::lookAt(point_lights[i].position, point_lights[i].position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms[i].push_back(shadowProj * glm::lookAt(point_lights[i].position, point_lights[i].position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms[i].push_back(shadowProj * glm::lookAt(point_lights[i].position, point_lights[i].position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms[i].push_back(shadowProj * glm::lookAt(point_lights[i].position, point_lights[i].position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms[i].push_back(shadowProj * glm::lookAt(point_lights[i].position, point_lights[i].position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

        // 1. Render scene to depth cubemap
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
        glClear(GL_DEPTH_BUFFER_BIT);
        depth_shader.use();
        for (GLuint j = 0; j < 6; ++j)
            glUniformMatrix4fv(glGetUniformLocation(depth_shader.ID, ("shadowMatrices[" + std::to_string(j) + "]").c_str()), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i][j]));
        glUniform1f(glGetUniformLocation(depth_shader.ID, "far_plane"), far);
        glUniform3fv(glGetUniformLocation(depth_shader.ID, "lightPos"), 1, &point_lights[i].position[0]);
        for (auto& worldModel : models) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, worldModel.angle, worldModel.rotateAxis);
            model = glm::translate(model, worldModel.position); // translate it down so it's at the center of the scene
            model = glm::scale(model, worldModel.scale);	// it's a bit too big for our scene, so scale it down
            depth_shader.setMat4("model", model);
            worldModel.model.Draw(depth_shader, false);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // reset viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < 3; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap[i]);
    }

    // render the loaded model
    for (auto& worldModel : models) {
        Shader* p_shader = worldModel.use_PBR ? &PBR_shader : &Phong_shader;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, worldModel.angle, worldModel.rotateAxis);
        model = glm::translate(model, worldModel.position); // translate it down so it's at the center of the scene
        model = glm::scale(model, worldModel.scale);	// it's a bit too big for our scene, so scale it down
        p_shader->use();
        p_shader->setMat4("model", model);
        p_shader->setMat4("projection", projection);
        p_shader->setMat4("view", view);
        p_shader->setFloat("far_plane", far);
        worldModel.model.Draw(*p_shader, worldModel.use_PBR);
    }
}