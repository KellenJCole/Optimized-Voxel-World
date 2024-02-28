#include "h/Rendering/VertexBuffer.h"
#include "h/Rendering/GLErrorCatcher.h"
#include <iostream>

VertexBuffer::VertexBuffer() : vertexBuffer_id(0), valid(false) {

}

bool VertexBuffer::isValid() {
	return valid;
}

void VertexBuffer::create(const void* data, unsigned int size) {
	GLCall(glGenBuffers(1, &vertexBuffer_id));
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW)); // Directly use data if provided
	Unbind();
	valid = true;
}

void VertexBuffer::clear() {
	Bind();
	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
}

// For full replacement
void VertexBuffer::update(const void* data, unsigned int size) {
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW)); // For full replacement
}

void VertexBuffer::destroy() {
	if (valid) {
		GLCall(glDeleteBuffers(1, &vertexBuffer_id));
		valid = false;
	}
}

void VertexBuffer::Bind() const {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_id));
}

void VertexBuffer::Unbind() const {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

VertexBuffer::~VertexBuffer() {
	destroy();
}