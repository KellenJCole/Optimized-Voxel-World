#include "h/Rendering/Renderer.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"
#include "h/Rendering/Utility/WindowConfig.h"

Renderer::Renderer() : width(WindowDetails::WindowWidth), height(WindowDetails::WindowHeight), vertexPool(nullptr) {
 
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


    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);
    
    return true;
}

void Renderer::setupVertexAttributes() {
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
	if (!vertexPool) return;

    std::lock_guard<std::mutex> lock(renderMtx);
    vertexArray.Bind(); // ensure VAO is bound
    vertexPool->buildIndirectCommands(chunksBeingRendered);
    vertexPool->renderIndirect();
}

void Renderer::updateRenderChunks(std::vector<std::pair<int, int>>& renderChunks) {
    chunksBeingRendered = renderChunks;
}

void Renderer::cleanup() {
    vertexArray.destroy();
}

void Renderer::setWindowPointer(GLFWwindow* w) {
    window = w;
    glfwGetWindowSize(window, &width, &height);
}

void Renderer::setVertexPoolPointer(VertexPool* vp) {
	vertexPool = vp;
    setupVertexAttributes();
}