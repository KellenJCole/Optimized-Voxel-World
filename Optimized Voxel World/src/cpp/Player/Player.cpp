#include "h/Player/Player.h"

Player::Player() :
	gravitationalAcceleration(70),
	gravitationalVelocity(0),
	gravityOn(true),
	currentGravitationalCollision(false),
	isJumping(false),
	jumpAcceleration(20),
	jumpVelocity(0),
	breakBlockDelay(0)
{
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
	camera->setMode(false);
}

void Player::update(float deltaTime) {
	if (gravityOn) {
		glm::vec3 currPos = camera->getCameraPos();
		glm::vec3 origPos = currPos;
		currPos.y -= (gravitationalVelocity - jumpVelocity) * deltaTime;
		gravitationalVelocity += gravitationalAcceleration * deltaTime;
		if (jumpVelocity > 0) {
			jumpVelocity -= jumpAcceleration * deltaTime;
		}
		else {
			jumpVelocity = 0;
		}

		camera->setCameraPos(currPos);
		if (checkForGravitationalCollision()) {
			camera->setCameraPos(origPos);
			gravitationalVelocity = 0;
			jumpVelocity = 0;
			isJumping = false;
		}
		else if (checkHeadCollision()) {
			jumpVelocity = 0;
			isJumping = false;
		}
	}

}

bool Player::checkForGravitationalCollision() {
	glm::vec3 cameraPos = camera->getCameraPos();
	cameraPos.y = std::floor(cameraPos.y - 1.8);
	int minCheckX = std::floor(cameraPos.x - 0.2);
	int minCheckZ = std::floor(cameraPos.z - 0.2);
	int maxCheckX = std::floor(cameraPos.x + 0.2);
	int maxCheckZ = std::floor(cameraPos.z + 0.2);

	for (minCheckX; minCheckX <= maxCheckX; minCheckX++) {
		for (minCheckZ; minCheckZ <= maxCheckZ; minCheckZ++) {
			unsigned char block = world->getBlockAtGlobal(minCheckX, cameraPos.y, minCheckZ, false);
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
	cameraPos.y = std::floor(cameraPos.y + 0.2);
	unsigned char block = world->getBlockAtGlobal(std::floor(cameraPos.x), cameraPos.y + 1, std::floor(cameraPos.z), false);
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

	// Check for collision on the x-axis
	for (int x = xMin; x <= xMax; x++) {
		if (world->getBlockAtGlobal(x, std::floor(camPos.y) - 1, std::floor(camPos.z), false) != 0 &&
			world->getBlockAtGlobal(x, std::floor(camPos.y) - 1, std::floor(camPos.z), false) != 69) {
			return true;
		}
		if (world->getBlockAtGlobal(x, std::floor(camPos.y), std::floor(camPos.z), false) != 0 &&
			world->getBlockAtGlobal(x, std::floor(camPos.y), std::floor(camPos.z), false) != 69) {
			return true;
		}
	}

	// Check for collision on the z-axis
	for (int z = zMin; z <= zMax; z++) {
		if (world->getBlockAtGlobal(std::floor(camPos.x), std::floor(camPos.y) - 1, z, false) != 0 &&
			world->getBlockAtGlobal(std::floor(camPos.x), std::floor(camPos.y) - 1, z, false) != 69) {
			return true;
		}
		if (world->getBlockAtGlobal(std::floor(camPos.x), std::floor(camPos.y), z, false) != 0 &&
			world->getBlockAtGlobal(std::floor(camPos.x), std::floor(camPos.y), z, false) != 69) {
			return true;
		}
	}

	return false;
}

void Player::processKeyboardInput(int key, float deltaTime) {
	glm::vec3 beforePos = camera->getCameraPos();
	if (key == GLFW_KEY_SPACE) {
		if (!isJumping) {
			jump();
		}
		return;
	}
	else if (key == GLFW_MOUSE_BUTTON_1) {
		float now = glfwGetTime();
		if (now - breakBlockDelay > 0.001) {
			breakBlockDelay = now;
			raycast(camera->getCameraPos(), camera->getCameraFront(), 5);
		}
	}
	camera->processKeyboardInput(key, deltaTime);

	bool colliding = checkForHorizontalCollision();

	/*
	*
	* This is an awkward method of allowing the player to slide along walls (instead of entirely blocking movement when it results in a horizontal collision)
	* but it works well enough so I'll explain why this code is the way it is
	* 
	* The player is first allowed to move wherever. We next check if we are horizontally colliding. If we are, we attempt to allow the x-axis movement. If we are still colliding, we reset the x position and
	* attempt z-axis movement. If we are still colliding, we reset the player back to where they were before any collision occured.
	* 
	*/

	if (colliding) {
		glm::vec3 currPos = camera->getCameraPos();
		camera->setCameraPos({ beforePos.x, currPos.y, currPos.z });
		colliding = checkForHorizontalCollision();
		if (colliding) {
			camera->setCameraPos({ currPos.x, currPos.y, beforePos.z });
			colliding = checkForHorizontalCollision();
			if (colliding) {
				camera->setCameraPos(beforePos);
			}
		}
	}
}

void Player::jump() {
	if (currentGravitationalCollision) {
		jumpVelocity = jumpAcceleration;
		isJumping = true;
	}
}

int mod(int value, int modulus) { // Fixes negative modulus issue
	return (value % modulus + modulus) % modulus;
}

float intbound(float s, float ds) {
	// Find the smallest positive t such that s+t*ds is an integer.
	if (ds < 0) {
		return intbound(-s, -ds);
	}
	else {
		s = mod(s, 1);
		// problem is now s+t*ds = 1
		return (1 - s) / ds;
	}
}

int signum(float x) { // Returns 1 if input is above 0, 0 if the number is 0, and -1 in all other cases
	return x > 0 ? 1 : x < 0 ? -1 : 0;
}

/*
	Raycasting algorithm to determine what block the player is looking at.
	Algorithm implementation obtained from Kevin Reid, accessed March 2nd, 2024, at https://gamedev.stackexchange.com/questions/47362/cast-ray-to-select-block-in-voxel-game/49423#49423, altered for C++
	Algorithm designed by John Amanatides and Andrew Woo of University of Toronto
	"A Fast Voxel Traversal Algorithm for Ray Tracing" - http://www.cse.yorku.ca/~amana/research/grid.pdf accessed March 2nd, 2024
*/

void Player::raycast(glm::vec3 origin, glm::vec3 direction, float radius) {

	// Cube containing origin point.
	int x = std::floor(origin.x);
	int y = std::floor(origin.y);
	int z = std::floor(origin.z);

	// Break apart direction vector into variables
	float dx = direction.x;
	float dy = direction.y;
	float dz = direction.z;

	// Direction to increment x, y, z when stepping
	float stepX = signum(dx);
	float stepY = signum(dy);
	float stepZ = signum(dz);

	float tMaxX = intbound(origin.x, dx);
	float tMaxY = intbound(origin.y, dy);
	float tMaxZ = intbound(origin.z, dz);

	// change in t
	float tDeltaX = stepX / dx;
	float tDeltaY = stepY / dy;
	float tDeltaZ = stepZ / dz;

	glm::vec3 face;

	if (dx == 0 && dy == 0 && dz == 0) {
		return;
	}

	radius /= sqrt(dx * dx + dy * dy + dz * dz);

	while (stepY > 0 ? y < 255 : y >= 0) {

		if (!(y < 255 && y >= 0))
			std::cout << x << ", " << y << ", " << z;
			if (world->getBlockAtGlobal(x, y, z, false) != 0 && world->getBlockAtGlobal(x, y, z, false) != 69) {
				if (y != 0) {
					world->breakBlock(x, y, z);
				}
				break;
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
}
