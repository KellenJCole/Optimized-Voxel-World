#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "h/Engine/AppWindow.h"
#include "h/Rendering/PostProcessingPass.h"
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Camera.h"
#include "h/Player/Player.h"
#include "h/Rendering/UI/DebugUI.h"
#include "h/Rendering/UI/UserInterface.h"
#include "h/Terrain/ProcGen/ProcGenGui.h"

class VoxelEngine {
public: 
	VoxelEngine();
	bool initialize();
	void run();
	void cleanup();
private:
	void processInput();
	void update();
	void render();

	void processMouseInput(double xpos, double ypos);

	AppWindow app;
	WorldManager worldManager;
	Shader blockShader, debugShader, userInterfaceShader;
	Camera camera;
	VertexPool vertexPool;
	DebugUI debugUI;
	UserInterface userInterface;;
	ProcGenGui proceduralGenerationGui;
	ProcGen proceduralGenerator;

	Player player;

	int fps;
	float deltaTime;
	float lastFrame;

	int currChunkX, currChunkZ;
	int lastChunkX, lastChunkZ;

	bool renderDebug;
	float fpsUpdateTime;

	bool imGuiCursor;
	std::map<GLuint, bool> keyStates;
	std::map<GLuint, bool> lastKeyStates;
	std::map<GLuint, bool> playerKeyStates;
	static const GLuint engineKeys[7];
	static const GLuint playerKeys[6];

	int renderRadius;

	float swapRenderMethodCooldown;

	PostProcessingPass postFX;
	bool usePostProcessing = true;
};