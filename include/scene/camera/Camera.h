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
Camera class definition.
*/

#include <glm/glm.hpp>

/// <summary>
/// Camera class, implemented using glm library for matrix transformations
/// and GLFW for movement
/// </summary>
class Camera 
{
    public:
        Camera();
		~Camera();

		//fov get,set
		float fov();
        void setFov(float fov);

		//camera position get,set
		const glm::vec3& position();
		void setPosition(const glm::vec3& position);
		void move(const glm::vec3& offset);

		//near & far planes get, set
		float nearPlane();
		float farPlane();
		void setNearPlane(float nearPlane);
		void setFarPlane(float farPlane);

		//camera orientation based on mouse movement
		void angleOrientation(float upAngle, float rightAngle);
		glm::mat4 orientation();

		//camera directions
        glm::vec3 forward();
        glm::vec3 right();
        glm::vec3 up();

		//camera matrices
        glm::mat4 projection();
        glm::mat4 view();

		void lookAt(glm::vec3 position);

    protected:
		void normalizeAngles();

        glm::vec3 m_position;
        float m_hAngle;
        float m_vAngle;
        
		float m_fov;
        
		float m_nearPlane;
        float m_farPlane;
    };
