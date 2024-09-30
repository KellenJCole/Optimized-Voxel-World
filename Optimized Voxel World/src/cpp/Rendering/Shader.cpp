#include "h/Rendering/Shader.h"
#include "h/Rendering/GLErrorCatcher.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader() {

}

Shader::Shader(const char* filepath) {
	std::ifstream stream;
	std::string vertexCode, fragmentCode;

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	stream.open(filepath);
	if (!stream) {
		std::cout << "Failed to open shader file: " << filepath << std::endl;
		return;
	}
	std::stringstream ss[2];
	std::string line;

	ShaderType type = ShaderType::NONE;
	while (getline(stream, line)) {
		if (line.find("#shader") != std::string::npos) {
			if (line.find("vertex") != std::string::npos) {
				type = ShaderType::VERTEX;
			}
			else if (line.find("fragment") != std::string::npos) {
				type = ShaderType::FRAGMENT;
			}
		}
		else {
			if (type == ShaderType::VERTEX) {
				ss[0] << line << "\n";
			}
			else if (type == ShaderType::FRAGMENT) {
				ss[1] << line << "\n";
			}
		}
	}

	if (stream.bad()) {
		std::cerr << "An error occurred during file reading that wasn't EOF." << std::endl;
	}

	stream.close();

	vertexCode = ss[0].str();
	fragmentCode = ss[1].str();

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();


	// Now compile shaders
	unsigned int vertex, fragment;
	int success;
	char infoLog[512];
	//vertex
	GLCall(vertex = glCreateShader(GL_VERTEX_SHADER));
	GLCall(glShaderSource(vertex, 1, &vShaderCode, NULL));
	GLCall(glCompileShader(vertex));
	GLCall(glGetShaderiv(vertex, GL_COMPILE_STATUS, &success));
	if (!success) {
		GLCall(glGetShaderInfoLog(vertex, 512, NULL, infoLog));
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << "\n";
	}

	//fragment
	GLCall(fragment = glCreateShader(GL_FRAGMENT_SHADER));
	GLCall(glShaderSource(fragment, 1, &fShaderCode, NULL));
	GLCall(glCompileShader(fragment));
	GLCall(glGetShaderiv(fragment, GL_COMPILE_STATUS, &success));
	if (!success) {
		GLCall(glGetShaderInfoLog(fragment, 512, NULL, infoLog));
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << "\n";
	}

	// shader Program
	GLCall(id = glCreateProgram());
	GLCall(glAttachShader(id, vertex));
	GLCall(glAttachShader(id, fragment));
	GLCall(glLinkProgram(id));

	GLCall(glGetProgramiv(id, GL_LINK_STATUS, &success));
	if (!success) {
		GLCall(glGetProgramInfoLog(id, 512, NULL, infoLog));
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << "\n";
	}

	GLCall(glDeleteShader(vertex));
	GLCall(glDeleteShader(fragment));
}

void Shader::use() {
	GLCall(glUseProgram(id));
}

void Shader::setBool(const std::string& name, bool value) const {
	GLCall(glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value));
}

void Shader::setInt(const std::string& name, int value) const {
	GLCall(glUniform1i(glGetUniformLocation(id, name.c_str()), value));
}

void Shader::setFloat(const std::string& name, float value) const {
	GLCall(glUniform1f(glGetUniformLocation(id, name.c_str()), value));
}

void Shader::setUniform1i(const std::string& name, int value) const {
	GLCall(glUniform1i(glGetUniformLocation(id, name.c_str()), value));
}

void Shader::setUniform4fv(const std::string& name, glm::mat4& transform) const {
	GLCall(glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(transform)));
}

void Shader::setVec3(const std::string& name, glm::vec3& value) const {
	GLCall(glUniform3f(glGetUniformLocation(id, name.c_str()), value.x, value.y, value.z));
}

void Shader::deleteProgram() {
	GLCall(glDeleteProgram(id));
}