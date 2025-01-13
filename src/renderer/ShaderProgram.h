
#include<glad/glad.h>
#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<cerrno>

class ShaderProgram {
public:
	ShaderProgram(const char* vertexPath, const char* fragmentPath);
	~ShaderProgram(){ glDeleteProgram(programID);}
	void Activate() { glUseProgram(programID); }
private:
	GLuint programID;
	std::string get_file_contents(const char* filename);
};