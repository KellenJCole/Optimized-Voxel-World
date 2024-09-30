#shader vertex
#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
	TexCoords = aTexCoords;
	gl_Position = vec4(aPos, 0.0, 1.0);
}


#shader fragment
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
const vec3 skyColor = vec3(0.529, 0.808, 0.922);

void main() {
    vec3 current = texture(screenTexture, TexCoords).rgb;

    // Check if the current pixel color is the same as the sky color
    if (distance(current, skyColor) < 0.01) {  // Use a small tolerance for floating-point precision
        vec3 sum = vec3(0.0);
        int count = 0;

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                vec2 samplePos = TexCoords + vec2(x, y) * (1.0 / 800.0);  // Adjust offset as needed
                vec3 neighbor = texture(screenTexture, samplePos).rgb;

                // Consider non-sky pixels
                if (distance(neighbor, skyColor) >= 0.01) {
                    sum += neighbor;
                    count++;
                }
            }
        }

        // If neighboring pixels exist, average their colors
        if (count > 0) {
            FragColor = vec4(sum / float(count), 1.0);
        }
        else {
            FragColor = vec4(skyColor, 1.0);  // Keep sky color if no neighbors found
        }
    }
    else {
        // Keep the current pixel color if it's not the sky color
        FragColor = vec4(current, 1.0);
    }
}
