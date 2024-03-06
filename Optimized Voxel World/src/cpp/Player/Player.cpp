#include "h/Player/Player.h"

Player::Player() :
	gravitationalAcceleration(70),
	verticalVelocity(0),
	xVelocity(0),
	zVelocity(0),
	horizontalAcceleration(5.f),
	gravityOn(true),
	currentGravitationalCollision(false),
	isJumping(false),
	jumpAcceleration(-15),
	breakBlockDelay(0)
{
	srand(time(NULL));
}

void Player::initialize() {
	prevKeyStates[GLFW_KEY_W] = false;
	prevKeyStates[GLFW_KEY_A] = false;
	prevKeyStates[GLFW_KEY_S] = false;
	prevKeyStates[GLFW_KEY_D] = false;
	prevKeyStates[GLFW_KEY_SPACE] = false;
	prevKeyStates[GLFW_MOUSE_BUTTON_LEFT] = false;
	prevKeyStates[GLFW_MOUSE_BUTTON_RIGHT] = false;
}

void Player::setCamera(Camera* c) {
	camera = c;
	camera->setMode(false);
}

void Player::setWorld(WorldManager* w) {
	world = w;
}

void Player::toggleGravity() {
	gravityOn = !gravityOn;
	camera->setMode(!gravityOn);
}

void Player::update(float deltaTime) {

	if (gravityOn) {
		glm::vec3 currPos = camera->getCameraPos();
		glm::vec3 origPos = currPos;

		// First move vertically
		currPos.y -= verticalVelocity * deltaTime;
		verticalVelocity += gravitationalAcceleration * deltaTime;

		if (isJumping) {
			verticalVelocity -= jumpAcceleration * deltaTime;
		}

		camera->setCameraPos(currPos);

		bool horizontal = checkForHorizontalCollision();
		bool gravitational = checkForGravitationalCollision();
		bool head = checkHeadCollision();
		if (horizontal) {
			camera->setCameraPos(origPos);
		}
		if (gravitational || head) {
			camera->setCameraPos(origPos);
			verticalVelocity = 0;
			isJumping = false;
		}

		currPos = camera->getCameraPos();
		origPos = currPos;

		// First handle horizontal collisions
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
	}
}

bool Player::checkForGravitationalCollision() {
	glm::vec3 cameraPos = camera->getCameraPos();
	cameraPos.y = floor(cameraPos.y - 1.8);
	int minCheckX = floor(cameraPos.x - 0.3);
	int minCheckZ = floor(cameraPos.z - 0.3);
	int maxCheckX = floor(cameraPos.x + 0.3);
	int maxCheckZ = floor(cameraPos.z + 0.3);

	for (int x = minCheckX; x <= maxCheckX; x++) {
		for (int z = minCheckZ; z <= maxCheckZ; z++) {
			unsigned char block = world->getBlockAtGlobal(x, cameraPos.y, z, false);
			if (block != 0 && block != 69) {
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
	unsigned char block = world->getBlockAtGlobal(floor(cameraPos.x), floor(cameraPos.y + 0.1), floor(cameraPos.z), false);
	if (block != 0 && block != 69) {
		return true;
	}

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

	// Check for collision on the x-axis
	for (int x = xMin; x <= xMax; x++) {
		for (int y = yMin; y <= yMax; y++) {
			if (world->getBlockAtGlobal(x, y, std::floor(camPos.z), false) != 0 &&
				world->getBlockAtGlobal(x, y, std::floor(camPos.z), false) != 69) {
				return true;
			}
		}
	}

	// Check for collision on the z-axis
	for (int z = zMin; z <= zMax; z++) {
		for (int y = yMin; y <= yMax; y++) {
			if (world->getBlockAtGlobal(std::floor(camPos.x), y, z, false) != 0 &&
				world->getBlockAtGlobal(std::floor(camPos.x), y, z, false) != 69) {
				return true;
			}
		}
	}

	// Check for collision both axis
	for (int x = xMin; x <= xMax; x++) {
		for (int z = zMin; z <= zMax; z++) {
			for (int y = yMin; y <= yMax; y++) {
				if (world->getBlockAtGlobal(x, y, z, false) != 0 &&
					world->getBlockAtGlobal(x, y, z, false) != 69) {
					return true;
				}
			}
		}
	}

	return false;
}

int signum(float x) { // Returns 1 if input is above 0, 0 if the number is 0, and -1 in all other cases
	return x > 0 ? 1 : x < 0 ? -1 : 0;
}

void Player::processKeyboardInput(std::map<GLuint, bool> keyStates,  float deltaTime) {
	float now = glfwGetTime();
	if (keyStates[GLFW_MOUSE_BUTTON_LEFT] && prevKeyStates[GLFW_MOUSE_BUTTON_LEFT]) { // Break block
		if (now - breakBlockDelay > 0.1) {
			breakBlockDelay = now;
			auto returnVecs = raycast(camera->getCameraPos(), camera->getCameraFront(), 5);
			world->breakBlock(returnVecs.first.x, returnVecs.first.y, returnVecs.first.z);
		}
	}
	else if (keyStates[GLFW_MOUSE_BUTTON_LEFT] && !prevKeyStates[GLFW_MOUSE_BUTTON_LEFT]) {
		breakBlockDelay = now;
		auto returnVecs = raycast(camera->getCameraPos(), camera->getCameraFront(), 5);
		world->breakBlock(returnVecs.first.x, returnVecs.first.y, returnVecs.first.z);

	}
	if (keyStates[GLFW_MOUSE_BUTTON_RIGHT] && prevKeyStates[GLFW_MOUSE_BUTTON_RIGHT]) { // Place block
		if (now - breakBlockDelay > 0.15) {
			breakBlockDelay = now;
			auto returnVecs = raycast(camera->getCameraPos(), camera->getCameraFront(), 5);
			glm::vec3 posToPlace = returnVecs.first + returnVecs.second;
			if (!checkAnyPlayerCollision(posToPlace)) {
				world->placeBlock(posToPlace.x, posToPlace.y, posToPlace.z, rand() % 5 + 1);
			}
		}
	}
	else if (keyStates[GLFW_MOUSE_BUTTON_RIGHT] && !prevKeyStates[GLFW_MOUSE_BUTTON_RIGHT]) {
		auto returnVecs = raycast(camera->getCameraPos(), camera->getCameraFront(), 5);
		glm::vec3 posToPlace = returnVecs.first + returnVecs.second;
		if (!checkAnyPlayerCollision(posToPlace)) {
			world->placeBlock(posToPlace.x, posToPlace.y, posToPlace.z, rand() % 5 + 1);
		}
		breakBlockDelay = now;
	}

	if (gravityOn) {
		if (keyStates[GLFW_KEY_SPACE]) {
			if (!isJumping)
				jump();
		}

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
	else if (!gravityOn) {
		camera->processKeyboardInput(keyStates, deltaTime);
	}

	prevKeyStates = keyStates;
}

void Player::jump() {
	if (currentGravitationalCollision) {
		verticalVelocity = jumpAcceleration;
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
				if (glm::vec3(x, y, z ) == blockPos) {
					return true;
				}
			}
		}
	}

	return false;
}

int mod(int value, int modulus) { // Fixes negative modulus issue
	return (value % modulus + modulus) % modulus;
}

/*
intbound() from Will, https://gamedev.stackexchange.com/users/4129/will, works well with negative coordinates
*/

float intbound(float s, float ds) {
	return (ds > 0 ? ceil(s) - s : s - floor(s)) / abs(ds);
}

/*
	Raycasting algorithm to determine what block the player is looking at.
	Algorithm implementation obtained from Kevin Reid, accessed March 2nd, 2024, at https://gamedev.stackexchange.com/questions/47362/cast-ray-to-select-block-in-voxel-game/49423#49423, altered for C++
	Algorithm designed by John Amanatides and Andrew Woo of University of Toronto
	"A Fast Voxel Traversal Algorithm for Ray Tracing" - http://www.cse.yorku.ca/~amana/research/grid.pdf accessed March 2nd, 2024
*/

std::pair<glm::vec3, glm::vec3> Player::raycast(glm::vec3 origin, glm::vec3 direction, float radius) {
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
		return { {NULL, NULL, NULL}, {NULL, NULL, NULL} };
	}

	radius /= sqrt(dx * dx + dy * dy + dz * dz);

	while (stepY > 0 ? y < 255 : y >= 0) {
		if (y < 255 && y >= 0) {
			if (world->getBlockAtGlobal(x, y, z, false) != 0 && world->getBlockAtGlobal(x, y, z, false) != 69) {
				if (y != 0) {
					return { {x, y, z}, {face} };
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
	return { {NULL, NULL, NULL}, {NULL, NULL, NULL} };
}
