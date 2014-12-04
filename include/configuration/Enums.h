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
This file defines enums used in application
*/

/// <summary>
/// GPU vendors
/// </summary>
enum VENDOR
{
	AMD,
	NVIDIA,
	VENDOR_MAX,
};

/// <summary>
/// Rendering techniques in application, used for switching between them [AMD an other GPU]
/// </summary>
enum AMDRenderingTechnique
{
	AMD_Simple,
	AMD_TiledDeferred,
	AMD_TiledForward,
	AMD_Deferred,
	AMD_Max,
};

/// <summary>
/// Rendering techniques in application, used for switching between them [NVIDIA Cards]
/// </summary>
enum NVIDIARenderingTechnique
{
	NVIDIA_Simple,
	NVIDIA_TiledDeferred,
	NVIDIA_TiledForward,
	NVIDIA_Max,
};

/// <summary>
/// Binding slots for Deferred shading G-buffer textures
/// </summary>
enum deferredTextureBindings
{
	DTB_Diffuse,
	DTB_Normal,
	DTB_Position,
	DTB_Specular,
	DTB_Max,
};

/// <summary>
/// Binding slots for Tiled Deferred Shading Gbuffer and LightID textures
/// </summary>
enum tiledDeferredTextureBindings
{
	TDTB_Diffuse,
	TDTB_Normal,
	TDTB_Position,
	TDTB_Specular,
	TDTB_LightIndex,
	TDTB_Max,
};

/// <summary>
/// Binding slots for Tiled Shading Uniform Buffer Objects
/// </summary>
enum tiledBufferBindings
{
	TBB_LightGrid,
	TBB_LightPosAndRadius,
	TBB_LightColors,
	TBB_Max,
};