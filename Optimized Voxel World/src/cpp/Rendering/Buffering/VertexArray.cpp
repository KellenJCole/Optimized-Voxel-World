#pragma once
#include "h/Rendering/Buffering/VertexArray.h"

#include "h/Rendering/Buffering/VertexBuffer.h"
#include "h/Rendering/Buffering/VertexBufferLayout.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

VertexArray::VertexArray() {}

void VertexArray::create() { GLCall(glGenVertexArrays(1, &vertexArray_id)); }

void VertexArray::destroy() { GLCall(glDeleteVertexArrays(1, &vertexArray_id)); }

VertexArray::~VertexArray() { GLCall(glDeleteVertexArrays(1, &vertexArray_id)) }

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) {
    Bind();
    vb.Bind();
    const auto& elements = layout.GetElements();
    unsigned int offset = 0;
    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto& element = elements[i];
        GLCall(glEnableVertexAttribArray(i));

        if (element.type == GL_INT || element.type == GL_UNSIGNED_INT) {
            GLCall(glVertexAttribIPointer(i, element.count, element.type, layout.GetStride(), (const void*)offset));
        } else {
            GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(),
                                         (const void*)offset));
        }
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
        GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(),
                                     (const void*)offset));
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);

        if (element.count == 4 && element.type == GL_FLOAT) {
            GLCall(glVertexAttribDivisor(i, 1));  // This is an instanced attribute
        }
    }
}

void VertexArray::AddBufferAt(const VertexBuffer& vb, const VertexBufferLayout& layout, unsigned int baseAttrib) {
    Bind();
    vb.Bind();
    const auto& elements = layout.GetElements();
    unsigned int offset = 0;
    for (unsigned int i = 0; i < elements.size(); ++i) {
        const auto& e = elements[i];
        unsigned int loc = baseAttrib + i;
        GLCall(glEnableVertexAttribArray(loc));
        if (e.type == GL_INT || e.type == GL_UNSIGNED_INT) {
            GLCall(glVertexAttribIPointer(loc, e.count, e.type, layout.GetStride(), (const void*)offset));
        }
        else {
            GLCall(glVertexAttribPointer(loc, e.count, e.type, e.normalized, layout.GetStride(), (const void*)offset));
        }
        offset += e.count * VertexBufferElement::GetSizeOfType(e.type);
    }
}

void VertexArray::AddBufferInstancedAt(const VertexBuffer& vb, const VertexBufferLayout& layout,
    unsigned int baseAttrib, unsigned int divisor) {
    AddBufferAt(vb, layout, baseAttrib);
    const auto& elements = layout.GetElements();
    for (unsigned int i = 0; i < elements.size(); ++i) {
        GLCall(glVertexAttribDivisor(baseAttrib + i, divisor));
    }
}

void VertexArray::Bind() const { GLCall(glBindVertexArray(vertexArray_id)); }

void VertexArray::Unbind() const { GLCall(glBindVertexArray(0)); }
