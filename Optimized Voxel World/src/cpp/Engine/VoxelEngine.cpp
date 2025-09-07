#include "h/Engine/VoxelEngine.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

VoxelEngine::VoxelEngine() 
    : fps(std::numeric_limits<int>::min())
    , deltaTime(0.0f)
    , lastFrame(0.0f)
    , fpsUpdateTime(0.1f)
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
}

bool VoxelEngine::initialize() {
    AppWindow::WindowInfo info{
        WindowDetails::WindowWidth,
        WindowDetails::WindowHeight,
        "Voxel Engine",
        4, 6,
        /* vsync*/ false,
        /* samples */ 4,
        /* x */ 160,
        /* y */ 90
    };

    if (!app.initialize(info)) return false;
    app.setCursorDisabled(true);

    app.onResize = [this](int w, int h) {
        GLCall(glViewport(0, 0, w, h));
        // If you render to an FBO, resize attachments here:
        GLCall(glBindTexture(GL_TEXTURE_2D, textureColorBuffer));
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
        GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
        GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h));
    };

    app.onCursor = [this](double x, double y) {
        if (!imGuiCursor) camera.processMouseMovement(x, y);
    };

    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

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
    
    keyStates[GLFW_KEY_F] = false;                      // Toggle polygon fill mode
    keyStates[GLFW_KEY_G] = false;                      // Toggle gravity
    keyStates[GLFW_KEY_I] = false;                      // Toggle debug text rendering
    keyStates[GLFW_KEY_UP] = false;                     // Increase render distance
    keyStates[GLFW_KEY_DOWN] = false;                   // Decrease render distance
    keyStates[GLFW_KEY_ESCAPE] = false;                 // Switch cursor mode
    keyStates[GLFW_KEY_P] = false;                      // Toggle Post-Processing Shader

    lastKeyStates = keyStates;

    playerKeyStates[GLFW_KEY_W] = false;                // Move forward
    playerKeyStates[GLFW_KEY_A] = false;                // Walk left
    playerKeyStates[GLFW_KEY_S] = false;                // Walk backwards
    playerKeyStates[GLFW_KEY_D] = false;                // Walk right
    playerKeyStates[GLFW_KEY_SPACE] = false;            // Jump
    playerKeyStates[GLFW_KEY_LEFT_SHIFT] = false;       // Move down (when gravity off)
    playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = false;    // Break block
    playerKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = false;   // Place block

    debugUI.initialize();
    userInterface.initialize();

    GLFWwindow* win = app.getWindowPtr();

    proceduralGenerationGui.initialize(win, &proceduralGenerator);
    worldManager.setWindowPointer(win);

    return true;
}

void VoxelEngine::run() {
    worldManager.updateRenderChunks(0, 0, renderRadius, false);

    double lastFPSCheck = app.time();
    int frames = 0;

    while (!app.shouldClose()) {
        double currentTime = app.time();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        frames++;
        if (currentTime - lastFPSCheck >= fpsUpdateTime) {
            fps = frames;
            frames = 0;
            lastFPSCheck += fpsUpdateTime;
        }

        processInput();
        update();
        render();

        app.swap();
        app.poll();
    }
    cleanup();
}

void VoxelEngine::processInput() {
    GLFWwindow* win = app.getWindowPtr();
    float currFrame = app.time();

    for (int i = 0; i < 7; i++) keyStates[engineKeys[i]] = (glfwGetKey(win, engineKeys[i]) == GLFW_PRESS);
    for (int i = 0; i < 6; i++) playerKeyStates[playerKeys[i]] = (glfwGetKey(win, playerKeys[i]) == GLFW_PRESS);

    playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    playerKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    if (keyStates[GLFW_KEY_ESCAPE] && !lastKeyStates[GLFW_KEY_ESCAPE]) {    // Esc - Switch cursor mode
        imGuiCursor = !imGuiCursor;
        app.setCursorDisabled(!imGuiCursor);

        if (imGuiCursor) {
            for (int i = 0; i < 6; i++) playerKeyStates[playerKeys[i]] = false;

            playerKeyStates[GLFW_MOUSE_BUTTON_LEFT] = false;
            playerKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = false;
            player.processKeyboardInput(playerKeyStates, deltaTime);
        }
    }

    if (!imGuiCursor) player.processKeyboardInput(playerKeyStates, deltaTime);

    if (!imGuiCursor) {
        if (keyStates[GLFW_KEY_F] && !lastKeyStates[GLFW_KEY_F]) worldManager.switchRenderMethod();         // F -> Toggle polygon line/fill
		if (keyStates[GLFW_KEY_P] && !lastKeyStates[GLFW_KEY_P]) usePostProcessing = !usePostProcessing;    // P -> Toggle post-processing shader
        if (keyStates[GLFW_KEY_G] && !lastKeyStates[GLFW_KEY_G]) player.toggleGravity();                    // G -> Toggle gravity
        if (keyStates[GLFW_KEY_I] && !lastKeyStates[GLFW_KEY_I]) renderDebug = !renderDebug;                // I -> Toggle debug text
        if (keyStates[GLFW_KEY_UP] && !lastKeyStates[GLFW_KEY_UP]) {                                        // Up arrow -> Increase render radius by 1
            if (renderRadius < 512) {
                renderRadius++;
                worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, false);
            }
        }
        if (keyStates[GLFW_KEY_DOWN] && !lastKeyStates[GLFW_KEY_DOWN]) {                                    // Down arrow -> Decrease render radius by 1
            if (renderRadius > 1) {
                renderRadius--;
                worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, false);
            }
        }
    }

    lastKeyStates = keyStates;
}

void VoxelEngine::update() {
    if (worldManager.getReadyForPlayerUpdate()) player.update(deltaTime);

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
}


void VoxelEngine::cleanup() {
    worldManager.cleanup();
    blockShader.deleteProgram();
    debugShader.deleteProgram();
    userInterfaceShader.deleteProgram();
    app.close();
}

void VoxelEngine::processMouseInput(double xpos, double ypos) {
    if (!imGuiCursor) camera.processMouseMovement(xpos, ypos);
}

const GLuint VoxelEngine::engineKeys[7] = { GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_I, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_ESCAPE, GLFW_KEY_P };

const GLuint VoxelEngine::playerKeys[6] = { GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT };
