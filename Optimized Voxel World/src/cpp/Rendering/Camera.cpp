#include "h/Rendering/Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera() {}

Camera::Camera(float inputSensitivity)
	: cameraPos(0.5f, 250.0f, 0.5f),
	cameraFront(0.0f, 0.0f, -1.0f),
	cameraUp(0.0f, 1.0f, 0.0f),
	yaw(-90.0f),
	pitch(-89.f),
	firstMouse(true),
	sensitivity(inputSensitivity)
{
	projection = glm::perspective(glm::radians((float)WindowDetails::FOV), (float)WindowDetails::WindowWidth / (float)WindowDetails::WindowHeight, 0.1f, 20000.0f);
	view = glm::mat4(1.0f);
	updateCameraVectors(); 
	mode = false; // gravity on 

}

void Camera::update()
{
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::processKeyboardInput(std::map<unsigned int, bool> keyStates, float deltaTime)
{
	float camSpeed;
	if (mode) {
		camSpeed = 300.f * deltaTime;
		glm::vec3 moveDir;
		float originalY = cameraPos.y;

		if (keyStates[GLFW_KEY_W]) {
			moveDir = glm::normalize(glm::vec3(cameraFront.x, 0.0, cameraFront.z));
			cameraPos += moveDir * camSpeed;
		}

		if (keyStates[GLFW_KEY_S]) {
			moveDir = -glm::normalize(glm::vec3(cameraFront.x, 0.0, cameraFront.z));
			cameraPos += moveDir * camSpeed;
		}

		if (keyStates[GLFW_KEY_A]) {
			moveDir = -glm::normalize(glm::cross(cameraFront, cameraUp));
			cameraPos += moveDir * camSpeed;
		}

		if (keyStates[GLFW_KEY_D]) {
			moveDir = glm::normalize(glm::cross(cameraFront, cameraUp));
			cameraPos += moveDir * camSpeed;
		}

		if (keyStates[GLFW_KEY_LEFT_SHIFT]) {
			cameraPos.y -= camSpeed * 0.5;
			return;
		}
		if (keyStates[GLFW_KEY_SPACE]) {
			cameraPos.y += camSpeed * 0.5;
			return;
		}

		cameraPos += moveDir * camSpeed;
	}
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

	int centerX = ChunkUtils::worldToChunkCoord(static_cast<int>(floor(cameraPos.x)));
	int centerZ = ChunkUtils::worldToChunkCoord(static_cast<int>(floor(cameraPos.z)));

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
	boundingBox[0] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH,						0,						chunkCoord.second * ChunkUtils::DEPTH);
	boundingBox[1] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH + ChunkUtils::WIDTH,	0,						chunkCoord.second * ChunkUtils::DEPTH);
	boundingBox[2] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH + ChunkUtils::WIDTH,	0,						chunkCoord.second * ChunkUtils::DEPTH + ChunkUtils::DEPTH);
	boundingBox[3] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH,						0,						chunkCoord.second * ChunkUtils::DEPTH + ChunkUtils::DEPTH);
	boundingBox[4] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH,						ChunkUtils::HEIGHT,		chunkCoord.second * ChunkUtils::DEPTH);
	boundingBox[5] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH + ChunkUtils::WIDTH,	ChunkUtils::HEIGHT,		chunkCoord.second * ChunkUtils::DEPTH);
	boundingBox[6] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH + ChunkUtils::WIDTH,	ChunkUtils::HEIGHT,		chunkCoord.second * ChunkUtils::DEPTH + ChunkUtils::DEPTH);
	boundingBox[7] = glm::vec3(chunkCoord.first * ChunkUtils::WIDTH,						ChunkUtils::HEIGHT,		chunkCoord.second * ChunkUtils::DEPTH + ChunkUtils::DEPTH);

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

void Camera::setCameraPos(glm::vec3 pos) {
	cameraPos = pos;
}
