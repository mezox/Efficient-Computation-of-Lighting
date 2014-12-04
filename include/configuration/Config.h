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
Application configuration file. To change application parameters as resolution,
tile size or lights' count change corresponding values. To apply changes application
must be compiled to generate new shaders.
*/

#include <glm/glm.hpp>
#include <algorithm>
#include <array>

//to enable deferred shading on NVIDIA change this to 1
#define FORCE_AMD_CONFIG 0

//window
#define RES_X 1280
#define RES_Y 720

//quads
#define QUAD_WIDTH		(RES_X - (RES_X / 4)) / 4
#define QUAD_HEIGHT		RES_Y / 6
#define QUAD_POS		RES_X - (QUAD_WIDTH + RES_X / 20)

//lights
#define MAX_LIGHTS 1024
#define LIGHT_RADIUS_MIN 100.0
#define LIGHT_RADIUS_MAX 400.0

//lightgrid constants
#define TILE_SIZE_XY		32
#define LIGHT_GRID_DIM_X	((RES_X + TILE_SIZE_XY - 1) / TILE_SIZE_XY)
#define LIGHT_GRID_DIM_Y	((RES_Y + TILE_SIZE_XY - 1) / TILE_SIZE_XY)
#define TILES_COUNT			LIGHT_GRID_DIM_X * LIGHT_GRID_DIM_Y

//shader generator double expansion hack
#define S(x)			#x
#define S_(x)			S(x)

#define S_GRID_X		S_(LIGHT_GRID_DIM_X)
#define S_GRID_Y		S_(LIGHT_GRID_DIM_Y)
#define S_RES_X			S_(RES_X)
#define S_RES_Y			S_(RES_Y)
#define S_TILES_COUNT	S_(TILES_COUNT)
#define S_MAX_LIGHTS	S_(MAX_LIGHTS)
#define S_TILE_DIM		S_(TILE_SIZE_XY)

//mouse
#define MOUSE_SENSITIVITY 0.05


template <typename T>
void swap(T &a, T &b)
{
	T tmp(a);
	a = b;
	b = tmp;
}

const glm::vec2 resolution(RES_X, RES_Y);
const glm::vec2 tile_size(TILE_SIZE_XY, TILE_SIZE_XY);
const glm::vec2 grid_size(LIGHT_GRID_DIM_X, LIGHT_GRID_DIM_Y);

typedef struct{
	glm::vec2 min;
	glm::vec2 max;
} BoundingBox;

struct TileArea {
	glm::vec2 x;
	glm::vec2 y;
};

typedef struct
{
	float min;
	float max;
} MinMax;