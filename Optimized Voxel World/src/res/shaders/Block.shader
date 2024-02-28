#shader vertex
#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in int aNormal; // Add normal vector input

uniform vec3 translationModel;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec4 FragPos; // Pass fragment position to fragment shader
out vec3 Normal; // Pass normal to fragment shader
out float BlockType;

void main()
{
    vec4 translatedPosition = vec4(aPosition + translationModel, 1.0);
    TexCoord = aTexCoord;
    FragPos = translatedPosition;

    if (aNormal % 16 == 1) {
        Normal = vec3(-(aNormal % 16), 0, 0);
    }
    else if (aNormal % 16 == 2) {
        Normal = vec3(aNormal % 16, 0, 0);
    }
    else if (aNormal % 16 == 3) {
        Normal = vec3(0, -(aNormal >> 1), 0);
    }
    else if (aNormal % 16 == 4) {
        Normal = vec3(0, aNormal >> 1, 0);
    }
    else if (aNormal % 16 == 5) {
        Normal = vec3(0, 0, -1);
    }
    else if (aNormal % 16 == 6) {
        Normal = vec3(0, 0, 1);
    }

    BlockType = aNormal / 16;
    gl_Position = projection * view * translatedPosition;
}

#shader fragment
#version 450 core
out vec4 FragColor;
in vec2 TexCoord;
in vec4 FragPos; // Receive fragment position from vertex shaders
in vec3 Normal; // Receive normal from vertex shader
in float BlockType;

uniform sampler2DArray textureArray;
uniform vec3 lightPos; // Position of the light source
uniform vec3 lightColor; // Color of the light source
uniform vec3 viewPos; // Position of the camera/view

void main() {
    // Ambient lighting
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - vec3(FragPos));
    float diff = max(dot(norm, lightDir), 0.0);
    float diffuseStrength = 0.4;
    vec3 diffuse = diff * lightColor * diffuseStrength;

    // Combine results
    vec3 tex = texture(textureArray, vec3(TexCoord, BlockType - 1)).rgb;
    vec3 result = (ambient + diffuse) * tex;
    FragColor = vec4(result, 1.0); // Ensure the alpha value is set to 1 for full opacity
}