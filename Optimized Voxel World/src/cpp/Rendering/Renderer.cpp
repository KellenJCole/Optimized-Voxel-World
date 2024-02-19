#include "h/Rendering/Renderer.h"
#include "h/Rendering/GLErrorCatcher.h"

Renderer::Renderer() {
}

bool Renderer::initialize() {

	vertexArray.create();
	vertexArray.Bind();
	vertexBuffer.create(BlockGeometry::vertices, sizeof(BlockGeometry::vertices));
	//vertexBuffer.Bind();
	/*
	IndexBuffer index
				0	-x faces
				1	+x faces
				2	-y faces
				3	+y faces
				4	-z faces
				5	+z faces
	*/
	for (int i = 0; i < 6; i++) {
		indexBuffer[i].create(BlockGeometry::indices[i], 6);
	}

	layout.Push<float>(3);
	layout.Push<float>(2);
	layout.Push<float>(3);
	vertexArray.AddBuffer(vertexBuffer, layout);

	instanceBuffer.create(5000000);
	vertexArray.Unbind();

	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));				// Set to GL_LINE to view mesh.
	GLCall(glEnable(GL_FRAMEBUFFER_SRGB));							 

	return true;
}

void Renderer::instancedRenderByFace(std::vector<glm::vec3> translations, int faceType, int blockType) {
	if (translations.size() == 0) {
		return;
	}

	vertexArray.Bind();
	instanceBuffer.update(translations.data(), translations.size() * sizeof(glm::vec3));

	IndexBuffer* tempIndexBufferPtr = nullptr;
	tempIndexBufferPtr = &indexBuffer[faceType];
	tempIndexBufferPtr->Bind();

	GLCall(glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(translations.size())));

	tempIndexBufferPtr->Unbind();
	vertexArray.Unbind();
}

void Renderer::cleanup() {
	vertexArray.destroy();
	vertexBuffer.destroy();
	for (int i = 0; i < 6; i++) indexBuffer[i].destroy();
}