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
This file implements G-Buffer used in deferred and tiled deferred techniques.
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <assert.h>

#include "buffers\g-buffer\gbuffer.h"

GBuffer::GBuffer()
{
    _fbo = 0;
    _depthTexture = 0;
}

GBuffer::~GBuffer(){
    
	//delete FBO
	if (_fbo != 0)
	{
        glDeleteFramebuffers(1, &_fbo);
    }

	//delete attached textures
	for (unsigned short i = GBUFFER_TEX_DIFFUSE; i < GBUFFER_NUM_TEXTURES; i++)
	{
		glDeleteTextures(GBUFFER_NUM_TEXTURES, _textures);
	}

	//delete depth texture
    if (_depthTexture != 0)
	{
        glDeleteTextures(1, &_depthTexture);
    }
}

/// <summary>
/// Generates texture that will be attached to G-buffer. Default format is set to GL_RGBA16F,
/// for all G-Buffer textures.
/// </summary>
/// <param name="width">texture width.</param>
/// <param name="height">texture height.</param>
/// <param name="i">texture attachment.</param>
/// <returns>texture identifier</returns>
GLuint GBuffer::genTexture(unsigned short width, unsigned short height, unsigned short i)
{
    GLuint tex;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tex, 0);

    return tex;
}

/// <summary>
/// Initialize G-Buffer by generating textures for albedo diffuse, normals, positions, albedo specular 
/// and depth.
/// </summary>
/// <param name="width">window width.</param>
/// <param name="height">window height.</param>
void GBuffer::init(unsigned short width, unsigned short height)
{
    //generate and bind new framebuffer
    glGenFramebuffers(1,&_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    //generate textures (diffuse,normal,position,specular, output);
    for(unsigned short i = GBUFFER_TEX_DIFFUSE; i < GBUFFER_NUM_TEXTURES; i++)
	{
        _textures[i] = genTexture(width, height, i);
    }

    //generate depth texture
    glGenTextures(1, &_depthTexture);
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _depthTexture);

    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT32F,width,height,0,GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,_depthTexture, 0);

    //always check that our framebuffer is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error("GBuffer initialization failed.");
    }

	//bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/// <summary>
/// Attaches textures to framebuffer color attachments and clears them.
/// </summary>
void GBuffer::clearTextures(){

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	
	for(unsigned short i = GBUFFER_TEX_DIFFUSE; i < GBUFFER_NUM_TEXTURES; i++)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);

		glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glClear(GL_DEPTH_BUFFER_BIT);
}

/// <summary>
/// Binds G-buffer for geometry pass of deferred shading, sets drawbuffers for 
/// Multiple Render Targets
/// </summary>
void GBuffer::bindForGeomPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //diffuse, normal, position
	GLenum drawBuffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, drawBuffers);
}

/// <summary>
/// Binds G-buffer for lighting pass of deferred shading.
/// </summary>
void GBuffer::bindForLightPass()
{
    //blend omni lights contributions to final texture
    glDrawBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_TEX_AMBIENT);

	//activate texture slots
	for (unsigned short i = GBUFFER_TEX_DIFFUSE; i < GBUFFER_NUM_TEXTURES; i++)
	{
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, _textures[GBUFFER_TEX_DIFFUSE + i]);
    }
}

/// <summary>
/// Binds for final pass, blits texture to default FBO.
/// </summary>
/// <param name="i">index of the g-buffer texture to blit.</param>
void GBuffer::bindForFinalPass(int i)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
}

/*
	Returns texture ID
*/
/// <summary>
/// G-buffer color texture accesor function.
/// </summary>
/// <param name="index">index to gbuffer textures specified by enum GBUFFER_TEXTURE_TYPE.</param>
/// <returns>color texture ID</returns>
GLuint GBuffer::getTex(GLuint index)
{
	assert(0 <= index && index < GBUFFER_NUM_TEXTURES);

	return _textures[index];
}

/// <summary>
/// G-buffer depth texture accesor function.
/// </summary>
/// <returns>depth texture ID</returns>
GLuint GBuffer::getDepthTex()
{
	return _depthTexture;
}

/// <summary>
/// Gets the framebuffer identifier.
/// </summary>
/// <returns></returns>
GLuint GBuffer::getFramebufferID()
{
	return _fbo;
}
