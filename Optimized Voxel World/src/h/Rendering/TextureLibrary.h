#pragma once
#include "h/Rendering/Texture.h"
#include <string>
#include <map>

class TextureLibrary {
public:
	TextureLibrary();
	void setTextures();
	Texture* getTexture(char id);
private:
	std::map<char, Texture*> texMap;
};

