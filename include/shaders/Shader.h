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
Definition of shader class.
*/

#include <GL/glew.h>
#include <string>

class shader {

    public:

		static shader readFile(const std::string& file, GLenum type, std::ofstream &log);

		shader(const std::string& file, GLenum type, std::ofstream &log);

        GLuint getObject() const;

        shader(const shader& other);

        shader& operator =(const shader& other);

        ~shader();

    private:

        GLuint _object;
        unsigned* _refCount;
		unsigned int _version = 330;
		
		static bool insertMacro(std::string name, std::string value, std::string &buf);

        void _retain();
        void _release();
};
