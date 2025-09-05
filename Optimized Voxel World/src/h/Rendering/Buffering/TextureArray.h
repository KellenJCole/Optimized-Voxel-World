#pragma once

#include <string>
#include <iostream>
#include <vector>

class TextureArray {
private:
	unsigned int TextureID;
public:
	TextureArray();
	TextureArray(std::vector<std::string> imageNames, bool flipVertically);
	void Bind();
	~TextureArray();
	inline unsigned int getTextureID() const { return TextureID; }
};

