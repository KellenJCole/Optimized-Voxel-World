#shader vertex
#version 460 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 iCenter;
layout(location=2) in vec3 iHalf;
layout(location=3) in vec3 iColor;

uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main() {
	vec3 world = iCenter + aPos * iHalf;
	vColor = iColor;
	gl_Position = projection * view * vec4(world, 1.0);
}

#shader fragment
#version 460 core

in vec3 vColor;
out vec4 FragColor;
void main() {
	FragColor = vec4(vColor, 1.0);
}