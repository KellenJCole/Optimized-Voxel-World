#include "h/Rendering/UI/DebugUI.h"

#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

DebugUI::DebugUI() {}

// Class from https://learnopengl.com/In-Practice/Text-Rendering

bool DebugUI::initialize() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, "src/res/fonts/Roboto-Regular.ttf", 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 32);  // Calculate width of each glyph based off height

    GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    for (unsigned char c = 0; c < 128; c++) {
        // load the character's glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "FreeType failure to load glyph\n";
            continue;
        }

        // gen texture
        unsigned int texture;  // id
        GLCall(glGenTextures(1, &texture));
        GLCall(glBindTexture(GL_TEXTURE_2D, texture));
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED,
                            GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer));

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {texture, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                               glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), face->glyph->advance.x};
        characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void DebugUI::renderText(Shader& s, std::string text, float x, float y, float scale, glm::vec3 color) {
    float copyX = x;
    s.use();
    glUniform3f(glGetUniformLocation(s.id, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // iterate through the characters

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = characters[*c];

        if (*c == '\n') {
            y -= ch.size.y * 1.3 * scale;
            x = copyX;
        } else {
            float xpos = x + ch.bearing.x * scale;
            float ypos = y - (ch.size.y - ch.bearing.y) * scale;

            float w = ch.size.x * scale;
            float h = ch.size.y * scale;

            float vertices[6][4] = {
                {xpos, ypos + h, 0.0f, 0.0f}, {xpos, ypos, 0.0f, 1.0f},     {xpos + w, ypos, 1.0f, 1.0f},

                {xpos, ypos + h, 0.0f, 0.0f}, {xpos + w, ypos, 1.0f, 1.0f}, {xpos + w, ypos + h, 1.0f, 0.0f}};

            // render glyph over quad defined above
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            // update vbo
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // advance to next glyph (advance is number of 1/64 pixels)
            x += (ch.advance >> 6) * scale;  // bitshift by 6 to get value in pixels. divided by 64
        }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
