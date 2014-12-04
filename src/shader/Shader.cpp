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
This file implements shader loader and compiler. It's goal is to load shaders code 
from file and generate preprocessor definitions to make final shaders based on user
defined parameters (see config.h).
*/

#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <iostream>
#include "shaders\Shader.h"
#include "configuration\config.h"

/// <summary>
/// Compiles new vertex/fragment shader.
/// </summary>
/// <param name="file">The file.</param>
/// <param name="type">The type.</param>
/// <param name="log">The log.</param>
shader::shader(const std::string& file, GLenum type, std::ofstream &log) : _object(0), _refCount(NULL)
{
	//create the shader object
    _object = glCreateShader(type);

	//check state of createshader
	if (_object == 0)
	{
		throw std::runtime_error("glCreateShader failed");
		log << "Shader creation failed. [glCreateShader]\n";
	}
        
    //set the source code
    const char* code = file.c_str();
    glShaderSource(_object, 1, (const GLchar**)&code, NULL);

    //compile
    glCompileShader(_object);

	//throw exception if compile error occurred
    GLint status;
    glGetShaderiv(_object, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
	{
        std::string msg("Compile failure in shader:\n");

        GLint infoLogLength;
        glGetShaderiv(_object, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* strInfoLog = new char[infoLogLength + 1];
        glGetShaderInfoLog(_object, infoLogLength, NULL, strInfoLog);
        msg += strInfoLog;
        delete[] strInfoLog;

		log << msg << std::endl;

        glDeleteShader(_object); _object = 0;
        throw std::runtime_error(msg);
    }

    _refCount = new unsigned;
    *_refCount = 1;
}


shader::shader(const shader& other):_object(other._object),_refCount(other._refCount)
{
    _retain();
}

/// <summary>
/// Finalizes an instance of the <see cref="shader"/> class.
/// </summary>
shader::~shader()
{
	if (_refCount)
	{
		_release();
	}
	
}


/// <summary>
/// .
/// </summary>
/// <returns>Shader ID</returns>
GLuint shader::getObject() const
{
    return _object;
}

/// <summary>
/// .
/// </summary>
/// <param name="other">The other.</param>
/// <returns>Instance of this shader</returns>
shader& shader::operator = (const shader& other)
{
    _release();
    _object = other._object;
    _refCount = other._refCount;
    _retain();

    return *this;
}

/// <summary>
/// Inserts the macro into shader.
/// </summary>
/// <param name="name">Name of preprocessor definition.</param>
/// <param name="value">Value.</param>
/// <param name="buf">Buffer of preprocessor definitions.</param>
/// <returns></returns>
bool shader::insertMacro(std::string name, std::string value, std::string &buf)
{
	buf.append("#define " + name + " " + value + "\n");
	return true;
}


/// <summary>
/// Reads the shader file.
/// </summary>
/// <param name="file">shader definition file.</param>
/// <param name="type">type of shader [vertex/fragment].</param>
/// <param name="log">shader compilation log file.</param>
/// <returns></returns>
shader shader::readFile(const std::string& file, GLenum type, std::ofstream &log)
{
	log << "----------------------------------------------\n";

	if (type == GL_VERTEX_SHADER)
	{
		log << "Loading vertex shader file:\t" << file << std::endl;
	}
	else if (type == GL_FRAGMENT_SHADER)
	{
		log << "Loading fragment shader file:\t" << file << std::endl;
	}
	else
	{
		log << "Loading compute shader file:\t" << file << std::endl;
	}

    //open file
    std::ifstream f;
    f.open(file.c_str(), std::ios::in | std::ios::binary);

    //check open state
    if(!f.is_open())
	{
        throw std::runtime_error(std::string("Failed to open file: ") + file);
		log << "Failed to open file:" << file << std::endl;
    }

    //read whole file into stringstream buffer
    std::stringstream buffer;
    buffer << f.rdbuf();

	std::string version = "#version 330 compatibility\n\n";
	
	const std::string temp = buffer.str();
	buffer.seekp(0);

	insertMacro("GRID_X", S_GRID_X, version);
	insertMacro("GRID_Y", S_GRID_Y, version);
	
	insertMacro("WIDTH", S_RES_X, version);
	insertMacro("HEIGHT", S_RES_Y, version);

	insertMacro("TILE_DIM", S_TILE_DIM, version);
	insertMacro("TILES_COUNT", S_TILES_COUNT, version);

	insertMacro("MAX_LIGHTS", S_MAX_LIGHTS, version);

	buffer << version;
	buffer << temp;

    //return new shader
    shader shader(buffer.str(), type, log);

	log << "File successfully loaded\n";

    return shader;
}


/// <summary>
/// Retains this instance.
/// </summary>
void shader::_retain()
{
    assert(_refCount);
    *_refCount += 1;
}


/// <summary>
/// Releases this instance.
/// </summary>
void shader::_release()
{
    assert(_refCount && *_refCount > 0);
    *_refCount -= 1;

	if(*_refCount == 0)
	{
        glDeleteShader(_object);
		_object = 0;
        
		delete _refCount;
		_refCount = NULL;
    }
}
