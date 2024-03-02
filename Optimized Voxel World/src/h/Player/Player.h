#pragma once
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Camera.h"

class Player {
public:
	Player();
	void setCamera(Camera* c);
	void setWorld(WorldManager* w);
	void toggleGravity();
	void update(float deltaTime);
	void processKeyboardInput(int key, float deltaTime);
private:
	void raycast(glm::vec3 origin, glm::vec3 direction, float radius); // callback?
	void jump();
	bool checkForGravitationalCollision();
	bool checkForHorizontalCollision();
	bool checkHeadCollision();
	float gravitationalAcceleration;
	float gravitationalVelocity;
	bool gravityOn;
	bool currentGravitationalCollision;
	float jumpAcceleration;
	float jumpVelocity;
	bool isJumping;
	Camera* camera;
	WorldManager* world;
	float breakBlockDelay;
};