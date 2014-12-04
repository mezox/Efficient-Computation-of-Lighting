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
G-Buffer definition.
*/


/// <summary>
/// G-buffer class, creates new FBO for deferred pass of deferred shading
/// to store normals, positions and material textures. Default format for
/// textures is set to GL_RBA16F as it satisfies our precision needs.
/// </summary>
class GBuffer
{
    public:
        
		enum GBUFFER_TEXTURE_TYPE {
            GBUFFER_TEX_DIFFUSE,
            GBUFFER_TEX_NORMAL,
            GBUFFER_TEX_POSITION,
			GBUFFER_TEX_SPEC,
			GBUFFER_TEX_AMBIENT,
            GBUFFER_NUM_TEXTURES
        };

        GBuffer();
        ~GBuffer();

        void init(unsigned short width, unsigned short height);

        void clearTextures();
        void bindForGeomPass();
        void bindForLightPass();
        void bindForFinalPass(int i);

		GLuint getTex(GLuint index);
		GLuint getDepthTex();
		GLuint getFramebufferID();
		
    protected:
		GLuint genTexture(unsigned short, unsigned short, unsigned short);

		GLuint _fbo;
        GLuint _textures[GBUFFER_NUM_TEXTURES];
        GLuint _depthTexture;
};