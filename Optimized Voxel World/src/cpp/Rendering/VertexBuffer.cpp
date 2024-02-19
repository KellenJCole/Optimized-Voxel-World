#include "h/Rendering/VertexBuffer.h"
#include "h/Rendering/GLErrorCatcher.h"
#include <iostream>

VertexBuffer::VertexBuffer() {

}

void VertexBuffer::create(const void* data, unsigned int size) {
	GLCall(glGenBuffers(1, &vertexBuffer_id));
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
	Unbind();
}

void VertexBuffer::update(const void* data, unsigned int size) {
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

void VertexBuffer::destroy() {
	GLCall(glDeleteBuffers(1, &vertexBuffer_id));
}

void VertexBuffer::Bind() const {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_id));
}

void VertexBuffer::Unbind() const {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

VertexBuffer::~VertexBuffer() {
	GLCall(glDeleteBuffers(1, &vertexBuffer_id));
}