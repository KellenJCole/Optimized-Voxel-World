#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "h/Rendering/Renderer.h"
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

	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

	GLFWwindow* window;
	WorldManager worldManager;
	Shader blockShader, debugShader, userInterfaceShader;
	Camera camera;
	VertexPool vertexPool;
	DebugUI debugUI;
	UserInterface userInterface;;
	ProcGenGui proceduralGenerationGui;
	ProcGen proceduralGeneration;

	Player player;

	float deltaTime;
	float lastFrame;

	int currChunkX, currChunkZ;
	int lastChunkX, lastChunkZ;
	int fps;
	bool renderDebug;
	bool imGuiCursor;
	float fpsUpdateTime;
	std::map<GLuint, bool> keyStates;
	std::map<GLuint, bool> lastKeyStates;
	std::map<GLuint, bool> playerKeyStates;
	static const GLuint engineKeys[7];
	static const GLuint playerKeys[6];

	int renderRadius;

	float swapRenderMethodCooldown;

	GLuint framebuffer;
	GLuint textureColorBuffer;
	GLuint rbo;
	Shader postProcessingShader;
	GLuint quadVAO, quadVBO;
	bool usePostProcessing;
};