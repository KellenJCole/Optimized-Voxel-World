#include "h/Rendering/Renderer.h"
#include "h/Rendering/VertexBufferLayout.h"
#include "h/Rendering/GLErrorCatcher.h"

Renderer::Renderer() {
 
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize() {
    vertexArray.create();

    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    gl_fill = true;
    GLCall(glEnable(GL_FRAMEBUFFER_SRGB)); // Enable sRGB for gamma correction

    setupVertexAttributes();

    glfwWindowHint(GLFW_SAMPLES, 16);
    glEnable(GL_MULTISAMPLE);

    return true;
}

void Renderer::setupVertexAttributes() {
    layout.PushFloat(3); // Position
    layout.PushFloat(2); // TexCoords
    layout.PushInt(1); // Normal + blockType * 16
}

void Renderer::toggleFillLine() {
    gl_fill = !gl_fill;
    if (gl_fill) {
        GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    }
    else {
        GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    }
}


void Renderer::render() {
    vertexArray.Bind();
    for (auto& coords : chunksBeingRendered) {
        if (chunkToVertexBuffer.find(coords) != chunkToVertexBuffer.end() && chunkToIndexBuffer.find(coords) != chunkToIndexBuffer.end()) {
            chunkToVertexBuffer[coords].Bind();
            chunkToIndexBuffer[coords].Bind();
            vertexArray.AddBuffer(chunkToVertexBuffer[coords], layout);
            GLCall(glDrawElements(GL_TRIANGLES, chunkToIndexBuffer[coords].GetCount(), GL_UNSIGNED_INT, nullptr));
            chunkToVertexBuffer[coords].Unbind();
            chunkToIndexBuffer[coords].Unbind();
        }
    }
    vertexArray.Unbind();
}

void Renderer::updateRenderChunks(std::vector<std::pair<int, int>>& renderChunks) {
    chunksBeingRendered = renderChunks;
}

void Renderer::cleanup() {
    for (auto& pair : chunkToVertexBuffer) {
        pair.second.destroy();
    }
    for (auto& pair : chunkToIndexBuffer) {
        pair.second.destroy();
    }
    chunkToVertexBuffer.clear();
    chunkToIndexBuffer.clear();
    vertexArray.destroy();
}

void Renderer::updateVertexBuffer(const std::vector<Vertex>& vertices, std::pair<int, int> coords) {
    auto it = chunkToVertexBuffer.find(coords);
    if (it == chunkToVertexBuffer.end()) {
        chunkToVertexBuffer[coords].create(vertices.data(), vertices.size() * sizeof(Vertex));
    }
    else {
        it->second.update(vertices.data(), vertices.size() * sizeof(Vertex));
    }
}

void Renderer::updateIndexBuffer(const std::vector<unsigned int>& indices, std::pair<int, int> coords) {
    bool bufferExists = chunkToIndexBuffer.find(coords) != chunkToIndexBuffer.end();

    if (!bufferExists) {
        chunkToIndexBuffer[coords].create(indices.data(), indices.size());
    }
    else {
        chunkToIndexBuffer[coords].update(indices.data(), indices.size());
    }
}

void Renderer::eraseBuffers(const std::pair<int, int>& eraseKey) {
    if (chunkToVertexBuffer.find(eraseKey) != chunkToVertexBuffer.end()) {
        chunkToVertexBuffer[eraseKey].destroy();
        chunkToVertexBuffer.erase(eraseKey);
    }
    if (chunkToIndexBuffer.find(eraseKey) != chunkToIndexBuffer.end()) {
        chunkToIndexBuffer[eraseKey].destroy();
        chunkToIndexBuffer.erase(eraseKey);
    }
}