#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <h/external/glm/glm.hpp> 
#include <h/external/glm/gtc/type_ptr.hpp> 

#include <string>

class Shader {
public:
	unsigned int id;

	Shader();
	Shader(const char* filepath);

	void use();
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec3(const std::string& name, glm::vec3& value) const;
	void setUniform1i(const std::string& name, int value) const;
	void setUniform4fv(const std::string& name, glm::mat4& transform) const;
	void deleteProgram();
};

#endif

