#include "h/Rendering/IndexBuffer.h"
#include "h/Rendering/GLErrorCatcher.h"

IndexBuffer::IndexBuffer() {

}

void IndexBuffer::create(const unsigned int* data, unsigned int count) {
	GLCall(glGenBuffers(1, &indexBuffer_id));
	Bind();
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
	Unbind();
}

void IndexBuffer::Bind() const {
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_id));
}
void IndexBuffer::Unbind() const {
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void IndexBuffer::destroy() {
	GLCall(glDeleteBuffers(1, &indexBuffer_id));
}

IndexBuffer::~IndexBuffer() {
	GLCall(glDeleteBuffers(1, &indexBuffer_id));
}