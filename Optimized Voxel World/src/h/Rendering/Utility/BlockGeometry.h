#pragma once
#include <h/external/glm/glm.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec2 texCoords;
	int normalBlockType; // blockType = blockType * 16
};

class BlockGeometry {
public:
	static const float vertices[192];
	static const unsigned int indices[6][6];
};