#pragma once
#include <glad/glad.h>
#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

class PostProcessingPass {
public:
	PostProcessingPass();

	bool init(int w, int h, const char* shaderPath);

	void resize(int w, int h);

	void beginScene();

	void blitToScreen();

	void setEnabled(bool on) { use_ = on; }
	bool enabled() const { return use_; }

	void destroy();

private:
	Shader shader_;
	GLuint fbo_, color_, rbo_;
	GLuint vao_, vbo_;
	bool use_;
};
