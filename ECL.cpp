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
This file represents main source code file containing function main().
Basically whole important application logic is implemented in this file.
*/

//external
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <external\AntTweakBar\AntTweakBar.h>

//standard C++ libraries
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <string>
#include <locale>
#include <vector>
#include <direct.h>
#include <sstream>

#include "shaders\ShaderProgram.h"
#include "scene\camera\Camera.h"		
#include "scene\objloader\Mesh.h"
#include "lighting\lights\PointLight.h"
#include "buffers\g-buffer\Gbuffer.h"

//modules
#include "utils\Utils.h"
#include "lighting\tiled\Grid.h"
#include "buffers\ubo\buffer.h"
#include "configuration\Types.h"
#include "configuration\Enums.h"
#include "utils\timers\PerformanceTimer.h"


//	GLOBAL VARIABLES
#pragma region GLOBAL_VARIABLES
std::string windowTitle = "Efficient Computation of Lighting: ";
VENDOR GPUVendor = AMD;

static Matrices transformationMatrices;
static unsigned int technique = AMD_TiledDeferred;

double mouseXold, mouseYold;

unsigned int LIGHT_COUNT = 0;

//lights
Lights pointLights;
double start_time = 0.0f;

bool decFlag = true;

#pragma region Feature_Settings
bool showMRTQuads = false;
bool showBoundingQuads = false;
bool showAffectedTiles = false;
bool showLightHeatMap = false;
bool minMaxPass = true;
#pragma endregion Feature_Settings

#pragma region Performance_Outputs
std::vector<double> fpsVector;
std::vector<double> lightingTime;
std::vector<double> minmaxTime;
std::vector<double> gridTime;
#pragma endregion Performance_Outputs

#pragma region Shader_Programs
shprogram   * mrt = NULL;
shprogram   * deferredShader = NULL;
shprogram   * quadShader = NULL;
shprogram   * quadDShader = NULL;
shprogram   * minMaxDepthShader = NULL;
shprogram   * simpleShader = NULL;
shprogram	* tiledDeferredShader = NULL;
shprogram	* tiledForwardShader = NULL;
shprogram	* lightHeatMapShader = NULL;
shprogram	* affectedTilesShader = NULL;
#pragma endregion Shader_Programs

unsigned int lastLightCnt = MAX_LIGHTS;

bool showGBufferQuad[GBuffer::GBUFFER_NUM_TEXTURES] = {false};
bool showDepth = false;

//move position of camera based on WASD keys, and XZ keys for up and down
float moveSpeed = 400.0; //units per second

//camera object
Camera      gCamera;

//application window
GLFWwindow  * win = NULL;

//meshes
Mesh        * m_pMesh = NULL;		//scene
Mesh        * m_sphere = NULL;		//pointlight sphere

//gBuffer object
GBuffer     *gBuf = new GBuffer();
//gbuffer textures

GLuint gBufTexIndex = 4;

//VBOs
GLuint	quadVBO;

//models of point lights for deferred shading
glm::mat4 lightSpheres[MAX_LIGHTS];

LightGrid lightgrid;

#pragma region Framebuffers
GLuint minMaxDepthFbo;	//minMax downsample framebuffer
GLuint forwardFbo;		//forward framebuffer
#pragma endregion Framebuffers

#pragma region Queries
GLuint minMaxDepthQuery;
GLuint minMaxDepthQueryTime = 0;

GLuint lightingQuery;
GLuint lightingQueryTime;
#pragma endregion Queries

#pragma region Textures
GLuint gTexDiffuse, gTexNormal, gTexPos, gTexDepth, gTexSpec, gTexAmbient, depthTex; // G-Buffer Textures
GLuint forwardTex;
GLuint minMaxDepthTex;
GLuint lightIDtex = 0;	//LightIDs texture for tiled shading
#pragma endregion Textures

#pragma region Unifom_Buffer_Objects
GlBufferObject<int> lightIndicesBuffer;
GlBufferObject<glm::ivec4> countsAndOffsetsBuffer;
GlBufferObject<glm::vec4> posAndRadiusesBuffer;
GlBufferObject<glm::vec4> colorsBuffer;
#pragma endregion Unifom_Buffer_Objects

std::string AMDtechniqueNames[AMD_Max] = 
{
	"Simple",
	"TiledDeferred",
	"TiledForward",
	"Deferred"
};

std::string NVIDIAtechniqueNames[NVIDIA_Max] =
{
	"Simple",
	"TiledDeferred",
	"TiledForward"
};

#pragma endregion GLOBAL_VARIABLES


/// <summary>
/// Updates transformation matrices.
/// </summary>
static void updateMatrices()
{
	transformationMatrices.view = gCamera.view();
	transformationMatrices.projection = gCamera.projection();
	transformationMatrices.viewProjection = gCamera.projection() * gCamera.view();
	transformationMatrices.normal = glm::transpose(glm::inverse(transformationMatrices.view));
	transformationMatrices.inverseProjection = glm::inverse(gCamera.projection());
}


/// <summary>
/// Point lights matrices initialization, creates model matrix for sphere mesh,
/// translate to light's world position, scale to light's radius size
/// </summary>
static void initLightSpheres(unsigned count)
{
	for (unsigned i = 0; i < count; i++)
	{
		lightSpheres[i] = glm::scale(glm::translate(glm::mat4(), pointLights[i].position), glm::vec3(pointLights[i].radius));
	}
}


/// <summary>
/// Generates lights and their model matrices for deferred shading.
/// </summary>
/// <param name="count">lights' count.</param>
/// <param name="min">minimal radius.</param>
/// <param name="max">maximal radius.</param>
static void generateLights(unsigned short count, float min, float max)
{
	pointLights.resize(count);

	for (unsigned i = 0; i < count; i++)
	{
		pointLights[i].color = glm::vec3(randf(0.0, 1.0), randf(0.0, 1.0), randf(0.0, 1.0));
		pointLights[i].radius = randf(min, max);

		//replace values in randf for another scene boundaries
		pointLights[i].position = glm::vec3(randf(-1750.0, 1750.0), randf(0.0, 1550.0), randf(-1000.0, 1000.0));
	}

	//initialize sphere models to light pos for deferred shading
	initLightSpheres(count);

	//update light count
	LIGHT_COUNT = count;
}


/// <summary>
/// Renders screen space quad.
/// </summary>
/// <param name="shader">The shader.</param>
static void renderQuad(shprogram * shader)
{
	shader->use();

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	{
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Draw the triangles !
		// 2*3 indices starting at 0 -> 2 triangles
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	glDisableVertexAttribArray(0);

	shader->stopUsing();
}


/// <summary>
/// Keyboard callback function.
/// </summary>
/// <param name="window">context window</param>
/// <param name="key">key</param>
/// <param name="scancode">scancode of key</param>
/// <param name="action">action</param>
/// <param name="mods">modifiiers</param>
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			//increase camera movement speed
			case GLFW_KEY_KP_ADD:
				moveSpeed += 100.0;
				break;

			//decrease camera movement speed
			case GLFW_KEY_KP_SUBTRACT:
				moveSpeed -= 100.0;
				break;

			//show MRT quads
			case GLFW_KEY_M:
				showMRTQuads = !showMRTQuads;
				break;

			//generate +/- 128 lights
			case GLFW_KEY_G:
			{
				if (pointLights.size() <= 1024 && decFlag && pointLights.size() >= 256)
				{
					pointLights.clear();
					pointLights.shrink_to_fit();

					lastLightCnt -= 128;
				}
				else
				{
					decFlag = false;

					pointLights.clear();
					pointLights.shrink_to_fit();

					lastLightCnt += 128;

					if (lastLightCnt == 1024)
						decFlag = true;
				}

				generateLights(lastLightCnt, LIGHT_RADIUS_MIN, LIGHT_RADIUS_MAX);
			}
			break;


			//show affected tiles
			case GLFW_KEY_6:
			{
				if (technique)
					showAffectedTiles = !showAffectedTiles;
			}
			break;

			//show bounding quads
			case GLFW_KEY_B:
			{
				if (technique != AMD_Simple && technique != AMD_Deferred){
					showBoundingQuads = !showBoundingQuads;
				}
					
			}
			break;

			//switch rendering technique
			case GLFW_KEY_T:
			{
				if (GPUVendor == NVIDIA)
				{
					technique = NVIDIARenderingTechnique((technique + 1) % NVIDIA_Max);
				}
				else
				{
					technique = AMDRenderingTechnique((technique + 1) % AMD_Max);
				}
				
			}
			break;

			//depth min max optimization
			case GLFW_KEY_U:
			{
				if (technique == AMD_TiledDeferred || technique == AMD_TiledForward)
					minMaxPass = !minMaxPass;
			}
			break;

				//generate new colors for existing lights
			case GLFW_KEY_R:
				for (unsigned int i = 0; i < pointLights.size(); i++){
					pointLights[i].color = glm::vec3(randf(0.0, 1.0), randf(0.0, 1.0), randf(0.0, 1.0));
				}
				break;

			//generate new colors for existing lights
			case GLFW_KEY_J:
				showLightHeatMap = !showLightHeatMap;
				break;
			}
	}
}


/// <summary>
/// Mouse button callback function for AntTweakBar.
/// </summary>
/// <param name="button">mouse button.</param>
/// <param name="action">action.</param>
static void MouseButtonCB(GLFWwindow*, int button, int action, int mods)
{
	TwEventMouseButtonGLFW(button, action);
}


/// <summary>
/// Cursor position callback function for AntTweakBar.
/// </summary>
/// <param name="x">mouse x-coordinate</param>
/// <param name="y">mouse y-coordinate</param>
static void MousePosCB(GLFWwindow*, double x, double y)
{
	TwEventMousePosGLFW((int)x, (int)y);
}


/// <summary>
/// Keyboard callback function for AntTweakBar.
/// </summary>
/// <param name="window">graphic context window.</param>
/// <param name="key">key.</param>
/// <param name="scancode">scancode.</param>
/// <param name="action">action.</param>
/// <param name="mods">modifikators.</param>
static void KeyFunCB(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	TwEventKeyGLFW(key, action);
	TwEventCharGLFW(key, action);
}


/// <summary>
/// Fills view space lights' data into Buffers and binds these buffers as UBOs
/// </summary>
/// <param name="grid">The grid.</param>
static void bindGridBuffers(LightGrid &grid)
{
	glm::ivec4 countsAndOffsets[TILES_COUNT];
	glm::vec4 posAndRadiuses[MAX_LIGHTS];
	glm::vec4 colors[MAX_LIGHTS];

	//check validity of global light list
	if (grid.getLightListLength())
	{
		//get viewspace lights from lightgrid
		const Lights &lights = grid.getViewSpaceLights();

		//fetch position,radiuses and colors into buffers
		for (unsigned int i = 0; i < lights.size(); i++)
		{
			posAndRadiuses[i] = glm::vec4(lights[i].position, lights[i].radius);
			colors[i] = glm::vec4(lights[i].color, 1.0f);
		}

		//get tile light counts and offsets from lightgrid
		unsigned int * counts = grid.getCounts();
		unsigned int * offsets = grid.getOffsets();

		//fetch array
		for (unsigned int i = 0; i < TILES_COUNT; i++)
		{
			countsAndOffsets[i] = glm::ivec4(counts[i], offsets[i], 0, 0);
		}

		//copy data into buffers
		countsAndOffsetsBuffer.copyFromHost(countsAndOffsets, TILES_COUNT);
		posAndRadiusesBuffer.copyFromHost(posAndRadiuses, MAX_LIGHTS);
		colorsBuffer.copyFromHost(colors, MAX_LIGHTS);
		lightIndicesBuffer.copyFromHost(grid.getLightList(), grid.getLightListLength());

		//bind buffers to binding slots
		countsAndOffsetsBuffer.bindSlot(GL_UNIFORM_BUFFER, TBB_LightGrid);
		posAndRadiusesBuffer.bindSlot(GL_UNIFORM_BUFFER, TBB_LightPosAndRadius);
		colorsBuffer.bindSlot(GL_UNIFORM_BUFFER, TBB_LightColors);

		//bind light's ID texture
		glActiveTexture(GL_TEXTURE0 + TDTB_LightIndex);
		glBindTexture(GL_TEXTURE_BUFFER, lightIDtex);
	}
}


/// <summary>
/// Sets the quad vbo.
/// </summary>
static void createQuadVBO()
{
    GLfloat vertices[] = 
	{
        -1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f, 1.0f,
	
		-1.0f, 1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
    };

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices), vertices, GL_STATIC_DRAW);
}

/// <summary>
/// Renders Multiple Render Targets/debug Quads. We have to check texture type, 
/// because we need to bind another shader for linear depth rendering.
/// </summary>
/// <param name="x">x-coord of left bottom corner.</param>
/// <param name="y">y-coord of left bottom corner.</param>
/// <param name="width">quad width.</param>
/// <param name="height">quad height.</param>
/// <param name="tex">rendered texture.</param>
static void renderMRTquad(unsigned x, unsigned y, unsigned width, unsigned height, GLuint tex)
{
	shprogram * shader;

	//wanna render depth?
	if (tex != gTexDepth && tex != minMaxDepthTex)
		shader = quadShader;
	else
		shader = quadDShader;
	
	shader->use();
		shader->bindTexToUniform(0, tex, "in_tex");

		//change viewport to quad position & size
		glViewport(x, y, width, height);

		//1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		{
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			//draw the triangles !
			//2*3 indices starting at 0 -> 2 triangles
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		glDisableVertexAttribArray(0);

		//restore window viewport
		glViewport(0, 0, resolution.x, resolution.y);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	shader->stopUsing();
}


/// <summary>
/// GBuffer/Debug info - renders Multiple Render Targets quads to the screen.
/// </summary>
static void showMRT()
{
	//render diffuse,normal,pos,depth textures to quads
	if (showMRTQuads && technique != AMD_Simple)
	{
			if (technique != AMD_TiledForward)
			{
				renderMRTquad(QUAD_POS, QUAD_HEIGHT * 5, QUAD_WIDTH, QUAD_HEIGHT, minMaxDepthTex);
				renderMRTquad(QUAD_POS, QUAD_HEIGHT * 4, QUAD_WIDTH, QUAD_HEIGHT, gTexDiffuse);
				renderMRTquad(QUAD_POS, QUAD_HEIGHT * 3, QUAD_WIDTH, QUAD_HEIGHT, gTexNormal);
				renderMRTquad(QUAD_POS, QUAD_HEIGHT * 2, QUAD_WIDTH, QUAD_HEIGHT, gTexPos);
				renderMRTquad(QUAD_POS, QUAD_HEIGHT, QUAD_WIDTH, QUAD_HEIGHT, gTexSpec);
			}
			
			if (technique == AMD_TiledForward)
			{
				renderMRTquad(QUAD_POS, QUAD_HEIGHT * 2, QUAD_WIDTH, QUAD_HEIGHT, minMaxDepthTex);
			}

			renderMRTquad(QUAD_POS, 0, QUAD_WIDTH, QUAD_HEIGHT, gTexDepth);
	}

	//renders depth to application "fullscreen"
	if (showDepth)
	{
		renderMRTquad(0, 0, resolution.x, resolution.y, gTexDepth);
	}
}


/// <summary>
/// Unbinds active textures.
/// </summary>
/// <param name="count">active textures count.</param>
static void unbindTextures(unsigned short count)
{
	for (unsigned int i = 0; i < count; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


/// <summary>
/// Early-Z pass for tiled forward shading.
/// </summary>
static void depthPrePass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, forwardFbo);
	glDepthFunc(GL_LEQUAL);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	//bind Multiple Render Targets shader program
	simpleShader->use();

		//set the model view projection uniform
		simpleShader->setUniform("MVP", transformationMatrices.viewProjection);

		//render meshes
		m_pMesh->RenderSimple();

	//unbind shader program
	simpleShader->stopUsing();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}


/// <summary>
/// Calculates the minimum and maximum depth. Outputs vec4 containing vec2() 
/// min/max depth per tile in view space and vec2() min /max clamped to <0.0,1.0>
/// </summary>
/// <param name="tileDepthRanges">vector to store downsampled values.</param>
static void calcMinMaxDepth(std::vector<MinMax> &tileDepthRanges)
{
	glBindFramebuffer(GL_FRAMEBUFFER, minMaxDepthFbo);

	//clear color buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, grid_size.x, grid_size.y);

	//use downsampling shader
	minMaxDepthShader->use();

		minMaxDepthShader->bindTexToUniform(0, gTexDepth, "depthTex");
		minMaxDepthShader->setUniform("inverseProjectionMatrix", transformationMatrices.inverseProjection);

	minMaxDepthShader->stopUsing();

	//render quad
	renderQuad(minMaxDepthShader);

	tileDepthRanges.resize((unsigned short)grid_size.x * (unsigned short)grid_size.y);

	//read values from pixel buffer and store them in vector
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glReadPixels(0, 0, grid_size.x, grid_size.y, GL_RG, GL_FLOAT, &tileDepthRanges[0]);

	//bind default fbo and restore viewport
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, resolution.x, resolution.y);
}


/// <summary>
/// Geometry pass of deferred shading.
/// -	geometry is rendered to G-buffer
/// -	stores positions, normals, specular and diffuse textures
/// -	updates depth buffer
/// </summary>
void DSgeometryPass()
{
	glEnable(GL_DEPTH_TEST);

    //bind G-buffer for current pass
    gBuf->bindForGeomPass();

    //bind Multiple Render Targets shader program and render scene
    mrt->use();
		mrt->setUniform("projection", transformationMatrices.projection);
		mrt->setUniform("view", transformationMatrices.view);
		mrt->setUniform("model", glm::mat4());

		m_pMesh->Render(mrt->object());

	mrt->stopUsing();

     //do not update depth buffer
	glDisable(GL_DEPTH_TEST);
}


/// <summary>
/// Lighting pass of deferred shading.
/// -	applies lights to textures from Gbuffer
/// </summary>
void DSlightPass()
{
	//shading pass
	gBuf->bindForLightPass();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	deferredShader->use();

		//set transformation positions
		for (unsigned int i = 0; i < pointLights.size(); i++)
		{

			glm::vec4 posRad = transformationMatrices.view * (glm::vec4(pointLights[i].position, 1.0));
			posRad.w = pointLights[i].radius;
			glm::mat4 MVP = transformationMatrices.viewProjection * lightSpheres[i];

			deferredShader->setUniform("MVP", MVP);
			deferredShader->setUniform("light.positionRadius", posRad);
			deferredShader->setUniform("light.color", glm::vec3(pointLights[i].color));

			m_sphere->Render(deferredShader->object());
		}

	deferredShader->stopUsing();

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	unbindTextures(DTB_Max);
}


/// <summary>
/// Lighting pass of tiled deferred shading.
/// </summary>
static void TDSlightPass()
{
	glViewport(0, 0, resolution.x, resolution.y);

	//bind gbuffer for writing to ambient light texture
	gBuf->bindForLightPass();
	
	tiledDeferredShader->use();

		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		{
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// Draw the triangles !
			// 2*3 indices starting at 0 -> 2 triangles
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		glDisableVertexAttribArray(0);
	
	tiledDeferredShader->stopUsing();

	//unbind bound textures
	unbindTextures(TDTB_Max);
}


/// <summary>
/// Rendering function. Uses specific method of shading based on chosen parameters.
/// </summary>
static void Render()
{
	//update transformation matrices
	updateMatrices();

	//timer for light grid build
	PerformanceTimer gridTimer;

	switch(technique)
	{
		//simple shading with no lights
		#pragma region SIMPLE_SHADING
		case AMD_Simple:
		{
			glEnable(GL_DEPTH_TEST);
			
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
			//bind Multiple Render Targets shader program
			simpleShader->use();

				//set the model view projection uniform
				simpleShader->setUniform("MVP", transformationMatrices.viewProjection);

				//render meshes
				m_pMesh->RenderSimple();

			//unbind shader program
			simpleShader->stopUsing();

			glDisable(GL_DEPTH_TEST);
		}
		break;
		#pragma endregion SIMPLE_SHADING

		//standard deferred shading
		#pragma region DEFERRED_SHADING
		case AMD_Deferred:
		{
			//clear G-Buffer textures
			gBuf->clearTextures();

			//1st pass
			DSgeometryPass();

			//2nd pass
			DSlightPass();

			//set texture for final output
			gBuf->bindForFinalPass(gBufTexIndex);
			glBlitFramebuffer(0, 0, resolution.x, resolution.y, 0, 0, resolution.x, resolution.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			//render G-buffer textures to main FBO
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			//if enabled render G-buffer textures to quads
			if (showMRTQuads)
			{
				showMRT();
			}
			
		}
		break;
		#pragma endregion DEFERRED_SHADING

		//tiled deferred shading
		#pragma region TILED_DEFERRED_SHADING
		case AMD_TiledDeferred:
		{
			//clear G-buffer textures
			gBuf->clearTextures();

			//1st pass
			DSgeometryPass();

			//start grid build timer 
			//gridTimer.start();
				std::vector<MinMax> tileDepthRanges;

				if (minMaxPass)
				{
					//glBeginQuery(GL_TIME_ELAPSED, minMaxDepthQuery);
					calcMinMaxDepth(tileDepthRanges);
					//glEndQuery(GL_TIME_ELAPSED);
					//glGetQueryObjectuiv(minMaxDepthQuery, GL_QUERY_RESULT_NO_WAIT, &minMaxDepthQueryTime);
				}

				//build light grid
				if (pointLights.size() > 0)
				{
					lightgrid.buildLightGrid(tileDepthRanges, pointLights, gCamera.nearPlane(), transformationMatrices.view, transformationMatrices.projection);
				}
			//gridTimer.stop();
			
			//bind G-Buffer
			glBindFramebuffer(GL_FRAMEBUFFER, gBuf->getFramebufferID());

			//bind Uniform buffers
			bindGridBuffers(lightgrid);

			//render light heat map/affected tiles/lighting
			if (showLightHeatMap)
			{
				renderQuad(lightHeatMapShader);
			}
			else if (showAffectedTiles)
			{
				renderQuad(affectedTilesShader);
			}
			else
			{
				//glBeginQuery(GL_TIME_ELAPSED, lightingQuery);

				//2nd pass
				TDSlightPass();

				//glEndQuery(GL_TIME_ELAPSED);
				//glGetQueryObjectuiv(lightingQuery, GL_QUERY_RESULT_NO_WAIT, &lightingQueryTime);
			}

			//unbind light's ID tex
			glActiveTexture(GL_TEXTURE0 + TDTB_LightIndex);
			glBindTexture(GL_TEXTURE_BUFFER, 0);

			//set texture for final output
			gBuf->bindForFinalPass(gBufTexIndex);
			glBlitFramebuffer(0, 0, resolution.x, resolution.y, 0, 0, resolution.x, resolution.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			//final pass
			showMRT();
		}
		break;
		#pragma endregion TILED_DEFERRED_SHADING

		//tiled forward shading
		#pragma region TILED_FORWARD_SHADING
		case AMD_TiledForward:
		{
			glEnable(GL_DEPTH_TEST);

			//do depth pre pass
			depthPrePass();

			//gridTimer.start();
				std::vector<MinMax> tileDepthRanges;

				//depth optimization
				if (minMaxPass)
				{
					//glBeginQuery(GL_TIME_ELAPSED, minMaxDepthQuery);
					
					calcMinMaxDepth(tileDepthRanges);
					
					//glEndQuery(GL_TIME_ELAPSED);
					//glGetQueryObjectuiv(minMaxDepthQuery, GL_QUERY_RESULT_NO_WAIT, &minMaxDepthQueryTime);
				}

				//build lightgrid
				if (pointLights.size() > 0)
				{
					lightgrid.buildLightGrid(tileDepthRanges, pointLights, gCamera.nearPlane(), transformationMatrices.view, transformationMatrices.projection);
				}
			//gridTimer.stop();

			//Bind forward FBO
			glBindFramebuffer(GL_FRAMEBUFFER, forwardFbo);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glViewport(0, 0, resolution.x, resolution.y);
			
			//Bind uniform buffers
			bindGridBuffers(lightgrid);

			if (showLightHeatMap)
			{
				renderQuad(lightHeatMapShader);
			}
			else if (showAffectedTiles)
			{
				renderQuad(affectedTilesShader);
			}
			else
			{
				//glBeginQuery(GL_TIME_ELAPSED, lightingQuery);
				
				//render scene
				tiledForwardShader->use();
					tiledForwardShader->setUniform("viewProjection", transformationMatrices.viewProjection);
					tiledForwardShader->setUniform("view", transformationMatrices.view);
					tiledForwardShader->setUniform("normalMatrix", transformationMatrices.normal);

					m_pMesh->Render(tiledForwardShader->object());
				tiledForwardShader->stopUsing();

				//glEndQuery(GL_TIME_ELAPSED);
				//glGetQueryObjectuiv(lightingQuery, GL_QUERY_RESULT_NO_WAIT, &lightingQueryTime);
			}

			//unbind light's ID tex
			glActiveTexture(GL_TEXTURE0 + TDTB_LightIndex);
			glBindTexture(GL_TEXTURE_BUFFER, 0);

			//set read/write FBOs
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, forwardFbo);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, resolution.x, resolution.y, 0, 0, resolution.x, resolution.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			
			//final pass
			showMRT();

			glDisable(GL_DEPTH_TEST);
		}
		break;
		#pragma endregion TILED_FORWARD_SHADING

		default:
			break;
	}

	//Draw bounding quad
	if (showBoundingQuads)
	{
		lightgrid.showLightQuads();
	}

	//Draw AntTweakBar
	TwDraw();

	if (GPUVendor == AMD)
		calcFPS(win, 0.1, AMDtechniqueNames[technique]);
	else
		calcFPS(win, 0.1, NVIDIAtechniqueNames[technique]);

	//calculate frames per second and writes it to window title once pers 0.5s
	/*lightingTime.push_back(lightingQueryTime/1000000);
	minmaxTime.push_back(minMaxDepthQueryTime / 1000000);
	gridTime.push_back(float(gridTimer.getElapsedTime() * 1000.0));*/

	//watch events
	glfwPollEvents();

    //swap front and back buffers
    glfwSwapBuffers(win);
}


/// <summary>
/// Update the scene based on the time elapsed since last update, 
/// mostly used for movement in the scene, mouse and camera handle.
/// </summary>
/// <param name="time">Seconds elapsed since last invocation.</param>
void update(float time)
{

	//gCamera.lookAt(glm::vec3(1139.06, 228.744, -41.1216));

    //move camera forward/backward
    if(glfwGetKey(win,'S'))
	{
        gCamera.move(time * moveSpeed * -gCamera.forward());
    }
	else if(glfwGetKey(win,'W'))
	{
        gCamera.move(time * moveSpeed * gCamera.forward());
    }

	//move camera left/right
    if(glfwGetKey(win,'A'))
	{
        gCamera.move(time * moveSpeed * -gCamera.right());
    }
	else if(glfwGetKey(win,'D'))
	{
        gCamera.move(time * moveSpeed * gCamera.right());
    }

	//move camera up/down (y axis)
    if(glfwGetKey(win,'Z'))
	{
        gCamera.move(time * moveSpeed * -gCamera.up());
    }
	else if(glfwGetKey(win,'X'))
	{
		gCamera.move(time * moveSpeed * gCamera.up());
    }
	
    double mouseX, mouseY;

    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		glfwGetCursorPos(win, &mouseX, &mouseY);
		gCamera.angleOrientation(MOUSE_SENSITIVITY * (mouseY - mouseYold), MOUSE_SENSITIVITY * (mouseX - mouseXold));

		//reset the mouse, so it doesn't go out of the window
		glfwSetCursorPos(win, mouseXold, mouseYold); 
	}
	else
	{
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		//handle mouse moves, set camera
		glfwGetCursorPos(win, &mouseXold, &mouseYold);
	}

	for (unsigned i = 0; i < GBuffer::GBUFFER_NUM_TEXTURES; i++)
	{
		if (showGBufferQuad[i])
		{
			gBufTexIndex = i;
		}
		else
		{
			showGBufferQuad[i] = false;
		}
	}
}


/// <summary>
/// Initialization of AntTweakBar.
/// </summary>
static void antTweakBarInit()
{
	//initialize ATB
	TwInit(TW_OPENGL, NULL);

	//set callback functions
	glfwSetMouseButtonCallback(win, MouseButtonCB);
	glfwSetCursorPosCallback(win, MousePosCB);

	//create context
	TwWindowSize(resolution.x, resolution.y);

	TwDefine(" GLOBAL contained=true ");

	//basic options
	TwBar *bar;
	bar = TwNewBar("TweakBar");
	TwDefine(" TweakBar size='200 140' ");
	TwDefine(" TweakBar resizable = true ");
	TwDefine(" TweakBar movable = true ");
	TwDefine(" TweakBar position = '20 20' ");
	TwDefine(" TweakBar valueswidth = 100 ");
	TwDefine(" TweakBar visible = true ");
	TwDefine(" TweakBar alpha = 0 ");

	TwAddVarRW(bar, "moveSpeed", TW_TYPE_FLOAT, &moveSpeed, " label='Speed' group='Camera' min=0.0 step=100.0 keyIncr=+ keyDecr=- help='Movement speed' ");
	TwAddVarRO(bar, "LIGHT_COUNT", TW_TYPE_UINT32, &LIGHT_COUNT, " group='Lights' label='Count' help='Shows light count in scene' ");
	
	//tiled shading settings
	TwBar *tiledBar;
	tiledBar = TwNewBar("tiled");
	TwDefine(" tiled label='Tiled Shading' ");
	TwDefine(" tiled size='200 180' ");
	TwDefine(" tiled resizable = true ");
	TwDefine(" tiled movable = true ");
	TwDefine(" tiled valueswidth = 30 ");
	TwDefine(" tiled position = '20 360' ");
	TwDefine(" tiled alpha = 0 ");

	TwAddVarRO(tiledBar, "tile_size.x", TW_TYPE_FLOAT, &tile_size.x, " group='Info' label='Tile dimension' ");
	TwAddVarRO(tiledBar, "grid_size.x", TW_TYPE_FLOAT, &grid_size.x, " group='Info' label='Grid dimension X' ");
	TwAddVarRO(tiledBar, "grid_size.y", TW_TYPE_FLOAT, &grid_size.y, " group='Info' label='Grid dimension Y' ");
	TwAddVarRW(tiledBar, "showBoundingQuads", TW_TYPE_BOOLCPP, &showBoundingQuads, " group='Lights' label='Show Bounding Quads' ");
	TwAddVarRW(tiledBar, "showAffectedTiles", TW_TYPE_BOOLCPP, &showAffectedTiles, " group='Lights' label='Show Affected Tiles' ");
	TwAddVarRW(tiledBar, "showLightHeatMap", TW_TYPE_BOOLCPP, &showLightHeatMap, " group='Lights' label='Show Heat Map' ");
	TwAddVarRW(tiledBar, "minMaxPass", TW_TYPE_BOOLCPP, &minMaxPass, " label='Depth Optimization' ");

	//gbuffer settings
	TwBar *GBufferBar;
	GBufferBar = TwNewBar("GBuffer");
	TwDefine(" GBuffer size='200 160' ");
	TwDefine(" GBuffer resizable = true ");
	TwDefine(" GBuffer movable = true ");
	TwDefine(" GBuffer valueswidth = 30 ");
	TwDefine(" GBuffer position = '20 540' ");
	TwDefine(" GBuffer alpha = 0 ");

	TwAddVarRW(GBufferBar, "showMRTQuads", TW_TYPE_BOOLCPP, &showMRTQuads, " label='Show G-Buffer' key=m help='Renders debug quads containing g-buffer textures' ");

	TwAddVarRW(GBufferBar, "showGBufferQuad[0]", TW_TYPE_BOOLCPP, &showGBufferQuad[0], " label='Diffuse' ");
	TwAddVarRW(GBufferBar, "showGBufferQuad[1]", TW_TYPE_BOOLCPP, &showGBufferQuad[1], " label='Normals' ");
	TwAddVarRW(GBufferBar, "showGBufferQuad[2]", TW_TYPE_BOOLCPP, &showGBufferQuad[2], " label='Positions' ");
	TwAddVarRW(GBufferBar, "showGBufferQuad[3]", TW_TYPE_BOOLCPP, &showGBufferQuad[3], " label='Specular' ");
	TwAddVarRW(GBufferBar, "showGBufferQuad[4]", TW_TYPE_BOOLCPP, &showGBufferQuad[4], " label='Ambient' ");
	TwAddVarRW(GBufferBar, "showDepth", TW_TYPE_BOOLCPP, &showDepth, " label='Depth' ");
}


/// <summary>
/// Binds the simple shader uniforms.
/// </summary>
/// <param name="shader">simple shader.</param>
static void bindSimpleUniforms(shprogram * shader)
{
	glm::vec3 KdGlobal = m_pMesh->getKd();

	//bind textures
	shader->use();
		GLuint diff_tex_id = glGetUniformLocation(shader->object(), "diff_tex");
		assert(diff_tex_id != -1);
		glUniform1i(diff_tex_id, 0);
	
		GLuint Kd_loc = glGetUniformLocation(shader->object(), "Kd");
		assert(Kd_loc != -1);
		glUniform3f(Kd_loc, KdGlobal.r, KdGlobal.g, KdGlobal.b);
	shader->stopUsing();

}


/// <summary>
/// Binds the deferred uniforms.
/// </summary>
/// <param name="shader">The shader.</param>
static void bindDeferredUniforms(shprogram * shader)
{
	glm::vec3 KdGlobal = m_pMesh->getKd();

	shader->use();
		GLuint diff_tex_id;
		GLuint normal_tex_id;
		GLuint spec_tex_id;
	
		diff_tex_id = glGetUniformLocation(shader->object(), "diff_tex");
		normal_tex_id = glGetUniformLocation(shader->object(), "normal_map");
		spec_tex_id = glGetUniformLocation(shader->object(), "spec_map");

		//point shader var to use texture unit "tex_unit"
		glUniform1i(diff_tex_id, 0);
		glUniform1i(normal_tex_id, 1);
		glUniform1i(spec_tex_id, 2);

		GLuint specularExponent_loc = glGetUniformLocation(shader->object(), "specExponent");
		glUniform1f(specularExponent_loc, m_pMesh->getSpecExponent());

		GLuint Kd_loc = glGetUniformLocation(shader->object(), "Kd");
		glUniform3f(Kd_loc, KdGlobal.r, KdGlobal.g, KdGlobal.b);
	shader->stopUsing();
}


/// <summary>
/// Binds g-buffer textures to uniforms for deferred shading.
/// </summary>
/// <param name="shader">deferred shader object.</param>
static void bindDeferredLightUniforms(shprogram * shader)
{
	//bind textures
	shader->use();
		shader->bindTexToUniform(DTB_Diffuse, gTexDiffuse, "texColor");
		shader->bindTexToUniform(DTB_Normal, gTexNormal, "texNormal");
		shader->bindTexToUniform(DTB_Position, gTexPos, "texPos");
		shader->bindTexToUniform(DTB_Specular, gTexSpec, "texSpec");
	shader->stopUsing();
}


/// <summary>
/// Binds g-buffer textures and light texture to uniforms for tiled deferred shading.
/// </summary>
/// <param name="shader">tiled deferred shader object.</param>
static void bindTiledDeferredLightUniforms(shprogram * shader)
{
	//bind textures
	shader->use();
	shader->bindTexToUniform(TDTB_Diffuse, gTexDiffuse, "texColor");
	shader->bindTexToUniform(TDTB_Normal, gTexNormal, "texNormal");
	shader->bindTexToUniform(TDTB_Position, gTexPos, "texPos");
	shader->bindTexToUniform(TDTB_Specular, gTexSpec, "texSpec");
	shader->bindTexToUniform(TDTB_LightIndex, lightIDtex, "texLightID");
	shader->stopUsing();

	//set uniform buffers
	shader->bindBufferToUniform(TBB_LightGrid, "lightGrid");
	shader->bindBufferToUniform(TBB_LightPosAndRadius, "lightPosAndRadius");
	shader->bindBufferToUniform(TBB_LightColors, "lightColors");
}


/// <summary>
/// Creates framebuffer for downsampling.
/// </summary>
static void createMinMaxFBO()
{
	glGenFramebuffers(1, &minMaxDepthFbo);

	glBindFramebuffer(GL_FRAMEBUFFER, minMaxDepthFbo);

	glGenTextures(1, &minMaxDepthTex);

	glBindTexture(GL_TEXTURE_2D, minMaxDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, grid_size.x, grid_size.y, 0, GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, minMaxDepthTex, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/// <summary>
/// Creates framebuffer for tiled forward shading.
/// </summary>
static void createForwardFBO()
{
	glGenFramebuffers(1, &forwardFbo);

	glBindFramebuffer(GL_FRAMEBUFFER, forwardFbo);

	glGenTextures(1, &forwardTex);

	glBindTexture(GL_TEXTURE_2D, forwardTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, forwardTex, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gTexDepth, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/// <summary>
/// Creates all shader programs.
/// </summary>
static void initShaders()
{
	std::ofstream shaderLog;
	shaderLog.open("logs/shaders.txt");

	std::cout << "Loading shaders";
	mrt = new shprogram("shaders/mrt_vert.glsl", "shaders/mrt_frag.glsl", shaderLog);							//geometry pass shader
	deferredShader = new shprogram("shaders/stencil_vert.glsl", "shaders/deferred_frag.glsl", shaderLog);		//light pass deferred shader
	quadShader = new shprogram("shaders/quad_vert.glsl", "shaders/quad_frag.glsl", shaderLog);					//g-buffer quad shader
	quadDShader = new shprogram("shaders/quad_depth_vert.glsl", "shaders/quad_depth_frag.glsl", shaderLog);		//g-buffer depth quad shader
	minMaxDepthShader = new shprogram("shaders/deferred_vert.glsl", "shaders/minmaxdepth.glsl", shaderLog);		//tiled deferred depth optimalization (depth min-max)
	simpleShader = new shprogram("shaders/simple_vert.glsl", "shaders/simple_frag.glsl", shaderLog);			//forward shading (no lighting)
	tiledDeferredShader = new shprogram("shaders/deferred_vert.glsl", "shaders/tiled_deferred_frag.glsl", shaderLog);	//tiled deferred shader
	tiledForwardShader = new shprogram("shaders/tiled_forward_vert.glsl", "shaders/tiled_forward_frag.glsl", shaderLog);	//tiled forward shader
	lightHeatMapShader = new shprogram("shaders/deferred_vert.glsl", "shaders/light_heat_map_frag.glsl", shaderLog);	//light heat map shader
	affectedTilesShader = new shprogram("shaders/deferred_vert.glsl", "shaders/affected_tiles_frag.glsl", shaderLog);	//affected tiles shader
	std::cout << ". success\n";

	shaderLog.close();
}

/// <summary>
/// Mains the specified argc.
/// </summary>
/// <param name="argc">number of parameters.</param>
/// <param name="argv">parameters as array of strings.</param>
/// <returns></returns>
int main(int argc, char **argv)
{
	//initialise GLFW3
	if (!glfwInit())
	{
		throw std::runtime_error("glfwInit failed");
	}

    //window presets
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);		//show title bar
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);		//fixed size window
	glfwWindowHint(GLFW_SAMPLES, 0);		        //multisampling (0 = OFF)
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  //API version to be compatible with [major]
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  //API version to be compatible with [minor]

    //create window and check state
    win = glfwCreateWindow((int)resolution.x, (int)resolution.y, "ECL", NULL, NULL);

    if(!win)
	{
        throw std::runtime_error("glfwCreateWindow failed");
    }

    //make window's cotext current
    glfwMakeContextCurrent(win);

	//set keyboard callback function
	glfwSetKeyCallback(win, keyCallback);

    //set cursor to default position and disable it
    glfwSetCursorPos(win, resolution.x/2, resolution.y/2);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    //initialise GLEW
    if(glewInit() != GLEW_OK)
	{
        throw std::runtime_error("glewInit failed");
    }

	//set GPU vendor based on actual graphics card vendor
	//used to eliminate deferred shading on NVIDIA cards because
	//of application failure for unknow reasons
	std::string vendor = (char *)glGetString(GL_VENDOR);
	std::string nvidia = "nvidia";

	int nvidiaf = findSubstringCI(vendor, nvidia);

	if (nvidiaf < 0)
		GPUVendor = AMD;
	else
		GPUVendor = NVIDIA;

	std::cout << vendor << std::endl;
	
	//enable deferred shading?
	if (FORCE_AMD_CONFIG == 1)
		GPUVendor = AMD;

	//setup VBO for quads
	createQuadVBO();

	//create shaders
	initShaders();

	//initialize G-Buffer
	gBuf->init(resolution.x, resolution.y);

	gTexDiffuse = gBuf->getTex(GBuffer::GBUFFER_TEX_DIFFUSE);
	gTexNormal = gBuf->getTex(GBuffer::GBUFFER_TEX_NORMAL);
	gTexPos = gBuf->getTex(GBuffer::GBUFFER_TEX_POSITION);
	gTexSpec = gBuf->getTex(GBuffer::GBUFFER_TEX_SPEC);
	gTexAmbient = gBuf->getTex(GBuffer::GBUFFER_TEX_AMBIENT);
	gTexDepth = gBuf->getDepthTex();

	//create new Mesh objects for sponza and pointlight and load them
	m_pMesh = new Mesh();
	m_sphere = new Mesh();

	m_pMesh->LoadMesh("data/models/crysponza/sponza.obj");
	m_sphere->LoadMesh("data/models/sphere/sphere.obj");

	//camera presets
	gCamera.setPosition(glm::vec3(116.294, 238.282, -18.8551));
	gCamera.lookAt(glm::vec3(1139.06, 228.744, -41.1216));
	gCamera.setNearPlane(1.0f);
	gCamera.setFarPlane(5000.0f);

	//generate lights
	generateLights(MAX_LIGHTS, LIGHT_RADIUS_MIN, LIGHT_RADIUS_MAX);

	//initialize grid buffers
	countsAndOffsetsBuffer.init(TILES_COUNT, 0);
	posAndRadiusesBuffer.init(MAX_LIGHTS);
	colorsBuffer.init(MAX_LIGHTS);
	lightIndicesBuffer.init(1);

	//generate texture for storing light IDs
	glGenTextures(1, &lightIDtex);
	glBindTexture(GL_TEXTURE_BUFFER, lightIDtex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, lightIndicesBuffer);

	//create FBOs
	createForwardFBO();
	createMinMaxFBO();

	//bind uniforms and textures to shaders
	bindSimpleUniforms(simpleShader);
	bindDeferredUniforms(mrt);
	bindDeferredUniforms(tiledForwardShader);
	bindDeferredLightUniforms(deferredShader);
	bindTiledDeferredLightUniforms(tiledDeferredShader);
	bindTiledDeferredLightUniforms(tiledForwardShader);

	affectedTilesShader->use();
	affectedTilesShader->bindTexToUniform(0, lightIDtex, "texLightID");
	affectedTilesShader->stopUsing();

	//set uniform buffers
	affectedTilesShader->bindBufferToUniform(TBB_LightGrid, "lightGrid");
	affectedTilesShader->bindBufferToUniform(TBB_LightColors, "lightColors");

	showGBufferQuad[gBufTexIndex] = true;

	//glGenQueries(1, &lightingQuery);
	//glGenQueries(1, &minMaxDepthQuery);


	//initialize antTweakBar
	antTweakBarInit();

    // run while the window is open
    double lastTime = glfwGetTime();
	start_time = lastTime;

	while(!glfwWindowShouldClose(win)){

        // update the scene based on the time elapsed since last update
        double thisTime = glfwGetTime();
        
		update(thisTime - lastTime);
        lastTime = thisTime;

        // draw one frame
        Render();
    }

	#pragma region TIMER_OUTPUTS
	/*std::ofstream myfile;
	myfile.open("GridBuildTime.txt");

	for (unsigned i = 0; i < gridTime.size(); i++)
	{
		myfile << gridTime[i] << std::endl;
	}
	
	myfile.close();
	
	std::ofstream myfile2;
	myfile2.open("fps.txt");

	for (unsigned i = 0; i < fpsVector.size(); i++)
	{
		myfile2 << fpsVector[i] << " " << std::endl;
	}

	myfile2.close();

	std::ofstream myfile3;
	myfile3.open("minMaxDepthQuery.txt");

	for (unsigned i = 0; i < minmaxTime.size(); i++)
	{
		myfile3 << minmaxTime[i] << " " << std::endl;
	}

	myfile3.close();

	std::ofstream myfile4;
	myfile4.open("TiledDeferredlightingQuery.txt");

	for (unsigned i = 0; i < lightingTime.size(); i++)
	{
		myfile4 << lightingTime[i] << " " << std::endl;
	}

	myfile4.close();*/
	#pragma endregion TIMER_OUTPUTS

	glfwTerminate();
	TwTerminate();
	
	//unbind tiled uniform buffers
	colorsBuffer.unbind();
	posAndRadiusesBuffer.unbind();
	countsAndOffsetsBuffer.unbind();
	lightIndicesBuffer.unbind();

    return 0;
}