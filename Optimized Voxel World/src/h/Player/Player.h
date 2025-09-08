#pragma once

#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Camera.h"
#include "h/Terrain/Utility/ChunkUtils.h"
#include "h/Rendering/EntityAABBRenderer.h"

#include <map>
#include <mutex>


enum class CastType { Break, Place }; // for raycasting
class Player {
public:
	Player();
	void setCamera(Camera* c);
	void setWorld(WorldManager* w) { world = w; }
	void setEntityAABBRenderer(EntityAABBRenderer* e) { entityAABBRenderer = e; }
	void toggleGravity();
	void update(float deltaTime);
	void processKeyboardInput(std::map<GLuint, bool> keyStates, float deltaTime);
private:
	void jump();
	bool checkForGravitationalCollision();
	bool checkForHorizontalCollision();
	bool checkHeadCollision();
	bool checkAnyPlayerCollision(glm::vec3 blockPos);
	void setPlayerChunks();
	BlockID getBlockAt(int worldX, int worldY, int worldZ);
	void updateEntityBox();

	glm::vec3 raycast(glm::vec3 origin, glm::vec3 direction, float radius, CastType mode); // Returns in the first pair slot the block coordinates, in the second, the face info

	// Object access pointers
	Camera* camera;
	WorldManager* world;
	EntityAABBRenderer* entityAABBRenderer;

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
	bool playerChunksReady;
	bool gravityOn;
	float blockUpdateDelay;

	std::pair<std::pair<int, bool>, std::pair<int, bool>> lastChunkFractional;

	std::pair<std::pair<int, int>, std::vector<BlockID>> playerChunks[2][2];
};