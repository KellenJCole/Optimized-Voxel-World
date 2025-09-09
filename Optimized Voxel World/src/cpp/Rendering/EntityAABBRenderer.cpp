#include "h/Rendering/EntityAABBRenderer.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"
#include <array>

EntityAABBRenderer::EntityAABBRenderer()
{

}

bool EntityAABBRenderer::initialize(const char* shaderPath) {
	boxShader_ = Shader(shaderPath);

	static const float verts[] = {
		-1,-1,-1,  1,-1,-1,  -1, 1,-1,  1, 1,-1,
		-1,-1, 1,  1,-1, 1,  -1, 1, 1,  1, 1, 1
	};

	static const unsigned int idx[] = {
		0,1, 1,3, 3,2, 2,0,
		4,5, 5,7, 7,6, 6,4,
		0,4, 1,5, 2,6, 3,7
	};

	vao_.create();
	vao_.Bind();

	vbVerts_.create(verts, sizeof(verts));
	VertexBufferLayout pos;
	pos.PushFloat(3);
	vao_.AddBufferAt(vbVerts_, pos, 0);

	ibo_.create(idx, (unsigned)std::size(idx));

	vbInst_.create(nullptr, 0);
	VertexBufferLayout inst;
	inst.PushFloat(3); // center
	inst.PushFloat(3); // half
	inst.PushFloat(3); // color
	vao_.AddBufferInstancedAt(vbInst_, inst, 1, 1);

	vao_.Unbind();
	return true;
}

void EntityAABBRenderer::beginFrame(int maxBoxes) {
	instances_.clear();
	if (maxBoxes > 0) instances_.reserve((size_t)maxBoxes);
}

void EntityAABBRenderer::submit(const AABB& b) {
	instances_.push_back(b);
}

void EntityAABBRenderer::draw(glm::mat4& view, glm::mat4& projection, float lineWidth) {
	if (instances_.empty()) return;

	boxShader_.use();
	boxShader_.setUniform4fv("view", view);
	boxShader_.setUniform4fv("projection", projection);

	vao_.Bind();
	vbInst_.update(instances_.data(), (unsigned)(instances_.size() * sizeof(AABB)));

	GLCall(glLineWidth(lineWidth));
	GLCall(glDrawElementsInstanced(GL_LINES,
		ibo_.GetCount(),
		GL_UNSIGNED_INT,
		(void*)0,
		(GLsizei)instances_.size()));
	vao_.Unbind();
}

void EntityAABBRenderer::destroy() {
	vao_.Unbind();
	ibo_.destroy();
	vbInst_.destroy();
	vbVerts_.destroy();
	vao_.destroy();
	boxShader_.deleteProgram();
}