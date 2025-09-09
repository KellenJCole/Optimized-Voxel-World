#include "h/Engine/InputManager.h"

static const GLuint ENGINE_KEYS[7] = { GLFW_KEY_F, GLFW_KEY_I, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_ESCAPE, GLFW_KEY_P, GLFW_KEY_B };
static const GLuint PLAYER_KEYS[7] = { GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_G };

InputManager::InputManager() 
	: window_(nullptr)
{

}

bool InputManager::keyDown(int k) const {
	auto it = now_.find(k);
	return it != now_.end() && it->second;
}

bool InputManager::mouseDown(int b) const {
	auto it = mouseNow_.find(b);
	return it != mouseNow_.end() && it->second;
}

InputEvents InputManager::poll(bool uiCursorActive) {
	InputEvents ev;
	if (!window_) return ev;

	prev_ = now_;
	mousePrev_ = mouseNow_;

	for (GLuint k : ENGINE_KEYS) now_[k] = (glfwGetKey(window_, k) == GLFW_PRESS);
	for (GLuint k : PLAYER_KEYS) now_[k] = (glfwGetKey(window_, k) == GLFW_PRESS);
	mouseNow_[GLFW_MOUSE_BUTTON_LEFT] = (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
	mouseNow_[GLFW_MOUSE_BUTTON_RIGHT] = (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

	auto pressed = [&](int k) { return  now_[k] && !prev_[k]; };

	ev.cursorToggle = pressed(GLFW_KEY_ESCAPE);
	ev.toggleWireframe = pressed(GLFW_KEY_F) && !uiCursorActive;
	ev.togglePostFX = pressed(GLFW_KEY_P) && !uiCursorActive;
	ev.toggleDebug = pressed(GLFW_KEY_I) && !uiCursorActive;
	ev.toggleEntityBoxes = pressed(GLFW_KEY_B) && !uiCursorActive;
	ev.toggleGravity = pressed(GLFW_KEY_G) && !uiCursorActive;
	if (pressed(GLFW_KEY_UP) && !uiCursorActive) ev.renderRadiusDelta = +1;
	if (pressed(GLFW_KEY_DOWN) && !uiCursorActive) ev.renderRadiusDelta = -1;

	for (GLuint k : PLAYER_KEYS) ev.playerStates[k] = keyDown(k);
	ev.playerStates[GLFW_MOUSE_BUTTON_LEFT] = mouseDown(GLFW_MOUSE_BUTTON_LEFT);
	ev.playerStates[GLFW_MOUSE_BUTTON_RIGHT] = mouseDown(GLFW_MOUSE_BUTTON_RIGHT);

	return ev;
}