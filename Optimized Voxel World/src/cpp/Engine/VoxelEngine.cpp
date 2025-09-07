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
    // Create the default window info
    AppWindow::WindowInfo info{
        WindowDetails::WindowWidth,
        WindowDetails::WindowHeight,
        "Voxel Engine",
        4, 6,
        /* vsync*/      false,
        /* samples */   4,
        /* x */         160,
        /* y */         90
    };

    // Initialize objects
    if (!app.initialize(info)) return false;
    if (!postFX.init(info.width, info.height, "src/res/shaders/PostProcessingArtifact.shader")) {
        std::cerr << "PostFX init failed\n";
        return false;
    }
    GLFWwindow* win = app.getWindowPtr();
    input.setWindow(win);

    if (!debugUI.initialize()) {
        std::cerr << "DebugUI init failed\n";
        return false;
    }
    if (!userInterface.initialize()) {
        std::cerr << "User interface init failed\n";
        return false;
    }
    proceduralGenerationGui.initialize(win, &proceduralGenerator);
    if (!vertexPool.initialize()) {
        std::cerr << "VertexPool failed to initialize.\n";
        return false;
    }

    if (!worldManager.initialize(&proceduralGenerator, &vertexPool))
        std::cerr << "World Manager failed to initialize\n";
    worldManager.setWindowPointer(win);

    app.setCursorDisabled(true);

    app.onResize = [this](int w, int h) {
        GLCall(glViewport(0, 0, w, h));
        postFX.resize(w, h);
    };

    app.onCursor = [this](double x, double y) {
        if (!imGuiCursor) camera.processMouseMovement(x, y);
    };

    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    blockShader = Shader("src/res/shaders/Block.shader");
    debugShader = Shader("src/res/shaders/Debug.shader");
    userInterfaceShader = Shader("src/res/shaders/UserInterface.shader");

    debugShader.use();
    glm::mat4 projection = glm::ortho(0.0f, (float)WindowDetails::WindowWidth, 0.0f, (float)WindowDetails::WindowHeight);
    debugShader.setUniform4fv("projection", projection);

    userInterfaceShader.use();
    userInterfaceShader.setUniform4fv("projection", projection);

    camera = Camera(0.2);
    worldManager.passObjectPointers(&blockShader, &camera);

    player.setCamera(&camera);
    player.setWorld(&worldManager);

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

    InputEvents ev = input.poll(imGuiCursor);

    if (ev.cursorToggle) {                                      // Escape -> switch cursor mode
        imGuiCursor = !imGuiCursor;
        app.setCursorDisabled(!imGuiCursor);
        if (imGuiCursor) {
            auto cleared = ev.playerStates;
            for (auto& p : cleared) p.second = false;
            player.processKeyboardInput(cleared, deltaTime);
        }
    }

    if (!imGuiCursor) player.processKeyboardInput(ev.playerStates, deltaTime);

    if (!imGuiCursor) {
        if (ev.toggleWireframe) worldManager.switchRenderMethod();              // F
		if (ev.togglePostFX) usePostProcessing = !usePostProcessing;            // P
        if (ev.toggleGravity) player.toggleGravity();                           // G
        if (ev.toggleDebug) renderDebug = !renderDebug;                         // I

        if (ev.renderRadiusDelta != 0) {                                        // Up/down arrow
            int newRadius = renderRadius + ev.renderRadiusDelta;
            if (newRadius >= 1 && renderRadius < 512) {
                renderRadius = newRadius;
                worldManager.updateRenderChunks(currChunkX, currChunkZ, renderRadius, false);
            }
        }
    }
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

    camera.update();
}

void VoxelEngine::render() {
    if (usePostProcessing) postFX.beginScene();
    else {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    }

    GLCall(glClearColor(0.529f, 0.808f, 0.922f, 0.2f));
    
    blockShader.use();

    blockShader.setUniform4fv("view", (glm::mat4&)camera.getView());
    blockShader.setUniform4fv("projection", (glm::mat4&)camera.getProjection());
    // Render stuff below here
    proceduralGenerationGui.startLoop();

    GLCall(glEnable(GL_DEPTH_TEST));
    worldManager.render();

    if (usePostProcessing) {
        GLCall(glDisable(GL_DEPTH_TEST));
        postFX.blitToScreen();
        GLCall(glEnable(GL_DEPTH_TEST));
    }

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

    proceduralGenerationGui.endLoop();
}


void VoxelEngine::cleanup() {
    worldManager.cleanup();
    blockShader.deleteProgram();
    debugShader.deleteProgram();
    userInterfaceShader.deleteProgram();
    app.close();
    postFX.destroy();
}

void VoxelEngine::processMouseInput(double xpos, double ypos) {
    if (!imGuiCursor) camera.processMouseMovement(xpos, ypos);
}
