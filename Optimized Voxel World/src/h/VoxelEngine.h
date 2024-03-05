#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "h/Rendering/Renderer.h"
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Camera.h"
#include "h/Player/Player.h"
#include "h/Rendering/UI/DebugUI.h"

class VoxelEngine {
public: 
	VoxelEngine(); // Sets up basic variables
	bool initialize(); // Sets up GLFWwindow and OpenGL Context
	void run(); // Main game loop
	void cleanup(); // Clean up resources
private:
	// Render loop functions
	void processInput();
	void update();
	void render();

	void processMouseInput(double xpos, double ypos);

	// Handle window resizing
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

	GLFWwindow* window;
	WorldManager worldManager;
	Shader blockShader, debugShader;
	Camera camera;
	DebugUI debugUI;

	Player player;

	float deltaTime;
	float lastFrame;

	int currChunkX, currChunkZ;
	int lastChunkX, lastChunkZ;
	int fps;
	bool renderDebug;
	std::map<GLuint, bool> keyStates;
	std::map<GLuint, bool> playerKeyStates;

	int renderRadius;

	float swapRenderMethodCooldown;
};