#pragma once
#include <ft2build.h>
#include "h/glm/vec2.hpp"
#include "h/glm/common.hpp"
#include "h/glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <map>
#include FT_FREETYPE_H

struct Character {
	unsigned int TextureID; // per glpyh
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance; // offset to advance to next glyph
};

class Shader;
class DebugUI {
public:
	DebugUI();
	bool initialize();
	void renderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color);
private:
	unsigned int vao, vbo;
	std::map<char, Character> characters;
};