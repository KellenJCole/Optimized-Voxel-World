#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>

struct InputEvents {
	bool cursorToggle = false;
	bool toggleWireframe = false;
	bool togglePostFX = false;
	bool toggleGravity = false;
	bool toggleDebug = false;
	int renderRadiusDelta = 0;

	std::map<GLuint, bool> playerStates;
};

class InputManager {
public:
	InputManager();
	void setWindow(GLFWwindow* w) { window_ = w; }

	InputEvents poll(bool uiCursorActive);
private:
	bool keyDown(int k) const;
	bool mouseDown(int b) const;

	GLFWwindow* window_;
	std::map<GLuint, bool> now_, prev_;
	std::map<GLuint, bool> mouseNow_, mousePrev_;
};