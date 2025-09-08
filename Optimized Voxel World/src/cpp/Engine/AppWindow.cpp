#include "h/Engine/AppWindow.h"
#include <iostream>

AppWindow::AppWindow() 
	: width_(0)
	, height_(0)
	, window_(nullptr) {

}

bool AppWindow::initialize(const WindowInfo& windowInfo) {
	if (!glfwInit()) {
		std::cerr << "GLFW init failed\n";
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, windowInfo.glMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, windowInfo.glMinor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, windowInfo.samples);

	width_ = windowInfo.width; height_ = windowInfo.height;
	window_ = glfwCreateWindow(width_, height_, windowInfo.title.c_str(), nullptr, nullptr);

	if (!window_) {
		std::cerr << "Window creation failed\n";
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window_);
	glfwSetWindowUserPointer(window_, this);
	setVsync(windowInfo.vsync);
	glfwSetWindowPos(window_, windowInfo.x, windowInfo.y);

	if (!loadGlad()) {
		glfwDestroyWindow(window_);
		glfwTerminate();
		return false;
	}

	installCallbacks();
	return true;
}

bool AppWindow::loadGlad() {
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initiailize GLAD\n";
		return false;
	}
	return true;
}

void AppWindow::installCallbacks() {
	glfwSetFramebufferSizeCallback(window_, &AppWindow::sFramebufferSize);
	glfwSetCursorPosCallback(window_, &AppWindow::sCursorPos);
}

void AppWindow::sFramebufferSize(GLFWwindow* win, int w, int h) {
	auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(win));
	if (!self) return;
	self->width_ = w; self->height_ = h;
	glViewport(0, 0, w, h);
	if (self->onResize) self->onResize(w, h);
}

void AppWindow::sCursorPos(GLFWwindow* win, double x, double y) {
	auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(win));
	if (!self) return;
	if (self->onCursor) self->onCursor(x, y);
}

void AppWindow::close() {
	if (window_) {
		glfwDestroyWindow(window_);
		window_ = nullptr;
	}

	glfwTerminate();
}

