#include "h/Rendering/Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera() {

}

Camera::Camera(float inputSensitivity)
	: cameraPos(0.0f, 250.0f, 0.0f),
	cameraFront(0.0f, 0.0f, -1.0f),
	cameraUp(0.0f, 1.0f, 0.0f),
	yaw(-90.0f),
	pitch(-89.f),
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
	const float camSpeed = 200.f * deltaTime;
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
		cameraPos.y -= camSpeed * .5;
		cameraPos.y = std::max(0.0f, cameraPos.y);
		return;
	case GLFW_KEY_SPACE: // space
		cameraPos.y += camSpeed * .5;
		cameraPos.y = std::min(300.0f, cameraPos.y);
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

std::vector<std::pair<int, int>> Camera::getVisibleChunks(int renderDistance) {
	std::vector<std::pair<int, int>> visibleChunks;
	auto planes = calculateFrustumPlanes();

	int centerX = static_cast<int>(cameraPos.x / 16);
	int centerZ = static_cast<int>(cameraPos.z / 16);

	for (int x = centerX - renderDistance; x <= centerX + renderDistance; ++x) {
		for (int z = centerZ - renderDistance; z <= centerZ + renderDistance; ++z) {
			if (isChunkVisible(std::make_pair(x, z), planes)) {
				visibleChunks.push_back({ x, z });
			}
		}
	}

	return visibleChunks;
}

std::array<glm::vec4, 6> Camera::calculateFrustumPlanes() const {
	std::array<glm::vec4, 6> planes;
	glm::mat4 comb = projection * view;

	// Extract planes
	// Right
	planes[0] = glm::row(comb, 3) - glm::row(comb, 0);
	// Left
	planes[1] = glm::row(comb, 3) + glm::row(comb, 0);
	// Top
	planes[2] = glm::row(comb, 3) - glm::row(comb, 1);
	// Bottom
	planes[3] = glm::row(comb, 3) + glm::row(comb, 1);
	// Far
	planes[4] = glm::row(comb, 3) - glm::row(comb, 2);
	// Near
	planes[5] = glm::row(comb, 3) + glm::row(comb, 2);

	// Normalize planes
	for (auto& plane : planes) {
		plane /= glm::length(glm::vec3(plane));
	}

	return planes;
}

bool Camera::isChunkVisible(const std::pair<int, int>& chunkCoord, const std::array<glm::vec4, 6>& planes) const {
	glm::vec3 boundingBox[8];
	boundingBox[0] = glm::vec3(chunkCoord.first * 16, 0, chunkCoord.second * 16);
	boundingBox[1] = glm::vec3(chunkCoord.first * 16 + 16, 0, chunkCoord.second * 16);
	boundingBox[2] = glm::vec3(chunkCoord.first * 16 + 16, 0, chunkCoord.second * 16 + 16);
	boundingBox[3] = glm::vec3(chunkCoord.first * 16, 0, chunkCoord.second * 16 + 16);
	boundingBox[4] = glm::vec3(chunkCoord.first * 16, 256, chunkCoord.second * 16);
	boundingBox[5] = glm::vec3(chunkCoord.first * 16 + 16, 256, chunkCoord.second * 16);
	boundingBox[6] = glm::vec3(chunkCoord.first * 16 + 16, 256, chunkCoord.second * 16 + 16);
	boundingBox[7] = glm::vec3(chunkCoord.first * 16, 256, chunkCoord.second * 16 + 16);

	for (const auto& plane : planes) {
		int outside = 0;
		for (int i = 0; i < 8; ++i) {
			if (glm::dot(glm::vec3(plane), boundingBox[i]) + plane.w < 0) {
				++outside;
			}
		}
		if (outside == 8) return false;
	}


	return true;
}
