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
This file represents core of tiled shading techniques. Main goal here is to
construct grid of light lists for every tile. Light culling and light insertion
is also done here.
*/

#include "lighting\lights\PointLight.h"
#include "configuration\Config.h"
#include "configuration\Types.h"
#include "collision\SSBB.h"
#include "lighting\tiled\Grid.h"
#include <algorithm>

#define OFFSETS(i,j) (offsets[i + j * LIGHT_GRID_DIM_X])
#define COUNTS(i,j) (counts[i + j * LIGHT_GRID_DIM_X])

/// <summary>
/// Initializes a new instance of the <see cref="LightGrid"/> class.
/// </summary>
LightGrid::LightGrid()
{
}

/// <summary>
/// Finalizes an instance of the <see cref="LightGrid"/> class.
/// </summary>
LightGrid::~LightGrid()
{
	quads.clear();
	viewSpaceLights.clear();
	affectedTiles.clear();

	quads.shrink_to_fit();
	viewSpaceLights.shrink_to_fit();
	affectedTiles.shrink_to_fit();
}

/// <summary>
/// Builds the light grid.
/// </summary>
/// <param name="minMax">downsampled depth values of min/max.</param>
/// <param name="lights">The lights.</param>
/// <param name="n">near plane.</param>
/// <param name="view">view matrix.</param>
/// <param name="projection">projection matrix.</param>
void LightGrid::buildLightGrid(std::vector<MinMax> &minMax, Lights &lights, float n, const glm::mat4 &view, const glm::mat4 &projection)
{
	//store minimum/maximum depth to lightgrid
	gridMinMax = minMax;

	//compute ss bbs (bounding quads)
	computeBoundingQuads(lights,view,projection,n);

	//initialize light lists
	memset(offsets, 0, sizeof(offsets));
	memset(counts, 0, sizeof(counts));

	lightListLength = 0;

	//Find light count for each tile
	for (int i = 0; i < quads.size(); i++)
	{
		Light l = viewSpaceLights[i];

		for (unsigned int x = affectedTiles[i].x.x; x < affectedTiles[i].x.y - 1; x++)
		{
			for (unsigned int y = affectedTiles[i].y.x; y < affectedTiles[i].y.y - 1; y++)
			{
					//tests light against the minimum/maximum of depth buffer 
					if (gridMinMax.empty() || gridMinMax[y * grid_size.x + x].max < (l.position.z + l.radius) &&
						gridMinMax[y * grid_size.x + x].min >(l.position.z - l.radius))
					{
						lightListLength++;
						COUNTS(x, y) += 1;
					}
			}
		}
	}

	//set offsets
	unsigned int offset = 0;

	for (unsigned int y = 0; y < LIGHT_GRID_DIM_Y; y++)
	{
		for (unsigned int x = 0; x < LIGHT_GRID_DIM_X; x++)
		{
			unsigned int count = COUNTS(x, y);

			OFFSETS(x, y) = offset + count;
			offset += count;
		}
	}

	globalLightList.resize(lightListLength);
	
	if(quads.size() && !globalLightList.empty())
	{
		int *data = &globalLightList[0];
			
		for (unsigned int i = 0; i < quads.size(); ++i)
		{
			unsigned int lightId = i;

			Light l = viewSpaceLights[i];

			for (unsigned int x = affectedTiles[i].x.x; x < affectedTiles[i].x.y - 1; x++)
			{
				for (unsigned int y = affectedTiles[i].y.x; y < affectedTiles[i].y.y - 1; y++)
				{
					//tests light against the minimum/maximum of depth buffer 
					if (gridMinMax.empty() || gridMinMax[y * grid_size.x + x].max < (l.position.z + l.radius) &&
						gridMinMax[y * grid_size.x + x].min >(l.position.z - l.radius)){

						// store reversely into next free slot
						unsigned int offset = OFFSETS(x, y) - 1;
						data[offset] = lightId;
						OFFSETS(x, y) = offset;
					}
				}
			}
		}
	}
}

/// <summary>
/// Renders lights' bounding quads.
/// - for debugging purposes
/// - uses deprecated functions, wont work with core profile
/// </summary>
void LightGrid::showLightQuads()
{
	unsigned count = viewSpaceLights.size();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, float(resolution.x), 0.0, float(resolution.y), -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	for (unsigned int i = 0; i < count; i++)
	{
		glBegin(GL_LINE_STRIP);

			glColor4f(1.0, 1.0, 1.0, 1.0f);

			glVertex2f(float(quads[i].min.x), float(quads[i].min.y));
			glVertex2f(float(quads[i].max.x), float(quads[i].min.y));
			glVertex2f(float(quads[i].max.x), float(quads[i].max.y));
			glVertex2f(float(quads[i].min.x), float(quads[i].max.y));
			glVertex2f(float(quads[i].min.x), float(quads[i].min.y));

		glEnd();
	}

	glPopAttrib();
}

/// <summary>
/// Compute light's bounding quad in screen space viewport.
/// </summary>
/// <param name="lights">vector of pointlights.</param>
/// <param name="view">view matrix.</param>
/// <param name="projection">projection matrix.</param>
/// <param name="n">near plane of view frustum.</param>
void LightGrid::computeBoundingQuads(const Lights &lights, const glm::mat4 &view, const glm::mat4 &projection, float n)
{
	//clear vectors
	quads.clear();
	viewSpaceLights.clear();
	affectedTiles.clear();
	
	for(unsigned int i = 0; i < lights.size(); i++)
	{
		Light l = lights[i];
		
		//transform world light position to view space 
		glm::vec3 posVS = glm::vec3(view * glm::vec4(l.position, 1.0));
		
		//compute bounding quad in clip space
		glm::vec4 clip = computeBoundingQuad(posVS, l.radius, n, projection);

		//transform quad to viewport
		clip = -clip;
		swap(clip.x, clip.z);
		swap(clip.y, clip.w);

		//convert to the [0.0, 1.0] range
		clip *= 0.5f;
		clip += 0.5f;

		//convert clip region to viewport
		BoundingBox quad;
		quad.min.x = clip.x * resolution.x;
		quad.min.y = clip.y * resolution.y;
		quad.max.x = clip.z * resolution.x;
		quad.max.y = clip.w * resolution.y;

		//store viewspace lights and their quads
		//lights are stored in world space
		if (quad.min.x < quad.max.x && quad.min.y < quad.max.y)
		{
			//store ss quad
			quads.push_back(quad);

			//convert light to view space and store it as viewspace light
			l.position = posVS;
			viewSpaceLights.push_back(l);

			computeLightAffectedTiles(quad.min.x, quad.max.x, quad.min.y, quad.max.y);
		}
	}
}


/// <summary>
/// Computes an area (tiles) which is affected by Light from its bounding quad (screen space)
///	and store it in vector.
/// </summary>
/// <param name="minx">left bottom x coordinate.</param>
/// <param name="maxx">right bottom x coordinate.</param>
/// <param name="miny">left bottom y coordinate.</param>
/// <param name="maxy">right bottom y coordinate.</param>
void LightGrid::computeLightAffectedTiles(float minx, float maxx, float miny, float maxy)
{
	glm::vec2 x = glm::vec2(minx / TILE_SIZE_XY, (maxx + TILE_SIZE_XY - 1)/ TILE_SIZE_XY);
	glm::vec2 y = glm::vec2(miny/ TILE_SIZE_XY, (maxy + TILE_SIZE_XY - 1) / TILE_SIZE_XY);

	TileArea tmp;

	tmp.x = glm::clamp(x, glm::vec2(0.0), glm::vec2(grid_size.x + 1,grid_size.x + 1));
	tmp.y = glm::clamp(y, glm::vec2(0.0), glm::vec2(grid_size.y + 1, grid_size.y + 1));

	affectedTiles.push_back(tmp);
}