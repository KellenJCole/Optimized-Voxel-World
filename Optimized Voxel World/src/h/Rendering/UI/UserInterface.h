#pragma once
#include "h/Rendering/Texture.h"

class Shader;
class UserInterface {
public:
	UserInterface();
	bool initialize();
	void render(Shader& s);
private:
	unsigned int va, vb, eb;
	Texture textureArray;
	static const float crosshairVertices[4][6];
	static const unsigned int crosshairIndices[6];
};