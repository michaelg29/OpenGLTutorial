/*
    CODE
    ENGINE (Scene)
    OPENGL
    GPU
*/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <stack>

#include <glm/glm.hpp>

#include "graphics/memory/framememory.hpp"
#include "graphics/memory/uniformmemory.hpp"

#include "graphics/models/cube.hpp"
#include "graphics/models/lamp.hpp"
#include "graphics/models/gun.hpp"
#include "graphics/models/sphere.hpp"
#include "graphics/models/box.hpp"
#include "graphics/models/plane.hpp"
#include "graphics/models/brickwall.hpp"

#include "graphics/objects/model.h"

#include "graphics/rendering/shader.h"
#include "graphics/rendering/texture.h"
#include "graphics/rendering/light.h"
#include "graphics/rendering/cubemap.h"
#include "graphics/rendering/text.h"

#include "physics/environment.h"

#include "io/keyboard.h"
#include "io/mouse.h"
#include "io/joystick.h"
#include "io/camera.h"

#include "algorithms/states.hpp"
#include "algorithms/ray.h"

#include "scene.h"

Scene scene;

void processInput(double dt);
void renderScene(Shader shader);

Camera cam;

//Joystick mainJ(0);

double dt = 0.0f; // tme btwn frames
double lastFrame = 0.0f; // time of last frame

Sphere sphere(10);
Cube cube(10);
Lamp lamp(4);
Brickwall wall;

std::string Shader::defaultDirectory = "assets/shaders";

#include "physics/collisionmesh.h"

void renderScene(Shader objectShader, Shader lampShader);
void renderObjects(Shader shader);
void renderLamps(Shader shader);
void processInput(double dt);
void launchItem();
void emitRay();
void keyChanged(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursorChanged(GLFWwindow* window, double _x, double _y);
void mouseButtonChanged(GLFWwindow* window, int button, int action, int mods);
void scrollChanged(GLFWwindow* window, double dx, double dy);

int main() {
    std::cout << "Hello, OpenGL!" << std::endl;

    // construct scene
    scene = Scene(3, 3, "OpenGL Tutorial", 1200, 720);
    // test if GLFW successfully started and created window
    if (!scene.init()) {
        std::cout << "Could not open window" << std::endl;
        scene.cleanup();
        return -1;
    }

    // set camera
    scene.cameras.push_back(&cam);
    scene.activeCamera = 0;

    // SHADERS===============================
    Shader::loadIntoDefault("defaultHead.gh");

    Shader shader(true, "instanced/instanced.vs", "object.fs");
    Shader lampShader(true, "instanced/instanced.vs", "lamp.fs");
    Shader boxShader(false, "instanced/box.vs", "instanced/box.fs");
    
    Shader dirShadowShader(false, "shadows/dirSpotShadow.vs",
        "shadows/dirShadow.fs");
    Shader spotShadowShader(false, "shadows/dirSpotShadow.vs",
        "shadows/pointSpotShadow.fs");
    Shader pointShadowShader(false, "shadows/pointShadow.vs",
        "shadows/pointSpotShadow.fs",
        "shadows/pointShadow.gs");

    Shader::clearDefault();

    // FONTS===============================
    TextRenderer font(32);
    if (!scene.registerFont(&font, "comic", "assets/fonts/comic.ttf")) {
        std::cout << "Could not load font" << std::endl;
    }

    // MODELS==============================
    scene.registerModel(&lamp);

    scene.registerModel(&wall);

    scene.registerModel(&sphere);

    scene.registerModel(&cube);

    Box box;
    box.init();

    // load all model data
    scene.loadModels();

    // LIGHTS==============================

    // directional light
    DirLight dirLight(glm::vec3(-0.2f, -0.9f, -0.2f),
        glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
        BoundingRegion(glm::vec3(-20.0f, -20.0f, 0.5f), glm::vec3(20.0f, 20.0f, 50.0f)));
    scene.dirLight = &dirLight;

    // point lights
    glm::vec3 pointLightPositions[] = {
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f,  15.0f,  0.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    glm::vec4 ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
    glm::vec4 diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glm::vec4 specular = glm::vec4(1.0f);
    float k0 = 1.0f;
    float k1 = 0.0014f;
    float k2 = 0.000007f;

    PointLight pointLights[4];

    for (unsigned int i = 0; i < 1; i++) {
        pointLights[i] = PointLight(
            pointLightPositions[i],
            k0, k1, k2,
            ambient, diffuse, specular,
            0.5f, 50.0f
        );
        // create physical model for each lamp
        scene.generateInstance(lamp.id, glm::vec3(0.5f), 0.25f, pointLightPositions[i]);
        // add lamp to scene's light source
        scene.pointLights.push_back(&pointLights[i]);
        // activate lamp in scene
        States::activateIndex(&scene.activePointLights, i);
    }

    // spot light
    SpotLight spotLight(
        //glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        cam.cameraPos, cam.cameraFront, cam.cameraUp,
        glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
        1.0f, 0.0014f, 0.000007f,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f), glm::vec4(1.0f),
        0.1f, 100.0f
    );
    scene.spotLights.push_back(&spotLight);
    //scene.activeSpotLights = 1; // 0b00000001

    scene.generateInstance(cube.id, glm::vec3(20.0f, 0.1f, 20.0f), 100.0f, glm::vec3(0.0f, -3.0f, 0.0f));
    glm::vec3 cubePositions[] = {
        { 1.0f, 2.0f, 1.0f },
        { -7.25f, 2.1f, 1.5f },
        { -15.0f, 2.55f, 9.0f },
        { 4.0f, -3.5f, 5.0f },
        { 2.8f, 1.9f, -6.2f },
        { 3.5f, 6.3f, -1.0f },
        { -3.4f, 10.9f, -5.5f },
        { 0.0f, 11.0f, 0.0f },
        { 0.0f, 5.0f, 0.0f }
    };
    for (unsigned int i = 0; i < 9; i++) {
        //scene.generateInstance(cube.id, glm::vec3(0.5f), 1.0f, cubePositions[i]);
    }

    scene.generateInstance(sphere.id, glm::vec3(1.0f), 5.0f, glm::vec3(5.0f));

    // instantiate the brickwall plane
    scene.generateInstance(wall.id, glm::vec3(1.0f), 1.0f, 
        { 0.0f, 0.0f, 2.0f }, { -1.0f, glm::pi<float>(), 0.0f });

    // instantiate instances
    scene.initInstances();

    // finish preparations (octree, etc)
    scene.prepare(box, { shader });

    // joystick recognition
    /*mainJ.update();
    if (mainJ.isPresent()) {
        std::cout << mainJ.getName() << " is present." << std::endl;
    }*/

    Keyboard::keyCallbacks.push_back(keyChanged);
    Mouse::cursorPosCallbacks.push_back(cursorChanged);
    Mouse::mouseButtonCallbacks.push_back(mouseButtonChanged);
    Mouse::mouseWheelCallbacks.push_back(scrollChanged);

    scene.variableLog["time"] = (double)0.0;

    scene.defaultFBO.bind(); // bind default framebuffer

    while (!scene.shouldClose()) {
        // calculate dt
        double currentTime = glfwGetTime();
        dt = currentTime - lastFrame;
        lastFrame = currentTime;

        scene.variableLog["time"] += dt;
        scene.variableLog["fps"] = 1 / dt;

        // update screen values
        scene.update();

        // process input
        processInput(dt);

        // activate the directional light's FBO

        // remove launch objects if too far
        for (unsigned int i = 0; i < sphere.currentNoInstances; i++) {
            if (glm::length(cam.cameraPos - sphere.instances[i]->pos) > 250.0f) {
                scene.markForDeletion(sphere.instances[i]->instanceId);
            }
        }

        //// render scene to dirlight FBO
        //dirLight.shadowFBO.activate();
        //scene.renderDirLightShader(dirShadowShader);
        //renderScene(dirShadowShader);

        //// render scene to point light FBOs
        //for (unsigned int i = 0, len = scene.pointLights.size(); i < len; i++) {
        //    if (States::isIndexActive(&scene.activePointLights, i)) {
        //        scene.pointLights[i]->shadowFBO.activate();
        //        scene.renderPointLightShader(pointShadowShader, i);
        //        renderScene(pointShadowShader);
        //    }
        //}

        //// render scene to spot light FBOs
        //for (unsigned int i = 0, len = scene.spotLights.size(); i < len; i++) {
        //    if (States::isIndexActive(&scene.activeSpotLights, i)) {
        //        scene.spotLights[i]->shadowFBO.activate();
        //        scene.renderSpotLightShader(spotShadowShader, i);
        //        renderScene(spotShadowShader);
        //    }
        //}

        // render scene normally
        scene.defaultFBO.activate();
        scene.renderShader(shader);
        scene.renderShader(lampShader);
        renderScene(shader, lampShader);

        // render boxes
        scene.renderShader(boxShader, false);
        box.render(boxShader);

        // send new frame to window
        scene.newFrame(box);

        // clear instances that have been marked for deletion
        scene.clearDeadInstances();
    }

    // clean up objects
    scene.cleanup();
    return 0;
}

void renderScene(Shader objectShader, Shader lampShader) {
    renderObjects(objectShader);
    renderLamps(lampShader);
}

void renderObjects(Shader shader) {
    scene.renderInstances(sphere.id, shader, (float)dt);
    scene.renderInstances(cube.id, shader, (float)dt);
    scene.renderInstances(wall.id, shader, (float)dt);
}

void renderLamps(Shader shader) {
    scene.renderInstances(lamp.id, shader, (float)dt);
}

void processInput(double dt) {
    // update flash light
    if (States::isIndexActive(&scene.activeSpotLights, 0)) {
        scene.spotLights[0]->position = scene.getActiveCamera()->cameraPos;
        scene.spotLights[0]->direction = scene.getActiveCamera()->cameraFront;
        scene.spotLights[0]->up = scene.getActiveCamera()->cameraUp;
        scene.spotLights[0]->updateMatrices();
    }

    // process input with cameras
    scene.processInput(dt);
}

void launchItem() {
    RigidBody* rb = scene.generateInstance(sphere.id, glm::vec3(0.1f), 1.0f, cam.cameraPos);
    if (rb) {
        // instance generated successfully
        rb->transferEnergy(25.0f, cam.cameraFront);
        rb->applyAcceleration(Environment::gravitationalAcceleration);
    }
}

void emitRay() {
    Ray r(cam.cameraPos, cam.cameraFront);

    float tmin = std::numeric_limits<float>::max();
    BoundingRegion* intersected = scene.octree->checkCollisionsRay(r, tmin);
    if (intersected) {
        std::cout << "Hits " << intersected->instance->instanceId << " at t = " << tmin << std::endl;
        scene.markForDeletion(intersected->instance->instanceId);
    }
    else {
        std::cout << "No hit" << std::endl;
    }
}

void keyChanged(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (Keyboard::keyWentDown(GLFW_KEY_ESCAPE))
    {
        scene.setShouldClose(true);
    }
    if (Keyboard::keyWentDown(GLFW_KEY_F))
    {
        launchItem();
    }

    scene.keyChanged(window, key, scancode, action, mods);
}

void cursorChanged(GLFWwindow* window, double _x, double _y)
{
    scene.cursorChanged(window, _x, _y);
}

void mouseButtonChanged(GLFWwindow* window, int button, int action, int mods)
{
    if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_1))
    {
        emitRay();
    }

    scene.mouseButtonChanged(window, button, action, mods);
}

void scrollChanged(GLFWwindow* window, double dx, double dy)
{
    scene.scrollChanged(window, dx, dy);
}