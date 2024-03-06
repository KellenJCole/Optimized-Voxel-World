#shader vertex
#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in float textureID;

out vec2 TexCoord;
out float TextureID;

uniform mat4 projection;

void main()
{
    TexCoord = aTexCoord;

    gl_Position = projection * vec4(aPosition, 1.0);
    TextureID = textureID;
}

#shader fragment
#version 450 core
out vec4 FragColor;
in vec2 TexCoord;
in float TextureID;

uniform sampler2DArray textureArray;

void main() {

    // Combine results
    vec4 texColor = texture(textureArray, vec3(TexCoord, TextureID));
    FragColor = texColor;
}