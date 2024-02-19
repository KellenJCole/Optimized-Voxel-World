#include "h/VoxelEngine.h"
#include <iostream>
#include "h/Rendering/Shader.h"
#include "h/Rendering/GLErrorCatcher.h"

VoxelEngine::VoxelEngine() :
    deltaTime(0.0f),
    lastFrame(0.0f) {

    currChunkX = camera.getCameraPos().x / 16;
    currChunkZ = camera.getCameraPos().z / 16;
    lastChunkX = camera.getCameraPos().x / 16;
    lastChunkZ = camera.getCameraPos().z / 16;

}

bool VoxelEngine::initialize() {
    if (!glfwInit()) {
        std::cout << "GLFW failed to initialize\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1600, 900, "Voxel Engine", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        std::cout << "Window failed to create\n";
        return false;
    }

    glfwMakeContextCurrent(window); // Create the OpenGL context
    glfwSetWindowUserPointer(window, this);

    glfwSwapInterval(0); // V-SYNC

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetCursorPosCallback(window, cursor_position_callback);

    if (glewInit() != GLEW_OK) {
        std::cout << "glewInit() != GLEW_OK\n";
        return false;
    }

    std::cout << glGetString(GL_VERSION) << "\n";

    if (!worldManager.initialize()) {
        std::cout << "World Manager initialization failure\n";
    }
    shader = Shader("src/res/shaders/Block.shader");
    camera = Camera(0.2); // 0.2 is our camera's sensitivity.

    worldManager.setCamAndShaderPointers(&shader, &camera);

    std::cout << "VoxelEngine.initialize() successful\n";

    return true;
}

void VoxelEngine::run() {
    std::cout << "Entering Voxel Engine loop. ProcessInputs->Update->Render\n";
    worldManager.updateRenderChunks(0, 0);

    // TEMP CODE
    double timeAtLastFPSCheck = glfwGetTime();
    int numberOfFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        numberOfFrames++;
        if (currentTime - timeAtLastFPSCheck >= 1.0) {
            std::cout << numberOfFrames << " FPS\n";
            numberOfFrames = 0;
            timeAtLastFPSCheck += 1.0;
        }

        processInput();
        update();
        render();
    }
    cleanup();
}

void VoxelEngine::processInput() {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboardInput(GLFW_KEY_W, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboardInput(GLFW_KEY_S, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboardInput(GLFW_KEY_A, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboardInput(GLFW_KEY_D, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.processKeyboardInput(GLFW_KEY_LEFT_SHIFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.processKeyboardInput(GLFW_KEY_SPACE, deltaTime);
    }
}

void VoxelEngine::update() {
    currChunkX = camera.getCameraPos().x / 16;
    currChunkZ = camera.getCameraPos().z / 16;
    
    if (currChunkX != lastChunkX || currChunkZ != lastChunkZ) {
        worldManager.updateRenderChunks(currChunkX, currChunkZ);
    }
    lastChunkX = currChunkX;
    lastChunkZ = currChunkZ;

    worldManager.update();
}

void VoxelEngine::render() {
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GLCall(glClearColor(0.0f, 0.804f, 1.0f, 1.0f));
    
    glm::vec3 viewPos = camera.getCameraPos();
    camera.update();
    shader.use();

    shader.setUniform4fv("view", (glm::mat4&)camera.getView());
    shader.setUniform4fv("projection", (glm::mat4&)camera.getProjection());
    // Render stuff below here
    worldManager.render();

    glfwSwapBuffers(window);

    glfwPollEvents();
}

// Handle window resizing
void VoxelEngine::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    GLCall(glViewport(0, 0, width, height));
}

void VoxelEngine::cleanup() {
    worldManager.cleanup();
    glfwTerminate();
}

void VoxelEngine::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    VoxelEngine* engine = static_cast<VoxelEngine*>(glfwGetWindowUserPointer(window));
    if (engine) {
        engine->processMouseInput(xpos, ypos);
    }
}

void VoxelEngine::processMouseInput(double xpos, double ypos) {
    camera.processMouseMovement(xpos, ypos);
}
