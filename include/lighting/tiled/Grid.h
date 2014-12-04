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
Light grid definition.
*/

#include <GL\glew.h>
#include <vector>
#include <glm\glm.hpp>
#include <iostream>
#include <fstream>

class LightGrid
{
	public:
		LightGrid();
		~LightGrid();

		void showLightQuads();
		void showLightTiles();

		unsigned int *getCounts(){
			return counts;
		}
		
		unsigned int *getOffsets(){
			return offsets;
		}

		unsigned int getLightListLength(){
			return (unsigned int)globalLightList.size();
		}

		const int *getLightList() const { return &globalLightList[0]; }
		const Lights &getViewSpaceLights() const { return viewSpaceLights; }

		void buildLightGrid(std::vector<MinMax> &minMax, Lights &lights, float n, const glm::mat4 &view, const glm::mat4 &projection);
		
	protected:

		void computeBoundingQuads(const Lights &lights, const glm::mat4 &modelView, const glm::mat4 &projection, float n);
		void computeLightAffectedTiles(float minx, float maxx, float miny, float maxy);

		unsigned int lightListLength;
		std::vector<BoundingBox> quads;
		std::vector<Light> viewSpaceLights;
		std::vector<TileArea> affectedTiles;

		std::vector<int> globalLightList;
		unsigned int offsets[TILES_COUNT];
		unsigned int counts[TILES_COUNT];
		
		std::vector<MinMax> gridMinMax;

};