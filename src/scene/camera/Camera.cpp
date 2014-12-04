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
This file represents camera class.
*/

#include "scene\camera\camera.h"
#include <glm\gtc\matrix_transform.hpp>
#include "configuration\config.h"

#define M_PI 3.14

//must be less than 90 to avoid gimbal lock
static const float MaxVerticalAngle = 89.0f; 

static inline float convertRatiansToDegrees(float radians)
{
    return radians * 180.0f / (float) M_PI;
}


Camera::Camera():
    m_position(0.0f, 0.0f, 1.0f),
    m_hAngle(0.0f),
    m_vAngle(0.0f),
    m_fov(67.0f),
    m_nearPlane(0.01f),
    m_farPlane(100.0f)
{
}

Camera::~Camera() {}

/// <summary>
/// Returns position of camera.
/// </summary>
/// <returns></returns>
const glm::vec3& Camera::position()
{
    return m_position;
}

/// <summary>
/// Sets camera to specific position.
/// </summary>
/// <param name="position">The position.</param>
void Camera::setPosition(const glm::vec3& position)
{
    m_position = position;
}

/// <summary>
/// Moves camera by the specified offset.
/// </summary>
/// <param name="offset">The offset.</param>
void Camera::move(const glm::vec3& offset)
{
	m_position += offset;
}

/// <summary>
/// Returns field of view of camera.
/// </summary>
/// <returns></returns>
float Camera::fov()
{
    return m_fov;
}

/// <summary>
/// Sets the field of view. Must be from interval (0, 180)
/// </summary>
/// <param name="fov">Angle.</param>
void Camera::setFov(float fov)
{
	assert(fov > 0.0f && fov < 180.0f);
    m_fov = fov;
}

/// <summary>
/// Returns near plane.
/// </summary>
/// <returns></returns>
float Camera::nearPlane()
{
    return m_nearPlane;
}

/// <summary>
/// Returns far plane.
/// </summary>
/// <returns></returns>
float Camera::farPlane()
{
    return m_farPlane;
}

/// <summary>
/// Sets the near plane.
/// </summary>
/// <param name="nearPlane">The near plane.</param>
void Camera::setNearPlane(float nearPlane)
{
	assert(nearPlane > 0.0f);
	m_nearPlane = nearPlane;
}


/// <summary>
/// Sets the far plane.
/// </summary>
/// <param name="farPlane">The far plane.</param>
void Camera::setFarPlane(float farPlane)
{
	assert(farPlane > m_nearPlane);
	m_farPlane = farPlane;
}

/// <summary>
/// Computes orientation matrix of camera.
/// </summary>
/// <returns></returns>
glm::mat4 Camera::orientation()
{
    glm::mat4 orientation;
    orientation = glm::rotate(orientation, m_vAngle, glm::vec3(1,0,0));
    orientation = glm::rotate(orientation, m_hAngle, glm::vec3(0,1,0));
    return orientation;
}

/// <summary>
/// Changes the view angles of camera based on mouse movement.
/// </summary>
/// <param name="upAngle">Up angle.</param>
/// <param name="rightAngle">The right angle.</param>
void Camera::angleOrientation(float upAngle, float rightAngle)
{
    m_hAngle += rightAngle;
    m_vAngle += upAngle;
    
	normalizeAngles();
}

/// <summary>
/// Set camera direction to specific point.
/// </summary>
/// <param name="position">The position.</param>
void Camera::lookAt(glm::vec3 position)
{
	assert(position != m_position);
    
	glm::vec3 direction = glm::normalize(position - m_position);
    
	m_vAngle = convertRatiansToDegrees(asinf(-direction.y));
    m_hAngle = -convertRatiansToDegrees(atan2f(-direction.x, -direction.z));
    
	normalizeAngles();
}

/// <summary>
/// Returns forward direction of camera.
/// </summary>
/// <returns>forward vector</returns>
glm::vec3 Camera::forward()
{
    glm::vec4 forward = glm::inverse(orientation()) * glm::vec4(0,0,-1,1);
    return glm::vec3(forward);
}

/// <summary>
/// Returns right direction of camera.
/// </summary>
/// <returns>right vector</returns>
glm::vec3 Camera::right()
{
    glm::vec4 right = glm::inverse(orientation()) * glm::vec4(1,0,0,1);
    return glm::vec3(right);
}

/// <summary>
/// Returns up direction vector of camera.
/// </summary>
/// <returns>up vector</returns>
glm::vec3 Camera::up()
{
    glm::vec4 up = glm::inverse(orientation()) * glm::vec4(0,1,0,1);
    return glm::vec3(up);
}

/// <summary>
/// Computes projection matrix.
/// </summary>
/// <returns>Projection matrix</returns>
glm::mat4 Camera::projection()
{
    return glm::perspective(m_fov, resolution.x/resolution.y, m_nearPlane, m_farPlane);
}

/// <summary>
/// Computes view matrix.
/// </summary>
/// <returns></returns>
glm::mat4 Camera::view()
{
    return orientation() * glm::translate(glm::mat4(), -m_position);
}

/// <summary>
/// Normalizes camera angles.
/// </summary>
void Camera::normalizeAngles()
{
    m_hAngle = fmodf(m_hAngle, 360.0f);
    
	//fmodf can return negative values, but this will make them all positive
	if (m_hAngle < 0.0f)
	{
		m_hAngle += 360.0f;
	}

	if (m_vAngle > MaxVerticalAngle)
	{
		m_vAngle = MaxVerticalAngle;
	}
        
	else if (m_vAngle < -MaxVerticalAngle)
	{
		m_vAngle = -MaxVerticalAngle;
	}
        
}
