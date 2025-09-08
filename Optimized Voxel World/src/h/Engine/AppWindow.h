#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "h/Rendering/Utility/GLErrorCatcher.h"

#include <functional>
#include <string>

class AppWindow {
public:
	struct WindowInfo {
		int width, height;
		std::string title = "Voxel Engine";
		int glMajor = 4, glMinor = 6;	// OpenGL version
		bool vsync = false;
		int samples = 4;				// anti aliasing
		int x = 160, y = 90;			// initial window position
	};

	AppWindow();

	std::function<void(int, int)> onResize;
	std::function<void(double, double)> onCursor;

	bool initialize(const WindowInfo& windowInfo);
	void close();

	void poll() { glfwPollEvents(); }
	void swap() { glfwSwapBuffers(window_); }
	bool shouldClose() const { return glfwWindowShouldClose(window_); }
	double time() const { return glfwGetTime(); }

	void setVsync(bool on) { glfwSwapInterval(on ? 1 : 0); }
	void setCursorDisabled(bool disabled) {
		glfwSetInputMode(window_, GLFW_CURSOR, disabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	int getWidth() const { return width_; }
	int getHeight() const { return height_; }
	GLFWwindow* getWindowPtr() const { return window_; }

private:
	static void sFramebufferSize(GLFWwindow* win, int w, int h);
	static void sCursorPos(GLFWwindow* win, double x, double y);

	void installCallbacks();
	bool loadGlad();

	GLFWwindow* window_;
	int width_, height_;
};
