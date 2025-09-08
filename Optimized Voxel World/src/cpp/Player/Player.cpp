#include "h/Player/Player.h"

Player::Player() :
	camera(nullptr),
	world(nullptr),
	entityAABBRenderer(nullptr),
	gravitationalAcceleration(70),
	jumpAcceleration(-15),
	horizontalAcceleration(5.f),
	xVelocity(0),
	yVelocity(0),
	zVelocity(0),
	currentGravitationalCollision(false),
	isJumping(false),
	playerChunksReady(false),
	gravityOn(true),
	blockUpdateDelay(0.125) // 8 block updates per second allowed
{
	lastChunkFractional = { {std::numeric_limits<int>::min(), false},  {std::numeric_limits<int>::min(), false}};
	playerAABB.half = { 0.3f, 0.9f, 0.3f };
	playerAABB.color = { 1.f, 0.f, 0.f };
}

void Player::update(float deltaTime) {
	glm::vec3 currPos = camera->getCameraPos();

	float chunkXPrecise = currPos.x > 0 ? currPos.x / 64 : (currPos.x - 63) / 64;
	float chunkZPrecise = currPos.z > 0 ? currPos.z / 64 : (currPos.z - 63) / 64;
	double chunkXInt, chunkZInt;
	float chunkXFractional = modf(chunkXPrecise, &chunkXInt);
	float chunkZFractional = modf(chunkZPrecise, &chunkZInt);

	bool chunkXFractionalBool = chunkXFractional > 0 ? (chunkXFractional > 0.5 ? true : false) : (abs(chunkXFractional) > 0.5 ? false : true);
	bool chunkZFractionalBool = chunkZFractional > 0 ? (chunkZFractional > 0.5 ? true : false) : (abs(chunkZFractional) > 0.5 ? false : true);

	double now = glfwGetTime();
	if (!playerChunksReady) setPlayerChunks();

	if (gravityOn) {
		bool differentChunkEntered = (chunkXInt != lastChunkFractional.first.first || chunkXFractionalBool != lastChunkFractional.first.second || chunkZInt != lastChunkFractional.second.first || chunkZFractionalBool != lastChunkFractional.second.second);
		if (differentChunkEntered || !playerChunksReady) {
			playerChunksReady = false;
			if (!playerChunksReady)
				setPlayerChunks();
		}

		if (playerChunksReady) {
			glm::vec3 origPos = currPos;

			// First move vertically
			currPos.y -= yVelocity * deltaTime;
			yVelocity += gravitationalAcceleration * deltaTime;

			if (isJumping) {
				yVelocity -= jumpAcceleration * deltaTime;
			}

			camera->setCameraPos(currPos);

			bool horizontal = checkForHorizontalCollision();
			bool gravitational = checkForGravitationalCollision();
			bool head = checkHeadCollision();

			if (horizontal) camera->setCameraPos(origPos);
			if (gravitational || head) {
				camera->setCameraPos(origPos);
				yVelocity = 0;
				isJumping = false;
			}

			currPos = camera->getCameraPos();
			origPos = currPos;

			// handle horizontal collisions
			currPos.x += xVelocity * deltaTime;
			currPos.z += zVelocity * deltaTime;

			camera->setCameraPos(currPos);

			bool horizontalCollision = checkForHorizontalCollision();
			if (horizontalCollision) { // code designed to allow you to slide along walls even if you are colliding
				camera->setCameraPos({ origPos.x, currPos.y, currPos.z });
				horizontalCollision = checkForHorizontalCollision();
				if (horizontalCollision) {
					camera->setCameraPos({ currPos.x, currPos.y, origPos.z });
					horizontalCollision = checkForHorizontalCollision();
					if (horizontalCollision) {
						camera->setCameraPos({ origPos.x, currPos.y, origPos.z });
					}
				}
			}
			lastChunkFractional = { {static_cast<int>(chunkXInt), chunkXFractionalBool}, {static_cast<int>(chunkZInt), chunkZFractionalBool} };
		}
	}
	
	updateEntityBox();
}

void Player::updateEntityBox() {
	glm::vec3 camPos = camera->getCameraPos();
	playerAABB.center = { camPos.x, camPos.y - playerAABB.half.y, camPos.z };
	entityAABBRenderer->submit(playerAABB);
}

BlockID Player::getBlockAt(int worldX, int worldY, int worldZ) {
	if (worldY > 255) return BlockID::AIR;
	if (worldY == 0) return BlockID::BEDROCK;

	int chunkX = ChunkUtils::worldToChunkCoord(worldX);
	int chunkZ = ChunkUtils::worldToChunkCoord(worldZ);
	std::pair<int, int> key = { chunkX, chunkZ };

	int accessX = std::numeric_limits<int>::min(), accessZ = std::numeric_limits<int>::min();
	for (int x = 0; x < 2; x++) {
		for (int z = 0; z < 2; z++) {
			if (playerChunks[x][z].first == key) {
				accessX = x;
				accessZ = z;
			}
		}
	}
	
	int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
	int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

	int flatIndex = ChunkUtils::flattenChunkCoords(localX, worldY, localZ, 0);

	return playerChunks[accessX][accessZ].second[flatIndex];
}

void Player::setPlayerChunks() {
	glm::vec3 currPos = camera->getCameraPos();
	float chunkXPrecise = currPos.x > 0 ? currPos.x / 64 : (currPos.x - 63) / 64;
	float chunkZPrecise = currPos.z > 0 ? currPos.z / 64 : (currPos.z - 63) / 64;
	double chunkXInt, chunkZInt;
	float chunkXFractional = modf(chunkXPrecise, &chunkXInt);
	float chunkZFractional = modf(chunkZPrecise, &chunkZInt);

	int incrementX = chunkXFractional > 0 ? (chunkXFractional > 0.5 ? 1 : -1) : (abs(chunkXFractional) > 0.5 ? -1 : 1);
	int incrementZ = chunkZFractional > 0 ? (chunkZFractional > 0.5 ? 1 : -1) : (abs(chunkZFractional) > 0.5 ? -1 : 1);

	playerChunks[0][0].first = { static_cast<int>(chunkXInt), static_cast<int>(chunkZInt) };
	playerChunks[0][1].first = { static_cast<int>(chunkXInt + incrementX), static_cast<int>(chunkZInt) };
	playerChunks[1][0].first = { static_cast<int>(chunkXInt), static_cast<int>(chunkZInt + incrementZ) };
	playerChunks[1][1].first = { static_cast<int>(chunkXInt + incrementX), static_cast<int>(chunkZInt + incrementZ) };

	playerChunksReady = true;
	for (int x = 0; x < 2; x++) {
		for (int z = 0; z < 2; z++) {
			playerChunks[x][z].second = world->getChunkVector(playerChunks[x][z].first);
			if (playerChunks[x][z].second.size() == 1) {
				playerChunksReady = false;
			}
		}
	}
}

bool Player::checkForGravitationalCollision() {
	glm::vec3 cameraPos = camera->getCameraPos();
	cameraPos.y = (float)floor(cameraPos.y - 1.8);
	int minCheckX = floor(cameraPos.x - 0.3);
	int minCheckZ = floor(cameraPos.z - 0.3);
	int maxCheckX = floor(cameraPos.x + 0.3);
	int maxCheckZ = floor(cameraPos.z + 0.3);

	for (int x = minCheckX; x <= maxCheckX; x++) {
		for (int z = minCheckZ; z <= maxCheckZ; z++) {
			BlockID block = getBlockAt(x, cameraPos.y, z);
			if (block != BlockID::AIR) {
				currentGravitationalCollision = true;
				return true;
			}
		}
	}

	currentGravitationalCollision = false;
	return false;
}

bool Player::checkHeadCollision() {
	glm::vec3 cameraPos = camera->getCameraPos();
	BlockID block = getBlockAt(floor(cameraPos.x), floor(cameraPos.y + 0.1), floor(cameraPos.z));

	if (block != BlockID::AIR) return true;
	return false;
}

bool Player::checkForHorizontalCollision() {
	glm::vec3 camPos = camera->getCameraPos();
	glm::vec3 playerAABBmin(camPos.x - 0.3, camPos.y - 1.8, camPos.z - 0.3);
	glm::vec3 playerAABBmax(camPos.x + 0.3, camPos.y, camPos.z + 0.3);

	int xMin = floor(playerAABBmin.x);
	int xMax = floor(playerAABBmax.x);
	int zMin = floor(playerAABBmin.z);
	int zMax = floor(playerAABBmax.z);
	int yMin = floor(playerAABBmin.y);
	int yMax = floor(playerAABBmax.y);

	for (int x = xMin; x <= xMax; x++) {
		for (int y = yMin; y <= yMax; y++) {
			if (getBlockAt(x, y, std::floor(camPos.z)) != BlockID::AIR) return true;
		}
	}

	for (int z = zMin; z <= zMax; z++) {
		for (int y = yMin; y <= yMax; y++) {
			if (getBlockAt(std::floor(camPos.x), y, z) != BlockID::AIR) return true;
		}
	}

	for (int x = xMin; x <= xMax; x++) {
		for (int z = zMin; z <= zMax; z++) {
			for (int y = yMin; y <= yMax; y++) {
				if (getBlockAt(x, y, z) != BlockID::AIR) return true;
			}
		}
	}

	return false;
}

void Player::processKeyboardInput(std::map<GLuint, bool> keyStates, float deltaTime) {
	double now = glfwGetTime();
	if (playerChunksReady) {
		if (keyStates[GLFW_MOUSE_BUTTON_LEFT]) { // Break block
			if (now - blockUpdateDelay > 0.143) {
				blockUpdateDelay = now;
				glm::vec3 result = raycast(camera->getCameraPos(), camera->getCameraFront(), 5, CastType::Break);
				if (result.x != std::numeric_limits<float>::min()) {
					world->breakBlock(result.x, result.y, result.z);
					playerChunksReady = false;
					setPlayerChunks();
				}
			}
		}
		if (keyStates[GLFW_MOUSE_BUTTON_RIGHT]) { // Place block
			if (now - blockUpdateDelay > 0.143) {
				blockUpdateDelay = now;
				glm::vec3 result = raycast(camera->getCameraPos(), camera->getCameraFront(), 5, CastType::Place);
				if (result.x != std::numeric_limits<float>::min()) {
					if (!checkAnyPlayerCollision(result)) {
						world->placeBlock(result.x, result.y, result.z, BlockID::DIRT);
						playerChunksReady = false;
						setPlayerChunks();
					}
				}
			}
		}
	}

	if (gravityOn) {
		if (keyStates[GLFW_KEY_SPACE] && !isJumping) jump();

		glm::vec3 moveDir(0.0f);
		glm::vec3 forwardDir = glm::normalize(glm::vec3(camera->getCameraFront().x, 0.0, camera->getCameraFront().z));
		glm::vec3 rightDir = glm::normalize(glm::cross(camera->getCameraFront(), camera->getCameraUp()));

		if (keyStates[GLFW_KEY_W]) moveDir += forwardDir;
		if (keyStates[GLFW_KEY_S]) moveDir -= forwardDir;
		if (keyStates[GLFW_KEY_A]) moveDir -= rightDir;
		if (keyStates[GLFW_KEY_D]) moveDir += rightDir;


		// Normalize moveDir for consistent speed in all directions
		if (glm::length(moveDir) > 0)
			moveDir = glm::normalize(moveDir) * horizontalAcceleration;
		else {
			xVelocity = 0;
			zVelocity = 0;
		}

		xVelocity = moveDir.x;
		zVelocity = moveDir.z;

		glm::vec2 currentVelocity(xVelocity, zVelocity);
		if (glm::length(currentVelocity) > horizontalAcceleration) {
			currentVelocity = glm::normalize(currentVelocity) * horizontalAcceleration;
		}

		xVelocity = currentVelocity.x;
		zVelocity = currentVelocity.y;
	}

	else if (!gravityOn) camera->processKeyboardInput(keyStates, deltaTime);
}

void Player::jump() {
	if (currentGravitationalCollision) {
		yVelocity = jumpAcceleration;
		isJumping = true;
	}
}

bool Player::checkAnyPlayerCollision(glm::vec3 blockPos) {
	glm::vec3 cameraPos = camera->getCameraPos();
	glm::vec3 playerAABBmin(cameraPos.x - 0.2, cameraPos.y - 1.8, cameraPos.z - 0.2);
	glm::vec3 playerAABBmax(cameraPos.x + 0.2, cameraPos.y + 0.1, cameraPos.z + 0.2);

	int xMin = floor(playerAABBmin.x);
	int xMax = floor(playerAABBmax.x);
	int zMin = floor(playerAABBmin.z);
	int zMax = floor(playerAABBmax.z);
	int yMin = floor(playerAABBmin.y);
	int yMax = floor(playerAABBmax.y);

	// Check for collision on the x-axis
	for (int x = xMin; x <= xMax; x++) {
		for (int y = yMin; y <= yMax; y++) {
			for (int z = zMin; z <= zMax; z++) {
				if (glm::vec3(x, y, z ) == blockPos) return true;
			}
		}
	}

	return false;
}


void Player::setCamera(Camera* c) {
	camera = c;
	camera->toggleFlying(false);
}

void Player::toggleGravity() {
	gravityOn = !gravityOn;
	camera->toggleFlying(!gravityOn);
}

/*
	Raycasting algorithm to determine what block the player is looking at.
	Algorithm implementation obtained from Kevin Reid, accessed March 2nd, 2024, at https://gamedev.stackexchange.com/questions/47362/cast-ray-to-select-block-in-voxel-game/49423#49423, altered for C++
	Algorithm designed by John Amanatides and Andrew Woo of University of Toronto
	"A Fast Voxel Traversal Algorithm for Ray Tracing" - http://www.cse.yorku.ca/~amana/research/grid.pdf accessed March 2nd, 2024
*/
constexpr int signum(float x) {
	return x > 0 ? 1 : x < 0 ? -1 : 0;
}

// intbound() from Will, https://gamedev.stackexchange.com/users/4129/will, works well with negative coordinates
float intbound(float s, float ds) {
	return (ds > 0 ? ceil(s) - s : s - floor(s)) / abs(ds);
}

glm::vec3 Player::raycast(glm::vec3 origin, glm::vec3 direction, float radius, CastType mode) {
	glm::vec3 noHit = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };

	// Cube containing origin point.
	int x = std::floor(origin.x);
	int y = std::floor(origin.y);
	int z = std::floor(origin.z);

	// Break apart direction vector into variables
	float dx = direction.x;
	float dy = direction.y;
	float dz = direction.z;

	// Direction to increment x, y, z when stepping
	int stepX = signum(dx);
	int stepY = signum(dy);
	int stepZ = signum(dz);

	float tMaxX = intbound(origin.x, dx);
	float tMaxY = intbound(origin.y, dy);
	float tMaxZ = intbound(origin.z, dz);

	// change in t
	float tDeltaX = stepX / dx;
	float tDeltaY = stepY / dy;
	float tDeltaZ = stepZ / dz;

	glm::vec3 face;

	if (dx == 0 && dy == 0 && dz == 0) {
		return noHit;
	}

	radius /= sqrt(dx * dx + dy * dy + dz * dz);

	while (stepY > 0 ? y < 255 : y >= 0) {
		if (y < 255 && y >= 0) {
			if (world->getBlockAtGlobal(x, y, z) != BlockID::AIR) {
				if (y != 0) {
					if (mode == CastType::Break) return { x, y, z };
					return {x + face[0], y + face[1], z + face[2]};
				}
				break;
			}
		}

		// tMaxX stores the t-value at which we cross a cube boundary along the
		// X axis, and similarly for Y and Z. Therefore, choosing the least tMax
		// chooses the closest cube boundary. Only the first case of the four
		// has been commented in detail.
		if (tMaxX < tMaxY) {
			if (tMaxX < tMaxZ) {
				if (tMaxX > radius) break;
				x += stepX;
				tMaxX += tDeltaX;
				face[0] = -stepX;
				face[1] = 0;
				face[2] = 0;
			}
			else {
				if (tMaxZ > radius) break;
				z += stepZ;
				tMaxZ += tDeltaZ;
				face[0] = 0;
				face[1] = 0;
				face[2] = -stepZ;
			}
		}
		else {
			if (tMaxY < tMaxZ) {
				if (tMaxY > radius) break;
				y += stepY;
				tMaxY += tDeltaY;
				face[0] = 0;
				face[1] = -stepY;
				face[2] = 0;
			}
			else {
				if (tMaxZ > radius) break;
				z += stepZ;
				tMaxZ += tDeltaZ;
				face[0] = 0;
				face[1] = 0;
				face[2] = -stepZ;
			}
		}
	}
	return noHit;
}
