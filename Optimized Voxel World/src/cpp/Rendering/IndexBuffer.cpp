#include "h/Rendering/IndexBuffer.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

IndexBuffer::IndexBuffer() {
	valid = false;
}

void IndexBuffer::create(const unsigned int* data, unsigned int count) {
	GLCall(glGenBuffers(1, &indexBuffer_id));
	Bind();
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_DYNAMIC_DRAW));
	Unbind();
	valid = true;
	m_count = count; // Set the count here
}

void IndexBuffer::update(const unsigned int* data, unsigned int count) {
	Bind();
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_DYNAMIC_DRAW));
	m_count = count; // Update the count
}

void IndexBuffer::Bind() const {
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_id));
}
void IndexBuffer::Unbind() const {
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void IndexBuffer::destroy() {
	if (valid) {
		GLCall(glDeleteBuffers(1, &indexBuffer_id));
		valid = false;
	}
}

void IndexBuffer::clear() {
	Bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
	destroy();
}

bool IndexBuffer::isValid() {
	return valid;
}

unsigned int IndexBuffer::GetCount() {
	return m_count;
}