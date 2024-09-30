#pragma once
#include "h/Rendering/VertexArray.h"
#include "h/Rendering/VertexBuffer.h"
#include "h/Rendering/IndexBuffer.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/VertexBufferLayout.h"
#include <vector>
#include <map>
#include <h/glm/glm.hpp>
#include <mutex>

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    int normalBlockType; // blockType = blockType * 16
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    bool initialize();
    void render();
    void updateRenderChunks(std::vector<std::pair<int, int>>& renderChunks);
    void cleanup();
    void toggleFillLine();
    void updateVertexBuffer(const std::vector<Vertex>& vertices, std::pair<int, int> coords);
    void updateIndexBuffer(const std::vector<unsigned int>& indices, std::pair<int, int> coords);
    void eraseBuffers(const std::pair<int, int>& eraseKey);
    void setWindowPointer(GLFWwindow* w);
private:
    GLFWwindow* window;
    int width, height;
    VertexArray vertexArray;
    VertexBufferLayout layout;
    std::map<std::pair<int, int>, VertexBuffer> chunkToVertexBuffer;
    std::map<std::pair<int, int>, IndexBuffer> chunkToIndexBuffer;
    std::mutex renderMtx;
    std::vector<std::pair<int, int>> chunksBeingRendered;

    void setupVertexAttributes();

    bool gl_fill;
};
