#pragma once
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Camera.h"
#include <map>

class Player {
public:
	Player();
	void initialize();
	void setCamera(Camera* c);
	void setWorld(WorldManager* w);
	void toggleGravity();
	void update(float deltaTime);
	void processKeyboardInput(std::map<GLuint, bool> keyStates, float deltaTime);
private:
	std::pair<glm::vec3, glm::vec3> raycast(glm::vec3 origin, glm::vec3 direction, float radius); // Returns in the first pair slot the block coordinates, in the second, the face info
	void jump();
	bool checkForGravitationalCollision();
	bool checkForHorizontalCollision();
	bool checkHeadCollision();
	bool checkAnyPlayerCollision(glm::vec3 blockPos);

	std::map<GLuint, bool> prevKeyStates;
	float gravitationalAcceleration;
	float jumpAcceleration;
	float verticalVelocity;
	float horizontalAcceleration;
	float xVelocity;
	float zVelocity;
	bool gravityOn;
	bool currentGravitationalCollision;
	bool isJumping;
	Camera* camera;
	WorldManager* world;
	float breakBlockDelay;
};