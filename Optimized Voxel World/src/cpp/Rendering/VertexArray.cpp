#pragma once
#include "h/Rendering/VertexArray.h"
#include "h/Rendering/VertexBuffer.h"
#include "h/Rendering/VertexBufferLayout.h"
#include "h/Rendering/GLErrorCatcher.h"

VertexArray::VertexArray() {

}

void VertexArray::create() {
	GLCall(glGenVertexArrays(1, &vertexArray_id));
}

void VertexArray::destroy() {
	GLCall(glDeleteBuffers(1, &vertexArray_id));
}

VertexArray::~VertexArray() {
	GLCall(glDeleteVertexArrays(1, &vertexArray_id))
}

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) {
	Bind();
	vb.Bind();
	const auto& elements = layout.GetElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++) {
		const auto& element = elements[i];
		GLCall(glEnableVertexAttribArray(i));
		GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));
		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
	}
}

void VertexArray::AddBuffer(const unsigned int vb, const VertexBufferLayout& layout) {
	Bind();
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vb));
	const auto& elements = layout.GetElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++) {
		const auto& element = elements[i];
		GLCall(glEnableVertexAttribArray(i));
		GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));
		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);

		if (element.count == 4 && element.type == GL_FLOAT) {
			GLCall(glVertexAttribDivisor(i, 1)); // This is an instanced attribute
		}
	}
}

void VertexArray::Bind() const {
	GLCall(glBindVertexArray(vertexArray_id));
}

void VertexArray::Unbind() const {
	GLCall(glBindVertexArray(0));
}