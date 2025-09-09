#pragma once
#include "h/external/glm/vec3.hpp"
#include "h/Utility/AABB.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Buffering/VertexArray.h"
#include "h/Rendering/Buffering/VertexBuffer.h"
#include "h/Rendering/Buffering/IndexBuffer.h"
#include "h/Rendering/Buffering/VertexBufferLayout.h"

#include <vector>

class EntityAABBRenderer {
public:
	EntityAABBRenderer();
	bool initialize(const char* shaderPath);
	void destroy();

	void beginFrame(int maxBoxes);
	void submit(const AABB& b);
	
	void draw(glm::mat4& view, glm::mat4& projection, float lineWidth);
private:
	Shader boxShader_;
	VertexArray vao_;
	VertexBuffer vbVerts_;
	VertexBuffer vbInst_;
	IndexBuffer ibo_;

	std::vector<AABB> instances_;
};
