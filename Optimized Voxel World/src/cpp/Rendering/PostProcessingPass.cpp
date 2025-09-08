#include "h/Rendering/PostProcessingPass.h"

PostProcessingPass::PostProcessingPass() 
	: fbo_(0)
	, color_(0)
	, rbo_(0)
	, vao_(0)
	, vbo_(0)
	, use_(true)
{

}

bool PostProcessingPass::init(int w, int h, const char* shaderPath) {
	shader_ = Shader(shaderPath);

    // FBO + color
    GLCall(glGenFramebuffers(1, &fbo_));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));

    GLCall(glGenTextures(1, &color_));
    GLCall(glBindTexture(GL_TEXTURE_2D, color_));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0));

    GLCall(glGenRenderbuffers(1, &rbo_));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rbo_));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h));
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    float quad[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    GLCall(glGenVertexArrays(1, &vao_));
    GLCall(glGenBuffers(1, &vbo_));
    GLCall(glBindVertexArray(vao_));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW));
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))));
    GLCall(glBindVertexArray(0));

    return true;
}

void PostProcessingPass::resize(int w, int h) {
    if (!color_ || !rbo_) return;

    GLCall(glBindTexture(GL_TEXTURE_2D, color_));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rbo_));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h));
}

void PostProcessingPass::beginScene() {
    if (!use_) return;
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void PostProcessingPass::blitToScreen() {
    if (!use_) return;
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    shader_.use();

    GLCall(glActiveTexture(GL_TEXTURE0));
    GLCall(glBindTexture(GL_TEXTURE_2D, color_));

    shader_.setUniform1i("screenTexture", 0);

    GLCall(glBindVertexArray(vao_));
    GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
    GLCall(glBindVertexArray(0));
}

void PostProcessingPass::destroy() {
    if (vbo_) GLCall(glDeleteBuffers(1, &vbo_));
    if (vao_) GLCall(glDeleteVertexArrays(1, &vao_));
    if (rbo_) GLCall(glDeleteRenderbuffers(1, &rbo_));
    if (color_) GLCall(glDeleteTextures(1, &color_));
    if (fbo_) GLCall(glDeleteFramebuffers(1, &fbo_));
    vbo_ = vao_ = rbo_ = color_ = fbo_ = 0;
    shader_.deleteProgram();
}