#include "h/Rendering/TextureLibrary.h"

TextureLibrary::TextureLibrary() {

}

Texture* TextureLibrary::getTexture(char name) {
	if (texMap.find(name) != texMap.end())
		return texMap[name];
	else
		return texMap[0];
}

void TextureLibrary::setTextures() {
	texMap[0] = new Texture("missing.png", false);
	texMap[1] = new Texture("dirt.png", false);
	texMap[2] = new Texture("stone.png", false);
	texMap[3] = new Texture("bedrock.png", false);
}