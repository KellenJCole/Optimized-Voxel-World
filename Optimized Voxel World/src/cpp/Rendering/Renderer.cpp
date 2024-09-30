#include "h/Rendering/Renderer.h"
#include "h/Rendering/GLErrorCatcher.h"

Renderer::Renderer() : width(1600), height(900) {
 
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
    GLCall(glEnable(GL_FRAMEBUFFER_SRGB));

    setupVertexAttributes();

    glfwWindowHint(GLFW_SAMPLES, 4);
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
    std::lock_guard<std::mutex> lock(renderMtx);

    vertexArray.Bind();

    for (auto& coords : chunksBeingRendered) {
        if (chunkToVertexBuffer.find(coords) != chunkToVertexBuffer.end() && chunkToIndexBuffer.find(coords) != chunkToIndexBuffer.end()) {
            chunkToVertexBuffer[coords].Bind();
            chunkToIndexBuffer[coords].Bind();
            vertexArray.AddBuffer(chunkToVertexBuffer[coords], layout);
            GLCall(glDrawElements(GL_TRIANGLES, chunkToIndexBuffer[coords].GetCount(), GL_UNSIGNED_INT, nullptr));
        }
    }
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
    if (vertices.empty()) {
        std::cerr << "Error: Vertex data is empty for chunk: (" << coords.first << ", " << coords.second << ")\n";
        return;
    }

    const void* data = vertices.data();
    size_t dataSize = vertices.size() * sizeof(Vertex);

    std::lock_guard<std::mutex> lock(renderMtx);
    auto it = chunkToVertexBuffer.find(coords);
    if (it == chunkToVertexBuffer.end()) {
        chunkToVertexBuffer[coords].create(data, dataSize);  // Create the buffer if it doesn't exist
        vertexArray.AddBuffer(chunkToVertexBuffer[coords], layout);
    }
    else {
        it->second.update(data, dataSize);  // Update the buffer if it exists
    }
}

void Renderer::updateIndexBuffer(const std::vector<unsigned int>& indices, std::pair<int, int> coords) {
    if (indices.empty()) {
        std::cerr << "Error: Index data is empty for chunk: (" << coords.first << ", " << coords.second << ")\n";
        return;
    }

    const unsigned int* data = indices.data();
    size_t dataSize = indices.size();

    std::lock_guard<std::mutex> lock(renderMtx);
    bool bufferExists = chunkToIndexBuffer.find(coords) != chunkToIndexBuffer.end();

    if (!bufferExists) {
        chunkToIndexBuffer[coords].create(data, dataSize);  // Create the buffer if it doesn't exist
        vertexArray.Bind();
        chunkToIndexBuffer[coords].Bind();
    }
    else {
        chunkToIndexBuffer[coords].update(data, dataSize);  // Update the buffer if it exists
    }
}

void Renderer::eraseBuffers(const std::pair<int, int>& eraseKey) {
    std::lock_guard<std::mutex> lock(renderMtx);
    if (chunkToVertexBuffer.find(eraseKey) != chunkToVertexBuffer.end()) {
        chunkToVertexBuffer[eraseKey].destroy();
        chunkToVertexBuffer.erase(eraseKey);
    }
    if (chunkToIndexBuffer.find(eraseKey) != chunkToIndexBuffer.end()) {
        chunkToIndexBuffer[eraseKey].destroy();
        chunkToIndexBuffer.erase(eraseKey);
    }
}

void Renderer::setWindowPointer(GLFWwindow* w) {
    window = w;
    glfwGetWindowSize(window, &width, &height);
}