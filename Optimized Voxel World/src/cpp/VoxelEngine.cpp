#include "h/VoxelEngine.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

VoxelEngine::VoxelEngine() 
    : deltaTime(0.0f)
    , lastFrame(0.0f)
    , fps(std::numeric_limits<int>::min())
    , renderDebug(false)
    , imGuiCursor(false)
    , usePostProcessing(true)
    , renderRadius(64)
    , vertexPool(1ULL * 1024 * 1024 * 1024) 
{

	currChunkX = ChunkUtils::worldToChunkCoord(static_cast<int>(floor(camera.getCameraPos().x)));
	currChunkZ = ChunkUtils::worldToChunkCoord(static_cast<int>(floor(camera.getCameraPos().z)));
    lastChunkX = currChunkX;
    lastChunkZ = currChunkZ;

    fpsUpdateTime = .1;

}

bool VoxelEngine::initialize() {
    if (!glfwInit()) {
        std::cout << "GLFW failed to initialize\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WindowDetails::WindowWidth, WindowDetails::WindowHeight, "Voxel Engine", NULL, NULL);
    glfwSetWindowPos(window, 160, 90);
    if (!window)
    {
        glfwTerminate();
        std::cout << "Window failed to create\n";
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSwapInterval(0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwWindowHint(GLFW_SAMPLES, 4);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initiailize GLAD\n";
        return false;
    }

    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    std::cout << glGetString(GL_VERSION) << "\n";

    if (!vertexPool.initialize()) {
        std::cerr << "VertexPool failed to initialize.\n";
        return false;
    }

    if (!worldManager.initialize(&proceduralGenerator, &vertexPool))
        std::cout << "World Manager initialization failure\n";

    blockShader = Shader("src/res/shaders/Block.shader");
    debugShader = Shader("src/res/shaders/Debug.shader");
    userInterfaceShader = Shader("src/res/shaders/UserInterface.shader");

    GLCall(glGenFramebuffers(1, &framebuffer));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));

    GLCall(glGenTextures(1, &textureColorBuffer));
    GLCall(glBindTexture(GL_TEXTURE_2D, textureColorBuffer));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WindowDetails::WindowWidth, WindowDetails::WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0));

    GLCall(glGenRenderbuffers(1, &rbo));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WindowDetails::WindowWidth, WindowDetails::WindowHeight));
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        return false;
    }
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    postProcessingShader = Shader("src/res/shaders/PostProcessingArtifact.shader");

    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    GLCall(glGenVertexArrays(1, &quadVAO));
    GLCall(glGenBuffers(1, &quadVBO));
    GLCall(glBindVertexArray(quadVAO));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));
    GLCall(glBindVertexArray(0));

    debugShader.use();
    glm::mat4 projection = glm::ortho(0.0f, (float)WindowDetails::WindowWidth, 0.0f, (float)WindowDetails::WindowHeight);
    debugShader.setUniform4fv("projection", projection);

    userInterfaceShader.use();
    userInterfaceShader.setUniform4fv("projection", projection);

    camera = Camera(0.2);
    swapRenderMethodCooldown = 0.0f;
    worldManager.passObjectPointers(&blockShader, &camera);

    player.setCamera(&camera);
    player.setWorld(&worldManager);
    
    keyStates[GLFW_KEY_F] = false; // Toggle polygon fill mode
    keyStates[GLFW_KEY_G] = false; // Toggle gravity
    keyStates[GLFW_KEY_I] = false; // Toggle debug text rendering
    keyStates[GLFW_KEY_UP] = false; // Increase render distance
    keyStates[GLFW_KEY_DOWN] = false; // Decrease render distance
    keyStates[GLFW_KEY_ESCAPE] = false; // Switch cursor mode
    keyStates[GLFW_KEY_P] = false; // Toggle Post-Processing Shader

    lastKeyStates = keyStates;

    playerKeyStates[GLFW_KEY_W] = false;
    playerKeyStates[GLFW_KEY_A] = false;
    playerKeyStates[GLFW_KEY_S] = false;
    playerKeyStates[GLFW_KEY_D] = false;
    playerKeyStates[GLFW_KEY_SPACE] = false;
    playerKeyStates[GLFW_KEY_LEFT_SHIFT] = false;
    playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = false;
    playerKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = false;

    debugUI.initialize();
    userInterface.initialize();
    player.initialize();
    proceduralGenerationGui.initialize(window, &proceduralGenerator);

    worldManager.setWindowPointer(window);

    return true;
}

void VoxelEngine::run() {
    worldManager.updateRenderChunks(0, 0, renderRadius, false);

    double timeAtLastFPSCheck = glfwGetTime();
    int numberOfFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        numberOfFrames++;
        if (currentTime - timeAtLastFPSCheck >= fpsUpdateTime) {
            fps = numberOfFrames;
            numberOfFrames = 0;
            timeAtLastFPSCheck += fpsUpdateTime;
        }

        processInput();
        update();
        render();
    }
    cleanup();
}

void VoxelEngine::processInput() {
    float currFrame = glfwGetTime();

    for (int i = 0; i < 7; i++) {
        keyStates[engineKeys[i]] = (glfwGetKey(window, engineKeys[i]) == GLFW_PRESS) ? true : false;
    }

    for (int i = 0; i < 6; i++) {
        playerKeyStates[playerKeys[i]] = (glfwGetKey(window, playerKeys[i]) == GLFW_PRESS) ? true : false;
    }

    playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? true : false;
    playerKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) ? true : false;

    if (keyStates[GLFW_KEY_ESCAPE] && !lastKeyStates[GLFW_KEY_ESCAPE]) {    // Esc -Switch cursor mode
        imGuiCursor = !imGuiCursor;
        if (imGuiCursor) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            for (int i = 0; i < 6; i++) {
                playerKeyStates[playerKeys[i]] = false;
            }
            playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = false;
            playerKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = false;
            player.processKeyboardInput(playerKeyStates, deltaTime);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (!imGuiCursor) {
        player.processKeyboardInput(playerKeyStates, deltaTime);
    }

    if (!imGuiCursor) {
        if (keyStates[GLFW_KEY_F] && !lastKeyStates[GLFW_KEY_F])                // F -> Toggle polygon line/fill
            worldManager.switchRenderMethod();
		if (keyStates[GLFW_KEY_P] && !lastKeyStates[GLFW_KEY_P])                // P -> Toggle post-processing shader
            usePostProcessing = !usePostProcessing;
        if (keyStates[GLFW_KEY_G] && !lastKeyStates[GLFW_KEY_G])                // G -> Toggle gravity
            player.toggleGravity();
        if (keyStates[GLFW_KEY_I] && !lastKeyStates[GLFW_KEY_I])                // I -> Toggle debug text
            renderDebug = !renderDebug;
        if (keyStates[GLFW_KEY_UP] && !lastKeyStates[GLFW_KEY_UP]) {            // Up arrow -> Increase render radius by 1
            renderRadius += 1;
            worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, false);
        }
        if (keyStates[GLFW_KEY_DOWN] && !lastKeyStates[GLFW_KEY_DOWN]) {        // Down arrow -> Decrease render radius by 1
            if (renderRadius > 1) {
                renderRadius--;
                worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, false);
            }
        }
    }

    lastKeyStates = keyStates;
}

void VoxelEngine::update() {
    if (worldManager.getReadyForPlayerUpdate())
        player.update(deltaTime);

    int cameraPosX = static_cast<int>(floor(camera.getCameraPos().x));
    int cameraPosZ = static_cast<int>(floor(camera.getCameraPos().z));
	currChunkX = ChunkUtils::worldToChunkCoord(cameraPosX);
    currChunkZ = ChunkUtils::worldToChunkCoord(cameraPosZ);
    
    worldManager.update();

    if (proceduralGenerationGui.shouldUpdate())
        worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, true);
    if (currChunkX != lastChunkX || currChunkZ != lastChunkZ)
        worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, false);

    lastChunkX = currChunkX;
    lastChunkZ = currChunkZ;
}

void VoxelEngine::render() {
    if (usePostProcessing) {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer)); 
    }
    else {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }

    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GLCall(glClearColor(0.529f, 0.808f, 0.922f, 0.2f));
    

    glm::vec3 viewPos = camera.getCameraPos();
    camera.update();
    blockShader.use();

    blockShader.setUniform4fv("view", (glm::mat4&)camera.getView());
    blockShader.setUniform4fv("projection", (glm::mat4&)camera.getProjection());
    // Render stuff below here
    proceduralGenerationGui.startLoop();
    worldManager.render();

    if (usePostProcessing) {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        postProcessingShader.use();

        GLCall(glActiveTexture(GL_TEXTURE0));
        GLCall(glBindTexture(GL_TEXTURE_2D, textureColorBuffer));
        postProcessingShader.setUniform1i("screenTexture", 0);
        GLCall(glBindVertexArray(quadVAO));
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
    }

    GLCall(glDisable(GL_DEPTH_TEST));

    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    if (renderDebug) {
        glm::vec3 cameraPos = camera.getCameraPos();

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2);
        stream << fps * (1.f / fpsUpdateTime) << "\n";
        stream << cameraPos.x << " " << (cameraPos.y - 1.8) << " " << cameraPos.z << "\n";

        debugUI.renderText(debugShader, stream.str(), 10.0f, 1020.0f, 0.8f, glm::vec3(0.f, 0.f, 0.f));
    }

    userInterface.render(userInterfaceShader);
    
    GLCall(glEnable(GL_DEPTH_TEST));

    proceduralGenerationGui.endLoop();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void VoxelEngine::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    GLCall(glViewport(0, 0, width, height));
}

void VoxelEngine::cleanup() {
    worldManager.cleanup();
    blockShader.deleteProgram();
    debugShader.deleteProgram();
    userInterfaceShader.deleteProgram();
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
    if (!imGuiCursor) {
        camera.processMouseMovement(xpos, ypos);
    }
}

const GLuint VoxelEngine::engineKeys[7] = { GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_I, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_ESCAPE, GLFW_KEY_P };

const GLuint VoxelEngine::playerKeys[6] = { GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT };
