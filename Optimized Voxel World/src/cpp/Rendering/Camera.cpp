#include "h/Rendering/Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera() {

}

Camera::Camera(float inputSensitivity)
	: cameraPos(0.0f, 258.0f, 0.0f),
	cameraFront(0.0f, 0.0f, -1.0f),
	cameraUp(0.0f, 1.0f, 0.0f),
	yaw(-90.0f),
	pitch(0.0f),
	firstMouse(true),
	sensitivity(inputSensitivity)
{
	projection = glm::perspective(glm::radians(90.f), 1600.0f / 900.0f, 0.1f, 5000.0f);
	view = glm::mat4(1.0f);
	updateCameraVectors(); // Ensure initial direction vectors are correct

}

void Camera::update()
{
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

#include <iostream>
void Camera::processKeyboardInput(int key, float deltaTime)
{
	const float camSpeed = 30.f * deltaTime;
	glm::vec3 moveDir;
	float originalY = cameraPos.y;

	switch (key) {
	case GLFW_KEY_W: //w
		moveDir = glm::normalize(glm::vec3(cameraFront.x, 0.0, cameraFront.z));
		break;
	case GLFW_KEY_S: //s
		moveDir = -glm::normalize(glm::vec3(cameraFront.x, 0.0, cameraFront.z));
		break;
	case GLFW_KEY_A: //a
		moveDir = -glm::normalize(glm::cross(cameraFront, cameraUp));
		break;
	case GLFW_KEY_D: //d
		moveDir = glm::normalize(glm::cross(cameraFront, cameraUp));
		break;
	case GLFW_KEY_LEFT_SHIFT: // left shift
		cameraPos.y -= camSpeed * 5;
		return;
	case GLFW_KEY_SPACE: // space
		cameraPos.y += camSpeed * 5;
		return;
	}

	cameraPos += moveDir * camSpeed;
}

void Camera::processMouseMovement(double xPos, double yPos)
{
	if (firstMouse) {
		lastX = (float)xPos;
		lastY = (float)yPos;
		firstMouse = false;
	}

	float xOffset = (((float)xPos - lastX) * sensitivity);
	float yOffset = -(((float)yPos - lastY) * sensitivity);
	lastX = (float)xPos;
	lastY = (float)yPos;

	yaw += xOffset;
	pitch = glm::clamp(pitch + yOffset, -89.0f, 89.0f);

	updateCameraVectors();
}

void Camera::updateCameraVectors() {
	// Recalculate the front vector
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}
