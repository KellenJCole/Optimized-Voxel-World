#pragma once
#include <vector>
#include "h/Rendering/VertexArray.h"
#include "h/Rendering/VertexBuffer.h"
#include "h/Rendering/VertexBufferLayout.h"
#include "h/Rendering/IndexBuffer.h"
#include "h/Rendering/InstanceBuffer.h"
#include "h/Rendering/BlockGeometry.h"
#include <h/glm/glm.hpp>

class BlockGeometry;
class Renderer {
public:
	Renderer();
	bool initialize();
	//void passMouseMovementToRendererCamera(double xpos, double ypos);
	//void passKeyboardInputToRendererCamera();
	void instancedRenderByFace(std::vector<glm::vec3> translations, int faceType, int blockType);
	void cleanup();
private:
	VertexArray vertexArray;
	VertexBuffer vertexBuffer;
	VertexBufferLayout layout;
	IndexBuffer indexBuffer[6];
	InstanceBuffer instanceBuffer;
	
};