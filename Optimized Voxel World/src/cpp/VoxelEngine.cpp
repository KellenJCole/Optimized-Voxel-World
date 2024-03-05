#include "h/VoxelEngine.h"
#include <iostream>
#include "h/Rendering/Shader.h"
#include "h/Rendering/GLErrorCatcher.h"
#include <string>
#include <sstream>
#include <iomanip>

VoxelEngine::VoxelEngine() :
    deltaTime(0.0f),
    lastFrame(0.0f),
    fps(0),
renderDebug(false) {

    currChunkX = camera.getCameraPos().x / 16;
    currChunkZ = camera.getCameraPos().z / 16;
    lastChunkX = currChunkX;
    lastChunkZ = lastChunkZ;

    renderRadius = 30;
}

bool VoxelEngine::initialize() {
    if (!glfwInit()) {
        std::cout << "GLFW failed to initialize\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (glewInit() != GLEW_OK) {
        std::cout << "glewInit() != GLEW_OK\n";
        return false;
    }

    std::cout << glGetString(GL_VERSION) << "\n";

    if (!worldManager.initialize()) {
        std::cout << "World Manager initialization failure\n";
    }
    blockShader = Shader("src/res/shaders/Block.shader");
    debugShader = Shader("src/res/shaders/Debug.shader");

    debugShader.use();
    glm::mat4 projection = glm::ortho(0.0f, 1600.f, 0.0f, 900.f);
    debugShader.setUniform4fv("projection", projection);

    camera = Camera(0.2); // 0.2 is our camera's sensitivity.
    swapRenderMethodCooldown = 0.0f;

    worldManager.setCamAndShaderPointers(&blockShader, &camera);
    player.setCamera(&camera);
    player.setWorld(&worldManager);

    if (!debugUI.initialize()) {
        std::cout << "DebugUI failed initialization\n";
    }

    std::cout << "VoxelEngine.initialize() successful\n";

    // Set up keyStates map for keeping track of if a button was pressed last frame or is currently released for toggle buttons
    
    keyStates[GLFW_KEY_F] = false; // Toggle polygon fill mode
    keyStates[GLFW_KEY_G] = false; // Toggle gravity
    keyStates[GLFW_KEY_I] = false; // Toggle debug text rendering
    keyStates[GLFW_KEY_UP] = false; // Increase render distance
    keyStates[GLFW_KEY_DOWN] = false; // Decrease render distance

    playerKeyStates[GLFW_KEY_W] = false;
    playerKeyStates[GLFW_KEY_A] = false;
    playerKeyStates[GLFW_KEY_S] = false;
    playerKeyStates[GLFW_KEY_D] = false;
    playerKeyStates[GLFW_KEY_SPACE] = false;
    playerKeyStates[GLFW_KEY_LEFT_SHIFT] = false;
    playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = false;


    return true;
}

void VoxelEngine::run() {
    std::cout << "Entering Voxel Engine loop. ProcessInputs->Update->Render\n";
    worldManager.updateRenderChunks(0, 0, renderRadius);

    double timeAtLastFPSCheck = glfwGetTime();
    int numberOfFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        numberOfFrames++;
        if (currentTime - timeAtLastFPSCheck >= 0.1) {
            fps = numberOfFrames;
            numberOfFrames = 0;
            timeAtLastFPSCheck += 0.1;
        }

        processInput();
        update();
        render();
    }
    cleanup();
}

void VoxelEngine::processInput() {
    float currFrame = glfwGetTime();

    playerKeyStates[GLFW_KEY_W] =           (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)          ? true : false;
    playerKeyStates[GLFW_KEY_A] =           (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)          ? true : false;
    playerKeyStates[GLFW_KEY_S] =           (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)          ? true : false;
    playerKeyStates[GLFW_KEY_D] =           (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)          ? true : false;
    playerKeyStates[GLFW_KEY_SPACE] =       (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      ? true : false;
    playerKeyStates[GLFW_KEY_LEFT_SHIFT] =  (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? true : false;
    playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] =  (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? true : false;
    
    player.processKeyboardInput(playerKeyStates, deltaTime);

    /* Keys that should be counted as pressed once, no continuous holding */

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !keyStates[GLFW_KEY_F]) {           // F -> Toggle polygon mode between fill and line
        keyStates[GLFW_KEY_F] = true;
        worldManager.switchRenderMethod();
    }
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && keyStates[GLFW_KEY_F]) {
        keyStates[GLFW_KEY_F] = false;
    }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !keyStates[GLFW_KEY_G]) {           // G -> Toggle gravity
        keyStates[GLFW_KEY_G] = true;
        player.toggleGravity();
    }
    else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && keyStates[GLFW_KEY_G]) {
        keyStates[GLFW_KEY_G] = false;
    }

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !keyStates[GLFW_KEY_I]) {           // I -> Toggle debug text
        keyStates[GLFW_KEY_I] = true;
        renderDebug = !renderDebug;
    }
    else if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE && keyStates[GLFW_KEY_I]) {
        keyStates[GLFW_KEY_I] = false;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !keyStates[GLFW_KEY_UP]) {         // Up arrow -> Increase render radius by 1
        keyStates[GLFW_KEY_UP] = true;
        renderRadius += 1;
        worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius);
    }
    else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && keyStates[GLFW_KEY_UP]) {
        keyStates[GLFW_KEY_UP] = false;
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && !keyStates[GLFW_KEY_DOWN]) {     // Down arrow -> Decrease render radius by 1
        keyStates[GLFW_KEY_DOWN] = true;
        if (renderRadius > 1) {
            renderRadius--;
            worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius);
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && keyStates[GLFW_KEY_DOWN]) {
        keyStates[GLFW_KEY_DOWN] = false;
    }
}

void VoxelEngine::update() {
    player.update(deltaTime);

    currChunkX = camera.getCameraPos().x / 16;
    currChunkZ = camera.getCameraPos().z / 16;
    
    worldManager.update();

    if (currChunkX != lastChunkX || currChunkZ != lastChunkZ) {
        worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius);
    }
    lastChunkX = currChunkX;
    lastChunkZ = currChunkZ;
}

void VoxelEngine::render() {
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GLCall(glClearColor(0.529f, 0.808f, 0.922f, 1.0f));
    
    glm::vec3 viewPos = camera.getCameraPos();
    camera.update();
    blockShader.use();

    blockShader.setUniform4fv("view", (glm::mat4&)camera.getView());
    blockShader.setUniform4fv("projection", (glm::mat4&)camera.getProjection());
    // Render stuff below here
    worldManager.render();

    if (renderDebug) {
        glm::vec3 cameraPos = camera.getCameraPos();

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(3);
        stream << fps * 10 << " fps\n"
            << "World Coordinates: " << cameraPos.x << ", " << cameraPos.y - 1.8 << ", " << cameraPos.z;

        debugUI.renderText(debugShader, stream.str(), 10.0f, 870.0f, 0.8f, glm::vec3(0.0f, 0.5f, 0.5f));
    }
    
    glfwSwapBuffers(window);

    glfwPollEvents();
}

// Handle window resizing
void VoxelEngine::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    GLCall(glViewport(0, 0, width, height));
}

void VoxelEngine::cleanup() {
    worldManager.cleanup();
    blockShader.deleteProgram();
    debugShader.deleteProgram();
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
