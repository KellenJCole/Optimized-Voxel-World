#include "h/Rendering/TerrainRenderer.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"
#include "h/Rendering/Utility/WindowConfig.h"

TerrainRenderer::TerrainRenderer() 
    : width(WindowDetails::WindowWidth)
    , height(WindowDetails::WindowHeight)
    , gl_fill(true)
    , window(nullptr)
    , vertexPool(nullptr)
    , lightPos(0.0f, 500.f, 0.0f)
    , lightColor(0.9f, 1.f, 0.7f)
{
 
}

bool TerrainRenderer::initialize() {
    vertexArray.create();

    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    gl_fill = true;
    GLCall(glEnable(GL_FRAMEBUFFER_SRGB));


    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    terrainShader = Shader("src/res/shaders/Block.shader");
    blockTextureArray = TextureArray({ "blocks/dirt.png", "blocks/grass_top.png", "blocks/grass_side.png", "blocks/stone.png", "blocks/bedrock.png", "blocks/sand.png", "blocks/water.png" }, false);
    
    return true;
}

void TerrainRenderer::setupVertexAttributes() {
    vertexArray.Bind();
    if (!vertexPool) return;  // guard
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexPool->getVBO()));

    // Attribute 0: vec3 position
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)));

    // Attribute 1: vec2 texCoords
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords)));

    // Attribute 2: int normalBlockType
    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, normalBlockType)));

    glBindBuffer(GL_ARRAY_BUFFER, vertexPool->getVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexPool->getEBO());        // expose a getEBO() accessor
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, vertexPool->getIndirectBuf());

    GLCall(glBindVertexArray(0));
}

void TerrainRenderer::toggleFillLine() {
    gl_fill = !gl_fill;
    if (gl_fill) {
        GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    }
    else {
        GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    }
}

void TerrainRenderer::updateShaderUniforms(glm::mat4& view, glm::mat4& projection, glm::vec3& viewPos) {
    terrainShader.use();

    terrainShader.setUniform4fv("view", view);
    terrainShader.setUniform4fv("projection", projection);
    terrainShader.setVec3("viewPos",viewPos);
    terrainShader.setVec3("lightPos", lightPos);
    terrainShader.setVec3("lightColor", lightColor);
}


void TerrainRenderer::render() {
	if (!vertexPool) return;

    blockTextureArray.Bind();

    terrainShader.use();

    std::lock_guard<std::mutex> lock(renderMtx);
    vertexArray.Bind(); // ensure VAO is bound
    vertexPool->buildIndirectCommands(chunksBeingRendered);
    vertexPool->renderIndirect();
}

void TerrainRenderer::updateRenderChunks(std::vector<std::pair<int, int>>& renderChunks) {
    chunksBeingRendered = renderChunks;
}

void TerrainRenderer::setWindowPointer(GLFWwindow* w) {
    window = w;
    glfwGetWindowSize(window, &width, &height);
}

void TerrainRenderer::setVertexPoolPointer(VertexPool* vp) {
	vertexPool = vp;
    setupVertexAttributes();
}

void TerrainRenderer::cleanup() {
    vertexArray.destroy();
    terrainShader.deleteProgram();
    blockTextureArray.~TextureArray();
}

TerrainRenderer::~TerrainRenderer() {
    cleanup();
}