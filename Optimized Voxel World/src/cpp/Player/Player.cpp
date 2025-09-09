#include "h/Player/Player.h"

#include <chrono>
#include <iostream>

Player::Player() :
	camera(nullptr),
	world(nullptr),
	entityAABBRenderer(nullptr),
	entityTerrainCollision(nullptr),
	gravitationalAcceleration(70.f),
	jumpAcceleration(15.f),
	horizontalAcceleration(6.f),
	xVelocity(0),
	yVelocity(0),
	zVelocity(0),
	currentGravitationalCollision(false),
	isJumping(false),
	flightOn(false),
	lastBlockActionTime(0.0),
	blockActionCooldown(0.1),
	lastGravityToggleTime(0.0),
	gravityToggleCooldown(0.2)
{
	playerAABB.half = { 0.3f, 0.875f, 0.3f };
	playerAABB.color = { 1.f, 1.f, 1.f };
	playerAABB.velocity = { 0.f, 0.f, 0.f };
}

void Player::update(float deltaTime) {
	if (!flightOn)
		yVelocity -= gravitationalAcceleration * deltaTime;

	glm::vec3 currPos = camera->getCameraPos();

	// Keep the eye slightly below the top of the collider
	const float headroom = 0.10f;
	const float eyeOffset = playerAABB.half.y - headroom;

	playerAABB.center = { currPos.x, currPos.y - eyeOffset, currPos.z };
	updateEntityBox();
	playerAABB.velocity = { xVelocity, yVelocity, zVelocity };

	if (entityTerrainCollision) {
		auto res = entityTerrainCollision->sweepResolve(playerAABB, deltaTime);

		camera->setCameraPos({ res.newCenter.x, res.newCenter.y + eyeOffset, res.newCenter.z });

		xVelocity = res.newVelocity.x;
		yVelocity = res.newVelocity.y;
		zVelocity = res.newVelocity.z;

		currentGravitationalCollision = (res.contact.hitY && res.contact.normal.y > 0.f);
		if (!flightOn) {
			if (currentGravitationalCollision) {
				isJumping = false;
				if (yVelocity < 0.f) yVelocity = 0.f;
			}
		}
		else {
			currentGravitationalCollision = false;
			isJumping = false;
		}
	}
}

void Player::processKeyboardInput(const InputEvents& ev, float deltaTime) {
	double now = glfwGetTime();

	if (ev.toggleGravity) {
		lastGravityToggleTime = now;

		flightOn = !flightOn;
		if (!flightOn) {
			horizontalAcceleration = 6.f;
			gravitationalAcceleration = 70.f;
			jumpAcceleration = 15.f;
		}
		else {
			horizontalAcceleration = 300.f;
			gravitationalAcceleration = 0.f;
			jumpAcceleration = 0.f;
		}

		yVelocity = 0.f;
		isJumping = false;
	}

	auto keyStates = ev.playerStates;

	glm::vec3 moveDir(0.0f);
	glm::vec3 forwardDir = glm::normalize(glm::vec3(camera->getCameraFront().x, 0.0, camera->getCameraFront().z));
	glm::vec3 rightDir = glm::normalize(glm::cross(camera->getCameraFront(), camera->getCameraUp()));

	if (keyStates[GLFW_KEY_W]) moveDir += forwardDir;
	if (keyStates[GLFW_KEY_S]) moveDir -= forwardDir;
	if (keyStates[GLFW_KEY_A]) moveDir -= rightDir;
	if (keyStates[GLFW_KEY_D]) moveDir += rightDir;

	if (!flightOn)
		if (keyStates[GLFW_KEY_SPACE] && !isJumping) jump();

	if (flightOn) {
		float v = 0.f;
		if (keyStates[GLFW_KEY_SPACE])      v += horizontalAcceleration;
		if (keyStates[GLFW_KEY_LEFT_SHIFT]) v -= horizontalAcceleration;
		yVelocity = v;
	}

	if (keyStates[GLFW_MOUSE_BUTTON_LEFT]) { // Break block
		if (now - lastBlockActionTime > blockActionCooldown) {
			lastBlockActionTime = now;
			glm::vec3 result = raycast(camera->getCameraPos(), camera->getCameraFront(), 5, CastType::Break);
			if (result.x != std::numeric_limits<float>::min()) {
				world->breakBlock(result.x, result.y, result.z);
			}
		}
	}
	if (keyStates[GLFW_MOUSE_BUTTON_RIGHT]) { // Place block
		if (now - lastBlockActionTime > blockActionCooldown) {
			lastBlockActionTime = now;
			glm::vec3 result = raycast(camera->getCameraPos(), camera->getCameraFront(), 5, CastType::Place);
			if (result.x != std::numeric_limits<float>::min()) {
				if (!placementWouldOverlapAABB(result.x, result.y, result.z))
					world->placeBlock(result.x, result.y, result.z, BlockID::DIRT);
			}
		}
	}


	// Normalize moveDir for consistent speed in all directions
	if (glm::length(moveDir) > 0)
		moveDir = glm::normalize(moveDir) * horizontalAcceleration;
	else {
		xVelocity = 0;
		zVelocity = 0;
	}

	glm::vec2 currentVelocity(moveDir.x, moveDir.z);
	if (glm::length(currentVelocity) > horizontalAcceleration)
		currentVelocity = glm::normalize(currentVelocity) * horizontalAcceleration;

	xVelocity = currentVelocity.x;
	zVelocity = currentVelocity.y;

}

bool Player::placementWouldOverlapAABB(int x, int y, int z) {
	glm::vec3 center = { x + 0.5f, y + 0.5f, z + 0.5f };
	glm::vec3 half = { 0.5f, 0.5f, 0.5f };

	const glm::vec3& pCenter = playerAABB.center;
	const glm::vec3& pHalf = playerAABB.half;

	bool overlapX = std::abs(center.x - pCenter.x) <= (half.x + pHalf.x);
	bool overlapY = std::abs(center.y - pCenter.y) <= (half.y + pHalf.y);
	bool overlapZ = std::abs(center.z - pCenter.z) <= (half.z + pHalf.z);

	return overlapX && overlapY && overlapZ;
}

void Player::jump() {
	if (currentGravitationalCollision) {
		yVelocity = jumpAcceleration;
		isJumping = true;
	}
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
				if (mode == CastType::Break && y != 0) return { x, y, z };
				if (mode == CastType::Place) return {x + face[0], y + face[1], z + face[2]};
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
