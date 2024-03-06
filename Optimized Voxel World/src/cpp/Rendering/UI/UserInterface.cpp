#pragma once
#include "h/Rendering/UI/UserInterface.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/GLErrorCatcher.h"

const float UserInterface::crosshairVertices[4][6] = {
    // positions            // texture coords     //texture id
    790.f,  460.f, 0.0f,     0.0f, 1.0f,          0,  // top left 
    810.f,  460.f, 0.0f,     1.0f, 1.0f,          0,  // top right
    810.f,  440.f, 0.0f,     1.0f, 0.0f,          0,  // bottom right
    790.f,  440.f, 0.0f,     0.0f, 0.0f,          0,  // bottom left
};

const unsigned int UserInterface::crosshairIndices[] = {
    0, 1, 2,
    2, 3, 0
};

UserInterface::UserInterface() :
    va(0),
    vb(0),
    eb(0)
{

}

bool UserInterface::initialize() {
    textureArray = TextureArray({ "User Interface/crosshair.png" }, false);
    GLCall(glGenVertexArrays(1, &va));
    GLCall(glBindVertexArray(va));

    // Generate and bind VBO
    GLCall(glGenBuffers(1, &vb));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vb));
    GLCall(glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), crosshairVertices, GL_STATIC_DRAW));

    // Generate and bind EBO
    GLCall(glGenBuffers(1, &eb));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(crosshairIndices), crosshairIndices, GL_STATIC_DRAW));

    // Specify the layout of the vertex data
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0));
    GLCall(glEnableVertexAttribArray(0)); // Position attribute
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float))));
    GLCall(glEnableVertexAttribArray(2));
    return true;
}


// Simply rendering a bunch of quads in 2D on screen
void UserInterface::render(Shader& s) {
    GLCall(glDisable(GL_DEPTH_TEST));
    GLCall(glDisable(GL_CULL_FACE));

    textureArray.Bind();
	s.use();
    GLCall(glBindVertexArray(va));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb));
    GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));

    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
}