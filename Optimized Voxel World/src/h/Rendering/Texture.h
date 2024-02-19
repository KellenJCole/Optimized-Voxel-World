#pragma once
#include <string>
#include <iostream>

class Texture {
private:
	unsigned int TextureID;
public:
	Texture();
	Texture(std::string imageName, bool flipVertically);
	void Bind();
	~Texture();
	inline unsigned int getTextureID() const { return TextureID; }
};

