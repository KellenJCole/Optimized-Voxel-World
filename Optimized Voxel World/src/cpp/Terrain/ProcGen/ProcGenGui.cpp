#include "h/Terrain/ProcGen/ProcGenGui.h"
#include <iostream>
#include <string>

ProcGenGui::ProcGenGui() {
	update = false;
	state.resize(21);

	state[0] = 0.007;
	state[1] = 0.058;
	state[2] = 0.21;
	state[3] = 0.67;

	state[4] = 8;
	state[5] = 6;
	state[6] = 4;
	state[7] = 2;

	state[8] = 2;
	state[9] = 2;
	state[10] = 2;
	state[11] = 2;

	state[12] = 1;
	state[13] = 1;
	state[14] = 1;
	state[15] = 1;

	state[16] = 0.5;
	state[17] = 0.25;
	state[18] = 0.15;
	state[19] = 0.1;

	state[20] = 80;
}

void ProcGenGui::initialize(GLFWwindow* w, ProcGen* pg) {
	window = w;
	proceduralGeneration = pg;

	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
}

void ProcGenGui::startLoop() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("ProcGen Adjuster");

	if (ImGui::Button("Update ProcGen")) {
		proceduralGeneration->setNoiseState(state);
		update = true;
	}

	ImGui::InputFloat("Height Amplitude", &state[20], 1.0f, 1.0f, "%.1f");

	for (int i = 0; i < 4; i++) {
		if (ImGui::TreeNode(("Height Noise Layer " + std::to_string(i + 1)).c_str())) {
			ImGui::SliderFloat("Frequency", &state[i], 0.0f, 0.68f, "%.4f");
			ImGui::InputFloat("Octaves", &state[i + 4], 1.0f, 1.0f, "%.1f");
			ImGui::SliderFloat("Lacunarity", &state[i + 8], 0.0f, 4.0f, "%.4f");
			ImGui::SliderFloat("Gain", &state[i + 12], 0.0f, 4.0f, "%.4f");

			ImGui::TreePop();
		}
	}

	if (ImGui::TreeNode("Height Map Weights")) {
		ImGui::InputFloat("Layer 1", &state[16], 0.05f, 0.05f, "%.3f");
		ImGui::InputFloat("Layer 2", &state[17], 0.05f, 0.05f, "%.3f");
		ImGui::InputFloat("Layer 3", &state[18], 0.05f, 0.05f, "%.3f");
		ImGui::InputFloat("Layer 4", &state[19], 0.05f, 0.05f, "%.3f");
		ImGui::TreePop();
	}

	ImGui::End();
}

void ProcGenGui::endLoop() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ProcGenGui::shouldUpdate() {
	bool returnValue = update;
	if (returnValue) {
		update = false;
	}

	return returnValue;
}

ProcGenGui::~ProcGenGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

}