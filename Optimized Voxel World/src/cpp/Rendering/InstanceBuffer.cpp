#include "h/Rendering/InstanceBuffer.h"
#include "h/Rendering/GLErrorCatcher.h"
#include <h/glm/glm.hpp>
#include <iostream>

InstanceBuffer::InstanceBuffer() {

}

void InstanceBuffer::create(unsigned int size) {
	GLCall(glGenBuffers(1, &instanceBuffer_id));
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * size, nullptr, GL_DYNAMIC_DRAW));

	GLCall(glEnableVertexAttribArray(3)); // Assuming 3 is the starting index for instanced attributes
	GLCall(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0));
	GLCall(glVertexAttribDivisor(3, 1));
	Unbind();
}

void InstanceBuffer::update(const void* data, unsigned int size) {
	Bind();
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, size, data));
	Unbind();
}

void InstanceBuffer::Bind() const {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_id));
}
void InstanceBuffer::Unbind() const {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

InstanceBuffer::~InstanceBuffer() {
	GLCall(glDeleteBuffers(1, &instanceBuffer_id));
}