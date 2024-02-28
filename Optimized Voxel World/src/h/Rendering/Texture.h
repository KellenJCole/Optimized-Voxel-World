#pragma once
#include <string>
#include <iostream>
#include <vector>

class Texture {
private:
	unsigned int TextureID;
public:
	Texture();
	Texture(std::vector<std::string> imageNames, bool flipVertically);
	void Bind();
	~Texture();
	inline unsigned int getTextureID() const { return TextureID; }
};

