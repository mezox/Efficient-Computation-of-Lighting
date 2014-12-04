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
This file implements light culling by calculating screen space bounding quad
for every active light. Invisible lights are immediately culled, later culled once
more if depth optimalization is used.
*/

#include <algorithm>
#include <glm\glm.hpp>

/// <summary>
/// Updates the roots.
/// </summary>
/// <param name="Nc">Plane coordinate.</param>
/// <param name="Lc">Light position coordinate [x/y].</param>
/// <param name="Lz">Light position coordinate [z].</param>
/// <param name="Lr">Light's radius.</param>
/// <param name="proj">Projection matrix diagonal value.</param>
/// <param name="min">quad min coordinate.</param>
/// <param name="max">quad max coordinate.</param>
void updateRoots(float Nc, float Lc, float Lz, float Lr, float proj, float &min, float &max)
{
	float Nz = (Lr - Nc * Lc) / Lz;
	float Pz = (Lc * Lc + Lz * Lz - Lr * Lr) / (Lz - (Nz / Nc) * Lc);

	//check if point P lies in front of camera (z coords must be less than 0)
	if (Pz < 0.0f)
	{
		float c = -Nz * proj / Nc;
		if (Nc < 0.0f)
		{
			min = std::max(min, c);
		}
		else {
			max = std::min(max, c);
		}
	}
}

/// <summary>
/// Computes the roots of quadratic equation.
/// </summary>
/// <param name="Lc">Light position coordinate [x/y].</param>
/// <param name="Lz">Light position coordinate [z].</param>
/// <param name="Lr">Light's radius.</param>
/// <param name="proj">Projection matrix diagonal value.</param>
/// <param name="min">quad min coordinate.</param>
/// <param name="max">quad max coordinate.</param>
void computeRoots(float Lc, float Lz, float Lr, float proj, float &min, float &max)
{
	float LrSquare = Lr * Lr;
	float LcSquare = Lc * Lc;
	float LzSquare = Lz * Lz;

	float denominator = LcSquare + LzSquare;

	//eq (4.8)
	float D = LrSquare * LcSquare - denominator * (LrSquare - LzSquare);

	//check if point light does not fill whole screen
	if (D < 0.0)
	{
		return;
	}
	else
	{
		float Nx1 = (Lc * Lr + sqrt(D)) / denominator;
		float Nx2 = (Lc * Lr - sqrt(D)) / denominator;

		updateRoots(Nx1, Lc, Lz, Lr, proj, min, max);
		updateRoots(Nx2, Lc, Lz, Lr, proj, min, max);
	}
}

/// <summary>
/// Computes the bounding quad of specific light in view space.
/// </summary>
/// <param name="Lp">Light's position in view space.</param>
/// <param name="Lr">Light's radius/range.</param>
/// <param name="n">Near plane.</param>
/// <param name="projectionMatrix">The projection matrix.</param>
/// <returns>Screen space bounding box coordinates in clip space</returns>
glm::vec4 computeBoundingQuad(glm::vec3 Lp, float Lr, float n, const glm::mat4 &projectionMatrix)
{
	glm::vec4 boundingQuad = glm::vec4(1.0, 1.0, -1.0, -1.0);

	if (Lp.z - Lr <= -n)
	{
		boundingQuad = glm::vec4(-1.0, -1.0, 1.0, 1.0);

		computeRoots(Lp.x, Lp.z, Lr, projectionMatrix[0][0], boundingQuad.x, boundingQuad.z);
		computeRoots(Lp.y, Lp.z, Lr, projectionMatrix[1][1], boundingQuad.y, boundingQuad.w);
	}

	return boundingQuad;
}