#include "scene.h"
#include <fstream>
#include <string>

void Scene::set_shaders_parameters()
{
    ifstream fin;
    ofstream fout;
    int i;

    /* PBR_fs.glsl */
    fin.open(PBR_fs_path);
    string s1;
    i = 0;
    while (true) {
        string line;
        getline(fin, line);
        if (i == 2)
            s1 += "#define SHADOWS_SAMPLES " + to_string(SHADOWS_SAMPLES) + '\n';
        else {
            s1 += line;
            if (fin.eof())
                break;
            s1 += '\n';
        }
        i++;
    }
    fout.open(PBR_fs_path);
    fout << s1;
    fin.close();
    fout.close();

    /* SSR_fs.glsl */
    fin.open(SSR_fs_path);
    string s2;
    i = 0;
    while (true) {
        string line;
        getline(fin, line);
        if (i == 2)
            s2 += "#define SSR_SAMPLES " + to_string(SSR_SAMPLES) + '\n';
        else if(i == 3)
            s2 += "#define SCATTER_SAMPLES " + to_string(SCATTER_SAMPLES) + '\n';
        else {
            s2 += line;
            if (fin.eof())
                break;
            s2 += '\n';
        }
        i++;
    }
    fout.open(SSR_fs_path);
    fout << s2;
    fin.close();
    fout.close();
}

Scene::Scene()
{
    set_shaders_parameters();

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
        PBR_shader.use();
        PBR_shader.setInt("depthMap[" + to_string(i) + "]", i);
        /*water.water_shader.use();
        water.water_shader.setInt("depthMap[" + to_string(i) + "]", i);*/
    }

    SSR_shader = Shader(SSR_vs_path, SSR_fs_path);
    SSR_shader.use();
    SSR_shader.setInt("colorMap", 0);
    SSR_shader.setInt("depthMap", 1);
    SSR_shader.setInt("lightDepthMap", 2);

    Ocean.ocean_shader.use();
    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        Ocean.ocean_shader.setVec3("pointLights[" + to_string(i) + "].position", point_lights[i].position);
        Ocean.ocean_shader.setVec3("pointLights[" + to_string(i) + "].ambient", point_lights[i].ambient);
        Ocean.ocean_shader.setVec3("pointLights[" + to_string(i) + "].diffuse", point_lights[i].diffuse);
        Ocean.ocean_shader.setVec3("pointLights[" + to_string(i) + "].specular", point_lights[i].specular);
        Ocean.ocean_shader.setFloat("pointLights[" + to_string(i) + "].constant", point_lights[i].constant);
        Ocean.ocean_shader.setFloat("pointLights[" + to_string(i) + "].linear", point_lights[i].linear);
        Ocean.ocean_shader.setFloat("pointLights[" + to_string(i) + "].quadratic", point_lights[i].quadratic);
    }

    // framebuffer configuration
    // -------------------------
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &depthBuffer);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::load_models()
{
    WorldModel house = {
        glm::vec3(0.0f, 0.1f, 0.0f),
        glm::vec3(0.3f, 0.3f, 0.3f),
        glm::radians(-90.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/House/highpoly_town_house_01.obj")
    };
    models.push_back(house);

    WorldModel sofa = {
        glm::vec3(1.0f, 0.72f, 0.5f),
        glm::vec3(0.005f, 0.005f, 0.005f),
        glm::radians(-90.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Sofa/sofa.obj")
    };
    models.push_back(sofa);

    WorldModel carpet = {
        glm::vec3(-0.8f, 0.72f, 0.0f),
        glm::vec3(0.005f, 0.005f, 0.005f),
        glm::radians(-90.0f), glm::radians(90.0f),
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        Model("../resources/objects/Carpet/Carpet.obj")
    };
    models.push_back(carpet);

    WorldModel gramophone = {
        glm::vec3(-0.8f, 0.72f, 0.2f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::radians(-30.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Gramophone/gramophone.obj")
    };
    models.push_back(gramophone);

    WorldModel teapot = {
        glm::vec3(-0.8f, 0.72f, -0.3f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::radians(-90.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Teapot/teapot.obj")
    };
    models.push_back(teapot);

    WorldModel ground = {
        glm::vec3(0.0f, -1.75f, 0.0f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::radians(180.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Ground/Ground.obj")
    };
    models.push_back(ground);

    WorldModel table = {
        glm::vec3(-0.8f, 0.1f, 3.5f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::radians(0.0f), glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Table/table.obj")
    };
    models.push_back(table);

    WorldModel chair = {
        glm::vec3(0.0f, 0.4f, 3.5f),
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::radians(180.0f), glm::radians(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Chair/chair.obj")
    };
    models.push_back(chair);

    WorldModel roadLamp = {
        glm::vec3(-3.0f, 0.1f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::radians(0.0f), glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/roadLamp/Street_lamp_1.obj")
    };
    roadLamp.position = glm::vec3(-3.0f, 0.1f, 0.0f);
    models.push_back(roadLamp);
    roadLamp.position = glm::vec3(1.5f, 0.1f, 2.5f);
    models.push_back(roadLamp);

    WorldModel mug = {
        glm::vec3(-1.0f, 0.63f, 3.5f),
        glm::vec3(0.06f, 0.06f, 0.06f),
        glm::radians(0.0f), glm::radians(0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        Model("../resources/objects/Mug/Mug.obj")
    };
    models.push_back(mug);
}

void Scene::render(glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, int shadow_mode, bool SSR_test, bool SSR_ON, bool scatter_ON, float delatTime, float totalTime)
{
    FireParticle.Update(delatTime);
    SmokeParticle.Update(delatTime / 8);
    point_lights[POINT_LIGHT_NUM - 1].position = FireParticle.get_light_position();

    // 0. Create depth cubemap transformation matrices
    GLfloat aspect = (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT;
    GLfloat far = 25.0f;
    std::vector<glm::mat4> shadowTransforms[POINT_LIGHT_NUM];
    
    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        GLfloat near = i < POINT_LIGHT_NUM - 1 ? 1.0f : 0.1f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
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
        //models
        for (int j = 0; j < models.size(); j++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, models[j].position); // translate it down so it's at the center of the scene 
            model = glm::rotate(model, models[j].angle[1], models[j].rotateAxis[1]);
            model = glm::rotate(model, models[j].angle[0], models[j].rotateAxis[0]);
            model = glm::scale(model, models[j].scale);	// it's a bit too big for our scene, so scale it down
            depth_shader.setMat4("model", model);
            models[j].model.Draw(depth_shader);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // reset viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap[i]);
    }

    // render the loaded model
    for (int i = 0; i < models.size(); i++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, models[i].position); // translate it down so it's at the center of the scene
        model = glm::rotate(model, models[i].angle[1], models[i].rotateAxis[1]);
        model = glm::rotate(model, models[i].angle[0], models[i].rotateAxis[0]);
        model = glm::scale(model, models[i].scale);	// it's a bit too big for our scene, so scale it down
        PBR_shader.use();
        PBR_shader.setVec3("pointLights[" + to_string(POINT_LIGHT_NUM - 1) + "].position", point_lights[POINT_LIGHT_NUM - 1].position);
        PBR_shader.setInt("shadow_mode", shadow_mode);
        PBR_shader.setBool("SSR_test", SSR_test);
        PBR_shader.setVec3("viewPos", viewPos);
        PBR_shader.setMat4("model", model);
        PBR_shader.setMat4("projection", projection);
        PBR_shader.setMat4("view", view);
        PBR_shader.setFloat("far_plane", far);
        models[i].model.Draw(PBR_shader);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    FireParticle.Draw(projection, view, viewPos);
    SmokeParticle.Draw(projection, view);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap[0]);
    // render the loaded model
    for (int i = 0; i < models.size(); i++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, models[i].position); // translate it down so it's at the center of the scene
        model = glm::rotate(model, models[i].angle[1], models[i].rotateAxis[1]);
        model = glm::rotate(model, models[i].angle[0], models[i].rotateAxis[0]);
        model = glm::scale(model, models[i].scale);	// it's a bit too big for our scene, so scale it down
        SSR_shader.use();
        SSR_shader.setVec3("viewPos", viewPos);
        SSR_shader.setMat4("model", model);
        SSR_shader.setMat4("projection", projection);
        SSR_shader.setMat4("view", view);
        SSR_shader.setBool("SSR_test", SSR_test);
        SSR_shader.setBool("SSR_ON", SSR_ON);
        if (i < 5)  //only scatter in house
            SSR_shader.setBool("scatter_ON", scatter_ON);
        else
            SSR_shader.setBool("scatter_ON", false);
        SSR_shader.setFloat("far_plane", far);
        SSR_shader.setVec3("lightPos", point_lights[0].position);
        models[i].model.Draw(SSR_shader);
    }

    for (int i = 0; i < POINT_LIGHT_NUM; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap[i]);
    }
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    Ocean.ocean_shader.use();
    Ocean.ocean_shader.setVec3("pointLights[" + to_string(POINT_LIGHT_NUM - 1) + "].position", point_lights[POINT_LIGHT_NUM - 1].position);
    Ocean.ocean_shader.setInt("shadow_mode", shadow_mode);
    Ocean.ocean_shader.setVec3("viewPos", viewPos);
    Ocean.ocean_shader.setFloat("far_plane", far);
    Ocean.render(totalTime, viewPos, projection, view, glm::mat4(1.0f), 1);
}