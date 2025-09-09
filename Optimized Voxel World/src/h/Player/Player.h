#pragma once

#include "h/Physics/EntityTerrainCollision.h"
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Camera.h"
#include "h/Terrain/Utility/ChunkUtils.h"
#include "h/Rendering/EntityAABBRenderer.h"
#include "h/Engine/InputManager.h"

#include <map>
#include <mutex>


enum class CastType { Break, Place }; // for raycasting
class Player {
public:
	Player();
	void setCamera(Camera* c) { camera = c; }
	void setWorld(WorldManager* w) { world = w; }
	void setEntityAABBRenderer(EntityAABBRenderer* e) { entityAABBRenderer = e; }
	void setEntityTerrainCollisionPtr(EntityTerrainCollision* etc) { entityTerrainCollision = etc; }
	void update(float deltaTime);
	void processKeyboardInput(const InputEvents& ev, float deltaTime);
private:
	void jump();
	void updateEntityBox() { entityAABBRenderer->submit(playerAABB); }
	bool placementWouldOverlapAABB(int x, int y, int z);

	glm::vec3 raycast(glm::vec3 origin, glm::vec3 direction, float radius, CastType mode); // Returns in the first pair slot the block coordinates, in the second, the face info

	// Object access pointers
	Camera* camera;
	WorldManager* world;
	EntityAABBRenderer* entityAABBRenderer;
	EntityTerrainCollision* entityTerrainCollision;

	AABB playerAABB;

	// Acceleration constants
	float gravitationalAcceleration;
	float jumpAcceleration;
	float horizontalAcceleration;

	// Current velocity values
	float xVelocity;
	float yVelocity;
	float zVelocity;

	// Player action state values
	bool currentGravitationalCollision;
	bool isJumping;

	// Miscellanious
	bool flightOn;
	double lastBlockActionTime;
	double blockActionCooldown;
	
	double lastGravityToggleTime;
	double gravityToggleCooldown;
};