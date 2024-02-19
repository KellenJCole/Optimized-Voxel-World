#shader vertex
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal; // Add normal vector input
layout(location = 3) in vec3 translationModel;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec4 FragPos; // Pass fragment position to fragment shader
out vec3 Normal; // Pass normal to fragment shader

void main()
{
    vec4 translatedPosition = vec4(aPosition + translationModel, 1.0);
    TexCoord = aTexCoord;
    FragPos = translatedPosition;
    Normal = aNormal;
    gl_Position = projection * view * translatedPosition;
}

#shader fragment
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in vec4 FragPos; // Receive fragment position from vertex shaders
in vec3 Normal; // Receive normal from vertex shader

uniform sampler2D texture1;
uniform vec3 lightPos; // Position of the light source
uniform vec3 lightColor; // Color of the light source
uniform vec3 viewPos; // Position of the camera/view

void main() {
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - vec3(FragPos));
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Combine results
    vec3 tex = texture(texture1, TexCoord).rgb;
    vec3 result = (ambient + diffuse) * tex;
    FragColor = vec4(result, 1.0); // Ensure the alpha value is set to 1 for full opacity
    //FragColor = vec4((normalize(Normal) + 1.0) * 0.5, 1.0); // Normal visualized as color
}