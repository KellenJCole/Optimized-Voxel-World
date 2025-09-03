#pragma once
#include <GLFW/glfw3.h>
#include "h/Terrain/ProcGen/ProcGen.h"
#include "h/external/Dear ImGui/imgui.h"
#include "h/external/Dear ImGui/imgui_impl_glfw.h"
#include "h/external/Dear ImGui/imgui_impl_opengl3.h"

class ProcGenGui {
public:
	ProcGenGui();
	void initialize(GLFWwindow* w, ProcGen* pg);
	void startLoop();
	void endLoop();
	bool shouldUpdate();
	~ProcGenGui();
private:
	bool update;
	std::vector<float> state;
	ProcGen* proceduralGeneration;
	GLFWwindow* window;
};