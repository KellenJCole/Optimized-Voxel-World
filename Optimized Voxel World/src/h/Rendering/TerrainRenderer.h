#pragma once

#include "h/Rendering/Buffering/VertexArray.h"
#include "h/Rendering/Buffering/VertexBuffer.h"
#include "h/Rendering/Buffering/IndexBuffer.h"
#include "h/Rendering/Buffering/TextureArray.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/BlockGeometry.h"
#include "h/Rendering/VertexPool.h"
#include <h/external/glm/glm.hpp>

#include <vector>
#include <map>
#include <mutex>

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    bool initialize();
    void render();
    void updateRenderChunks(std::vector<std::pair<int, int>>& renderChunks);
    void cleanup();
    void toggleFillLine();
    void setWindowPointer(GLFWwindow* w);
	void setVertexPoolPointer(VertexPool* vp);
    void updateShaderUniforms(glm::mat4& view, glm::mat4& projection, glm::vec3& viewPos);
private:
    void setupVertexAttributes();

    int width, height;
    bool gl_fill;

    glm::vec3 lightPos;
    glm::vec3 lightColor;

    GLFWwindow* window;
	VertexPool* vertexPool;
    Shader terrainShader;

    VertexArray vertexArray;
    TextureArray blockTextureArray;

    std::mutex renderMtx;

    std::vector<std::pair<int, int>> chunksBeingRendered;
};
