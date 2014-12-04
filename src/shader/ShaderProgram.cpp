/*
Project:		Efficient computation of Lighting
Type:			Bachelor's thesis
Author:			Tomáš Kubovčík, xkubov02@stud.fit.vutbr.cz
Supervisor:		Ing. Tomáš Milet
School info:	Brno Univeristy of Technology (VUT)
Faculty of Information Technology (FIT)
Department of Computer Graphics and Multimedia (UPGM)

Project information
---------------------
The goal of this project is to efficiently compute lighting in scenes
with hundrends to thousands light sources. To handle this there have been
implemented lighting techniques as deferred shading, tiled deferred shading
and tiled forward shading. Application requires GPU supporting OpenGL 3.3+
but may be compatible with older versions. Application logic was implemented
using C/C++ with some external helper libraries to handle basic operations.

File information
-----------------
This file implements shader program loader, compiler and linker. It's goal
is to load shaders from file using shader class (shader.cpp), compile and link 
them to make a shader Program object. This process is stored in log file located
in ./logs/shaders.txt file
*/

#include "shaders\ShaderProgram.h"

#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>

/// <summary>
/// Creates new shader program from provided vertex and fragment shaders.
/// </summary>
/// <param name="vs">Vertex shader file.</param>
/// <param name="fs">Fragment shader file.</param>
/// <param name="log">Compile log file.</param>
shprogram::shprogram(const char * vs, const char * fs, std::ofstream &log)
{ 
	log << "***********************************************\n";
	log << "Attempting to create shader from files:\n"
		<< "-----------------------------------------------\n"
		<< vs << "\n" << fs << "\n"
		<< "-----------------------------------------------\n";

	std::vector<shader> shaders = LoadShaders(vs, fs, log);

	if (shaders.size() <= 0)
	{
		throw std::runtime_error("No shaders were provided to create the program");
		log << "No shaders were provided to create the program\n";
	}
	
    //create the program object
    _object = glCreateProgram();
    if(_object == 0)
        throw std::runtime_error("glCreateProgram failed");

    //attach all the shaders
	for (unsigned i = 0; i < shaders.size(); ++i)
	{
		glAttachShader(_object, shaders[i].getObject());
	}

    //link the shaders together
    glLinkProgram(_object);

    //detach all the shaders
	for (unsigned i = 0; i < shaders.size(); ++i)
	{
		glDetachShader(_object, shaders[i].getObject());
	}

    //throw exception if linking failed
    GLint status;
    glGetProgramiv(_object, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {

        std::string msg = "Program linking failure: ";

        GLint infoLogLength;
        glGetProgramiv(_object, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* strInfoLog = new char[infoLogLength + 1];
        glGetProgramInfoLog(_object, infoLogLength, NULL, strInfoLog);
        msg += strInfoLog;
        

		log << msg << " " << strInfoLog << std::endl;

		delete[] strInfoLog;

        glDeleteProgram(_object); _object = 0;
        throw std::runtime_error(msg);
    }
	else
	{
		log << "-----------------------------------------------\n";
		log << "Shader program successfully created\n";
		log << "***********************************************\n\n";
	}
}


/// <summary>
/// Finalizes an instance of the <see cref="shprogram"/> class.
/// </summary>
shprogram::~shprogram()
{
    //might be 0 if creator fails by throwing exception
    if(_object != 0)
		glDeleteProgram(_object);
}

/// <summary>
/// .
/// </summary>
/// <returns>Shader program ID</returns>
GLuint shprogram::object() const
{
    return _object;
}

/// <summary>
/// Activates this shader.
/// </summary>
void shprogram::use() const
{
    glUseProgram(_object);
}

/// <summary>
/// Determines whether this shader is in use.
/// </summary>
/// <returns>True on success.</returns>
bool shprogram::isInUse() const
{
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

	return (currentProgram == (GLint)_object);
}

/// <summary>
/// Deactivates current shader (this shader).
/// </summary>
void shprogram::stopUsing() const
{
    assert(isInUse());
    glUseProgram(0);
}

/// <summary>
/// Bind shader attribute.
/// </summary>
/// <param name="attribName">Name of the attribute.</param>
/// <returns></returns>
GLint shprogram::attrib(const GLchar* attribName) const
{
    if(!attribName)
        throw std::runtime_error("attribName was NULL");

    GLint attrib = glGetAttribLocation(_object, attribName);
    if(attrib == -1)
        throw std::runtime_error(std::string("Program attribute not found: ") + attribName);

    return attrib;
}

/// <summary>
/// Binds shader uniform.
/// </summary>
/// <param name="uniformName">Name of the uniform.</param>
/// <returns></returns>
GLint shprogram::uniform(const GLchar* uniformName) const
{
    if(!uniformName)
        throw std::runtime_error("uniformName was NULL");

    GLint uniform = glGetUniformLocation(_object, uniformName);
    if(uniform == -1)
        throw std::runtime_error(std::string("Program uniform not found: ") + uniformName);

    return uniform;
}

/// Macro deffinition for binding any type of variable to shader uniform using
/// same function [Black Box]
#define ATTRIB_N_UNIFORM_SETTERS(OGL_TYPE, TYPE_PREFIX, TYPE_SUFFIX) \
\
    void shprogram::setAttrib(const GLchar* name, OGL_TYPE v0) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 1 ## TYPE_SUFFIX (attrib(name), v0); } \
    void shprogram::setAttrib(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 2 ## TYPE_SUFFIX (attrib(name), v0, v1); } \
    void shprogram::setAttrib(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 3 ## TYPE_SUFFIX (attrib(name), v0, v1, v2); } \
    void shprogram::setAttrib(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2, OGL_TYPE v3) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 4 ## TYPE_SUFFIX (attrib(name), v0, v1, v2, v3); } \
\
    void shprogram::setAttrib1v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 1 ## TYPE_SUFFIX ## v (attrib(name), v); } \
    void shprogram::setAttrib2v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 2 ## TYPE_SUFFIX ## v (attrib(name), v); } \
    void shprogram::setAttrib3v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 3 ## TYPE_SUFFIX ## v (attrib(name), v); } \
    void shprogram::setAttrib4v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 4 ## TYPE_SUFFIX ## v (attrib(name), v); } \
\
    void shprogram::setUniform(const GLchar* name, OGL_TYPE v0) \
        { assert(isInUse()); glUniform1 ## TYPE_SUFFIX (uniform(name), v0); } \
    void shprogram::setUniform(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1) \
        { assert(isInUse()); glUniform2 ## TYPE_SUFFIX (uniform(name), v0, v1); } \
    void shprogram::setUniform(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2) \
        { assert(isInUse()); glUniform3 ## TYPE_SUFFIX (uniform(name), v0, v1, v2); } \
    void shprogram::setUniform(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2, OGL_TYPE v3) \
        { assert(isInUse()); glUniform4 ## TYPE_SUFFIX (uniform(name), v0, v1, v2, v3); } \
\
    void shprogram::setUniform1v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform1 ## TYPE_SUFFIX ## v (uniform(name), count, v); } \
    void shprogram::setUniform2v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform2 ## TYPE_SUFFIX ## v (uniform(name), count, v); } \
    void shprogram::setUniform3v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform3 ## TYPE_SUFFIX ## v (uniform(name), count, v); } \
    void shprogram::setUniform4v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform4 ## TYPE_SUFFIX ## v (uniform(name), count, v); }

ATTRIB_N_UNIFORM_SETTERS(GLfloat, , f);
ATTRIB_N_UNIFORM_SETTERS(GLdouble, , d);
ATTRIB_N_UNIFORM_SETTERS(GLint, I, i);
ATTRIB_N_UNIFORM_SETTERS(GLuint, I, ui);

void shprogram::setUniformMatrix2(const GLchar* name, const GLfloat* v, GLsizei count, GLboolean transpose)
{
    assert(isInUse());
    glUniformMatrix2fv(uniform(name), count, transpose, v);
}

void shprogram::setUniformMatrix3(const GLchar* name, const GLfloat* v, GLsizei count, GLboolean transpose)
{
    assert(isInUse());
    glUniformMatrix3fv(uniform(name), count, transpose, v);
}

void shprogram::setUniformMatrix4(const GLchar* name, const GLfloat* v, GLsizei count, GLboolean transpose)
{
    assert(isInUse());
    glUniformMatrix4fv(uniform(name), count, transpose, v);
}

void shprogram::setUniform(const GLchar* name, const glm::mat2& m, GLboolean transpose)
{
    assert(isInUse());
    glUniformMatrix2fv(uniform(name), 1, transpose, glm::value_ptr(m));
}

void shprogram::setUniform(const GLchar* name, const glm::mat3& m, GLboolean transpose)
{
    assert(isInUse());
    glUniformMatrix3fv(uniform(name), 1, transpose, glm::value_ptr(m));
}

void shprogram::setUniform(const GLchar* name, const glm::mat4& m, GLboolean transpose)
{
    assert(isInUse() ? _object : -1);
    glUniformMatrix4fv(uniform(name), 1, transpose, glm::value_ptr(m));
}

void shprogram::setUniform(const GLchar* uniformName, const glm::vec3& v)
{
    setUniform3v(uniformName, glm::value_ptr(v));
}

void shprogram::setUniform(const GLchar* uniformName, const glm::vec4& v)
{
    setUniform4v(uniformName, glm::value_ptr(v));
}

/// <summary>
/// Loads the vertex shader and fragment shader, stores them in vector.
/// </summary>
/// <param name="vs">path to vertex/compute shader.</param>
/// <param name="fs">path to fragment shader, if NULL vs represents compute shader.</param>
/// <returns>vector of simple shaders to link</returns>
std::vector<shader> shprogram::LoadShaders(const char * vs, const char * fs, std::ofstream &log){

	std::vector<shader> shaders;

	if (fs)
	{
		log << "Shader program type:\tvertex + fragment\n";
	
		shaders.push_back(shader::readFile(vs, GL_VERTEX_SHADER, log));
		shaders.push_back(shader::readFile(fs, GL_FRAGMENT_SHADER, log));
	}
	else
	{
		log << "Shader program type:\tcompute shader\n";
		
		shaders.push_back(shader::readFile(vs, GL_COMPUTE_SHADER, log));
	}

	return shaders;
}

/// <summary>
/// Binds the tex to uni.
/// </summary>
/// <param name="binding">texture slot.</param>
/// <param name="tex">texture to bind.</param>
/// <param name="name">uniform name.</param>
bool shprogram::bindTexToUniform(GLuint binding, GLuint tex, const char *name){

	if (!name)
		throw std::runtime_error("Cannot bind texture to uniform, 'name' not found");

	GLint loc = glGetUniformLocation(_object, name);

	//assert(loc != -1);

	if (loc >= 0){
		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_2D, tex);

		glUniform1i(loc, binding);
	}

	return loc >= 0;
}

/// <summary>
/// Binds the buffer to uniform.
/// </summary>
/// <param name="binding">binding slot</param>
/// <param name="name">buffer name in shader</param>
/// <returns>TRUE if binded successfully</returns>
bool shprogram::bindBufferToUniform(GLuint binding, const char *name){
	
	if (!name)
		throw std::runtime_error("Cannot bind buffer to uniform, 'name' not found");

	GLint loc = glGetUniformBlockIndex(_object, name);

	assert(loc != -1);
	
	if (loc >= 0){
		glUniformBlockBinding(_object, loc, binding);
	}
	
	return loc >= 0;
}
