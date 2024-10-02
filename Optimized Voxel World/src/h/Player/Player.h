#pragma once
#include "h/Terrain/WorldManager.h"
#include "h/Rendering/Camera.h"
#include <map>
#include <mutex>

class Player {
public:
	Player(std::recursive_mutex& wmm);
	void initialize();
	void setCamera(Camera* c);
	void setWorld(WorldManager* w);
	void toggleGravity();
	void update(float deltaTime);
	void setPlayerChunks();
	void processKeyboardInput(std::map<GLuint, bool> keyStates, float deltaTime);
private:
	std::pair<glm::vec3, glm::vec3> raycast(glm::vec3 origin, glm::vec3 direction, float radius); // Returns in the first pair slot the block coordinates, in the second, the face info
	void jump();
	bool checkForGravitationalCollision();
	bool checkForHorizontalCollision();
	bool checkHeadCollision();
	bool checkAnyPlayerCollision(glm::vec3 blockPos);
	unsigned char getBlockAt(int worldX, int worldY, int worldZ);
	int convertWorldCoordToChunkCoord(int worldCoord);
	int convert3DCoordinatesToFlatIndex(int x, int y, int z);

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
	bool gravityWasOff;
	Camera* camera;
	WorldManager* world;
	float breakBlockDelay;
	std::recursive_mutex& worldMapMutex;
	std::pair<std::pair<int, bool>, std::pair<int, bool>> lastChunkFractional;

	std::pair<std::pair<int, int>, std::vector<unsigned char>> playerChunks[2][2];
};