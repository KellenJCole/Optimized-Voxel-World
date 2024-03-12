#pragma once
#include "h/Rendering/VertexArray.h"
#include "h/Rendering/VertexBuffer.h"
#include "h/Rendering/IndexBuffer.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/VertexBufferLayout.h"
#include <vector>
#include <map>
#include "h/glm/glm.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    int normalBlockType; // blockType = blockType * 16
};

/*
Requirements of this class:
Take in a map of all the transparent meshes in the world, grouped by plane
Sort by distance from camera. 
Should probably be updated 
- within chunk and 3 closest chunks (arranged in an L around chunk) everytime player moves at all, 
- within 3 chunks everytime player steps over block boundary 
- within 10 chunks everytime player moves between chunks

As water will likely be 99.99% of transparent faces, we will only make it a transparent block within perhaps 100 blocks of player. For bonus points we can change it's alpha based off distance from player, could be done in shader.
*/

class Transparencies {
public:
	Transparencies();
private:
    VertexArray vertexArray;
    VertexBufferLayout layout;
    std::map<std::pair<int, int>, VertexBuffer> chunkToVertexBuffer;
    std::map<std::pair<int, int>, IndexBuffer> chunkToIndexBuffer;
    std::vector<std::pair<int, int>> chunksBeingRendered;

    Shader* transparencyShader;
	std::map<std::pair<int, int>, std::vector<Vertex>> meshMap;
};