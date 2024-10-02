#include "h/Rendering/TextureArray.h"
#include "h/Rendering/GLErrorCatcher.h"
#define STB_IMAGE_IMPLEMENTATION
#include <h/stb_image/stb_image.h>


TextureArray::TextureArray() {

}

TextureArray::TextureArray(std::vector<std::string> imageNames, bool flipVertically) {
    if (flipVertically)
        stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    // Assuming all textures have the same dimensions and format for simplicity
    unsigned char* firstImage = stbi_load(("src/res/textures/" + imageNames[0]).c_str(), &width, &height, &nrChannels, 0);
    if (!firstImage) {
        std::cout << "Failed to load texture" << std::endl;
        return;
    }
    GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB; // Simplified format determination
    std::cout << nrChannels << "\n";

    GLCall(glGenTextures(1, &TextureID));
    GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, TextureID));

    GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    GLCall(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 6, GL_SRGB8_ALPHA8, width, height, imageNames.size()));

    // Upload the first texture
    GLCall(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, format, GL_UNSIGNED_BYTE, firstImage));
    stbi_image_free(firstImage);

    // Upload the rest of the textures
    for (size_t i = 1; i < imageNames.size(); i++) {
        unsigned char* data = stbi_load(("src/res/textures/" + imageNames[i]).c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLCall(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, format, GL_UNSIGNED_BYTE, data));
            stbi_image_free(data);
        }
        else {
            std::cout << "Failed to load texture at index " << i << std::endl;
        }
    }

    GLCall(glGenerateMipmap(GL_TEXTURE_2D_ARRAY));
}

void TextureArray::Bind() {
    GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, getTextureID()));
}

TextureArray::~TextureArray() {

}
