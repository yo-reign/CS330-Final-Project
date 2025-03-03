//////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;  // Initialize texture counter
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Define materials for my objects
	DefineObjectMaterials();

	// Set up lighting before loading objects and textures
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// --- Load Textures ---
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();

	// --- Load Textures ---
	// Load the wood texture to be used for the wooden table.
	CreateGLTexture("textures/wood_texture.jpg", "wood");
	CreateGLTexture("textures/black_wood_texture.jpg", "black_wood");
	CreateGLTexture("textures/black_brushed_metal_texture.jpg", "black_metal");
	CreateGLTexture("textures/snhu_one.jpg", "monitor_screen");
	CreateGLTexture("textures/white_texture.jpg", "white");

	BindGLTextures();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ---------- Render the Plane (Floor) ----------
	// Transform for the plane
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Use the wood material and texture for the plane
	SetShaderMaterial("woodMat"); // Material defined in DefineObjectMaterials()
	SetShaderTexture("wood");     // Texture loaded with tag "wood"

	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	// ---------------------------------------------

	// ---------- Render the Plane (Wall) ----------
	// Transform for the plane
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 15.0f, -15.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// TODO: Change wall material and texture
	SetShaderMaterial("woodMat"); // Material defined in DefineObjectMaterials()
	SetShaderTexture("wood");     // Texture loaded with tag "wood"

	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	// ---------------------------------------------

	// Render the desk
	RenderDesk();

	// Render the monitor
	RenderMonitor();

	// Render the keyboard
	RenderKeyboard();

	// Render the mouse
	RenderMouse();
}

void SceneManager::SetupSceneLights()
{
	// Enable custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// ------------------ Directional Light ------------------
	// Global directional light (simulating sunlight)
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.2f, -1.0f, -0.3f);
	// Lower ambient to soften overall brightness
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.1f, 0.1f, 0.1f);
	// Moderate diffuse light for direct illumination
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	// Slightly reduced specular highlights
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// ------------------ Point Light ------------------
	// A point light to fill in shadowed areas
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 12.0f, 0.0f);
	// Lower ambient contribution for the point light
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	// Reduced diffuse intensity for softer lighting
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.6f, 0.6f, 0.6f);
	// Reduced specular intensity
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Deactivate any additional point lights (assuming 5 total)
	for (int i = 1; i < 5; i++)
	{
		std::string uniformName = "pointLights[" + std::to_string(i) + "].bActive";
		m_pShaderManager->setBoolValue(uniformName.c_str(), false);
	}
}

void SceneManager::DefineObjectMaterials()
{
	// Material for the plane (using the wood texture)
	OBJECT_MATERIAL woodMat;
	// When using a texture for color, keep the diffuse at white
	woodMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	// A modest specular term for a subtle shine
	woodMat.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMat.shininess = 32.0f;
	woodMat.tag = "woodMat";
	m_objectMaterials.push_back(woodMat);

	// Material for the desk tabletop (black wood texture)
	OBJECT_MATERIAL blackWoodMat;
	blackWoodMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	blackWoodMat.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	blackWoodMat.shininess = 16.0f;
	blackWoodMat.tag = "blackWoodMat";
	m_objectMaterials.push_back(blackWoodMat);

	// Material for the desk legs (black metal texture)
	OBJECT_MATERIAL blackMetalMat;
	blackMetalMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	blackMetalMat.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	blackMetalMat.shininess = 64.0f;
	blackMetalMat.tag = "blackMetalMat";
	m_objectMaterials.push_back(blackMetalMat);

	// Material for the monitor screen (SNHU Webpage)
	OBJECT_MATERIAL monitorScreenMat;
	monitorScreenMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	monitorScreenMat.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	monitorScreenMat.shininess = 64.0f;
	monitorScreenMat.tag = "monitorScreenMat";
	m_objectMaterials.push_back(monitorScreenMat);

	// Material for keyboard keys and mouse button keys
	OBJECT_MATERIAL whiteMat;
	whiteMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	whiteMat.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	whiteMat.shininess = 1.0f;
	whiteMat.tag = "whiteMat";
	m_objectMaterials.push_back(whiteMat);
}

void SceneManager::RenderDesk() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ---------- Render the Desk Tabletop ----------
	// Transform for the tabletop
	scaleXYZ = glm::vec3(24.0f, 0.75f, 16.0f);
	positionXYZ = glm::vec3(0.0f, 10.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackWoodMat");  // Material defined for black wood
	SetShaderTexture("black_wood");       // Texture loaded with tag "black_wood"
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG ----------
	// Transform for the left leg
	scaleXYZ = glm::vec3(1.25f, 10.0f, 1.25f);
	positionXYZ = glm::vec3(-9.0f, 5.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the black metal material and texture for the leg
	SetShaderMaterial("blackMetalMat");   // Material defined for metal parts
	SetShaderTexture("black_metal");      // Texture loaded with tag "black_metal"
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG: Piece1 ----------
	scaleXYZ = glm::vec3(1.25f, 1.25f, 1.25f);
	positionXYZ = glm::vec3(-8.0f, 9.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the black metal material and texture for the leg
	SetShaderMaterial("blackMetalMat");   // Material defined for metal parts
	SetShaderTexture("black_metal");      // Texture loaded with tag "black_metal"
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG: Piece2 ----------
	scaleXYZ = glm::vec3(1.25f, 0.5f, 12.0f);
	positionXYZ = glm::vec3(-8.0f, 9.5f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the black metal material and texture for the leg
	SetShaderMaterial("blackMetalMat");   // Material defined for metal parts
	SetShaderTexture("black_metal");      // Texture loaded with tag "black_metal"
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG: Piece3 ----------
	scaleXYZ = glm::vec3(1.25f, 0.5f, 12.0f);
	positionXYZ = glm::vec3(-9.0f, 0.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the black metal material and texture for the leg
	SetShaderMaterial("blackMetalMat");   // Material defined for metal parts
	SetShaderTexture("black_metal");      // Texture loaded with tag "black_metal"
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - RIGHT LEG ----------
	scaleXYZ = glm::vec3(1.25f, 10.0f, 1.25f);
	positionXYZ = glm::vec3(9.0f, 5.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the same black metal material and texture for the right leg
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG: Piece1 ----------
	scaleXYZ = glm::vec3(1.25f, 1.25f, 1.25f);
	positionXYZ = glm::vec3(8.0f, 9.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the same black metal material and texture for the right leg
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG: Piece2 ----------
	scaleXYZ = glm::vec3(1.25f, 0.5f, 12.0f);
	positionXYZ = glm::vec3(8.0f, 9.5f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the same black metal material and texture for the right leg
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render Desk - LEFT LEG: Piece3 ----------
	scaleXYZ = glm::vec3(1.25f, 0.5f, 15.0f);
	positionXYZ = glm::vec3(9.0f, 0.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Use the same black metal material and texture for the right leg
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderMonitor() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ---------- Render the stand lower flat base ----------
	scaleXYZ = glm::vec3(2.0f, 0.25f, 2.0f);
	positionXYZ = glm::vec3(-5.0f, 10.5f, -6.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the stand upper base ----------
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.5f);
	positionXYZ = glm::vec3(-5.0f, 10.75f, -6.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.5f, 6.0f, 0.5f);
	positionXYZ = glm::vec3(-5.0f, 12.5f, -6.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -30.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// ---------- Render the stand upper flat base ----------
	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f);
	positionXYZ = glm::vec3(-1.5f, 17.0f, -5.5f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the monitor screen ----------
	scaleXYZ = glm::vec3(15.5f, 10.5f, 0.5f);
	positionXYZ = glm::vec3(0.0f, 19.0f, -5.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the monitor screen ----------
	scaleXYZ = glm::vec3(15.0f, 10.0f, 0.25f);
	positionXYZ = glm::vec3(0.0f, 19.0f, -4.85f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("monitorScreenMat");
	SetShaderTexture("monitor_screen");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderKeyboard() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ---------- Render the keyboard base ----------
	scaleXYZ = glm::vec3(10.0f, 0.25f, 4.0f);
	positionXYZ = glm::vec3(0.0f, 10.5f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the keyboard keys ----------
	for (int j = 0; j < 6; j++) {
		for (int i = 0; i < 17; i++) {
			scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);
			positionXYZ = glm::vec3(-4.0f + (i / 2.0f), 10.5f, -1.0f + (j / 2.0f));
			SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
			SetShaderMaterial("whiteMat"); // Material defined in DefineObjectMaterials()
			SetShaderTexture("white");
			SetTextureUVScale(1.0f, 1.0f);
			m_basicMeshes->DrawBoxMesh();
		}
	}
}

void SceneManager::RenderMouse() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ---------- Render the mouse base ----------
	scaleXYZ = glm::vec3(1.25f, 0.5f, 1.75f);
	positionXYZ = glm::vec3(7.0f, 10.5f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the mouse hand rest ----------
	scaleXYZ = glm::vec3(1.25f, 0.75f, 0.875f);
	positionXYZ = glm::vec3(7.0f, 10.5f, 0.45f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the mouse's primary button ----------
	scaleXYZ = glm::vec3(0.5f, 0.75f, 0.75f);
	positionXYZ = glm::vec3(6.7f, 10.5f, -0.45f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("whiteMat");
	SetShaderTexture("white");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the mouse's secondary button ----------
	scaleXYZ = glm::vec3(0.5f, 0.75f, 0.75f);
	positionXYZ = glm::vec3(7.3f, 10.5f, -0.45f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("whiteMat");
	SetShaderTexture("white");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ---------- Render the mouse's middle button ----------
	scaleXYZ = glm::vec3(0.05f, 1.00f, 0.6f);
	positionXYZ = glm::vec3(7.0f, 10.5f, -0.45f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("blackMetalMat");
	SetShaderTexture("black_metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}
