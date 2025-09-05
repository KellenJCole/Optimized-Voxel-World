#pragma once

#include "h/Rendering/Buffering/VertexArray.h"
#include "h/Rendering/Buffering/VertexBuffer.h"
#include "h/Rendering/Buffering/IndexBuffer.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/BlockGeometry.h"
#include "h/Rendering/VertexPool.h"
#include <h/external/glm/glm.hpp>

#include <vector>
#include <map>
#include <mutex>

class Renderer {
public:
    Renderer();
    ~Renderer();
    bool initialize();
    void render();
    void updateRenderChunks(std::vector<std::pair<int, int>>& renderChunks);
    void cleanup();
    void toggleFillLine();
    void setWindowPointer(GLFWwindow* w);
	void setVertexPoolPointer(VertexPool* vp);
private:
    GLFWwindow* window;
    int width, height;
    VertexArray vertexArray;
	VertexPool* vertexPool;
    std::mutex renderMtx;
    std::vector<std::pair<int, int>> chunksBeingRendered;

    void setupVertexAttributes();

    bool gl_fill;
};
