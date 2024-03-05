#pragma once
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Camera.h"
#include <map>

class Player {
public:
	Player();
	void setCamera(Camera* c);
	void setWorld(WorldManager* w);
	void toggleGravity();
	void update(float deltaTime);
	void processKeyboardInput(std::map<GLuint, bool> keyStates, float deltaTime);
private:
	void raycast(glm::vec3 origin, glm::vec3 direction, float radius); // callback?
	void jump();
	bool checkForGravitationalCollision();
	bool checkForHorizontalCollision();
	bool checkHeadCollision();

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