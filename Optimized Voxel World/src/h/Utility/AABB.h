#pragma once

#include "h/external/glm/vec3.hpp"

struct AABB {
	glm::vec3 center;
	glm::vec3 half;
	glm::vec3 color;
	glm::vec3 velocity;
};