#include "h/Rendering/Texture.h"
#include "h/Rendering/GLErrorCatcher.h"
#define STB_IMAGE_IMPLEMENTATION
#include <h/stb_image/stb_image.h>

Texture::Texture() {

}

Texture::Texture(std::string imageName, bool flipVertically) {
    GLCall(glGenTextures(1, &TextureID));
    GLCall(glBindTexture(GL_TEXTURE_2D, TextureID));

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    if (flipVertically)
        stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(("src/res/textures/" + imageName).c_str(), &width, &height, &nrChannels, 0);

    GLenum format;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    GLenum internalFormat = format == GL_RGB ? GL_SRGB : GL_SRGB_ALPHA;
    if (data) {
        if (imageName.find(".jpg") != std::string::npos) {
            GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
        }
        else if (imageName.find(".png") != std::string::npos) {
            GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data));
        }
        else {
            std::cout << "Texture.h: Unrecognized image type.\n";
        }

        GLCall(glGenerateMipmap(GL_TEXTURE_2D));
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(data);
}

void Texture::Bind() {
    GLCall(glBindTexture(GL_TEXTURE_2D, getTextureID()));
}

Texture::~Texture() {

}
