#pragma once

#include "h/external/glm/glm.hpp"
#include "h/external/glm/gtc/matrix_access.hpp"
#include "h/external/glm/gtc/matrix_transform.hpp"
#include "h/Rendering/Utility/WindowConfig.h"
#include "h/Terrain/Utility/ChunkUtils.h"

#include <array>
#include <vector>
#include <map>

class Camera {
public:
	Camera();
	Camera(float inputSensitivity);

	// Update view matrix based off camera position and camera orientation
	void update();

	// Process user input
	void processKeyboardInput(std::map<unsigned int, bool> keyStates, float deltaTime);
	void processMouseMovement(double xPos, double yPos);

	void setCameraPos(glm::vec3 pos);

	inline void toggleFlying(bool fly) { flying = fly; }

	// Getters for camera properties
	glm::vec3 getCameraPos() const { return cameraPos; }
	glm::vec3 getCameraFront() const { return cameraFront; }
	glm::vec3 getCameraUp() const { return cameraUp; }
	glm::mat4 getProjection() const { return projection; }
	glm::mat4 getView() const { return view; }
	std::vector<std::pair<int, int>> getVisibleChunks(int renderDistance);

private:
	bool isChunkVisible(const std::pair<int, int>& chunkCoord, const std::array<glm::vec4, 6>& planes) const;
	std::array<glm::vec4, 6> calculateFrustumPlanes() const;

	std::array<glm::vec4, 6> frustumPlanes;
	glm::vec3 cameraPos; // The point in 3D space at which the camera resides
	glm::vec3 cameraFront; // The direction the camera is facing.
	glm::vec3 cameraUp; // The up direction vector. It never changes.
	glm::mat4 projection; // Responsible for converting 3D coordinates into 2D coordinates
	glm::mat4 view; // Used to transform the entire scene so that it appears from the camera's POV.

	float sensitivity;
	float yaw;
	float pitch;
	float lastX, lastY;
	bool firstMouse;
	bool flying;

	// Internal helper function
	void updateCameraVectors();
};
