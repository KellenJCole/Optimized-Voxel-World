#pragma once
#include "h/Rendering/Buffering/TextureArray.h"

class Shader;
class UserInterface {
public:
	UserInterface();
	bool initialize();
	void render(Shader& s);
private:
	unsigned int va, vb, eb;
	TextureArray textureArray;
	static const float crosshairVertices[4][6];
	static const unsigned int crosshairIndices[6];
};