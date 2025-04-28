///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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

void SceneManager::LoadSceneTextures()
{
		bool bReturn = false;

		bReturn = CreateGLTexture(
			"textures/Floor.jpg",
			"floor");

		bReturn = CreateGLTexture(
			"textures/Leg.jpg",
			"leg");

		bReturn = CreateGLTexture(
			"textures/Tabletop.jpg",
			"tabletop");

		bReturn = CreateGLTexture(
			"textures/Plate.jpg",
			"plate");

		bReturn = CreateGLTexture(
			"textures/Mug.jpg",
			"mug");

		

	BindGLTextures();
}

void SceneManager::DefineObjectMaterials() {
	OBJECT_MATERIAL gravelMaterial;

	gravelMaterial.diffuseColor = glm::vec3(0.502f, 0.502f, 0.502f);
	gravelMaterial.specularColor = glm::vec3(0.502f, 0.502f, 0.502f); //will project more of a grayish hue
	gravelMaterial.shininess = 20.0;
	gravelMaterial.tag = "gravel";
	m_objectMaterials.push_back(gravelMaterial);

	OBJECT_MATERIAL metalMaterial;

	metalMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	metalMaterial.specularColor = glm::vec3(0.78f, 0.78f, 0.78f); //projects more of a white-gray hue
	metalMaterial.shininess = 85.0; //determines the strength of the specular color
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL woodMaterial;

	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.25f, 0.24f);
	woodMaterial.specularColor = glm::vec3(0.66f, 0.26f, 0.18f); //should project more of a reddish brown hue
	woodMaterial.shininess = 80.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL porcelainMaterial;

	porcelainMaterial.diffuseColor = glm::vec3(0.96f, 0.96f, 0.96f);
	porcelainMaterial.specularColor = glm::vec3(0.78f, 0.78f, 0.78f);
	porcelainMaterial.shininess = 80.0;
	porcelainMaterial.tag = "porcelain";
	m_objectMaterials.push_back(porcelainMaterial);

	OBJECT_MATERIAL glassMaterial;

	glassMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.specularColor = glm::vec3(0.21f, 0.21f, 0.21f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);
}

void SceneManager::SetupSceneLights() {
	m_pShaderManager->setVec3Value("directionalLight.direction", -6.0f, 5.0f, 5.0f); //creates a light that lights up the entire scene in a bright but slightly dim light
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);


	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 15.0f, -8.0f); //creates a light that shines above the table
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.03f, 0.03f, 0.0f); //projects a constant dim yellow color
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.4f, 0.4f, 0.0f); //makes the light project yellow color
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 1.0f, 0.0f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);


	m_pShaderManager->setVec3Value("pointLights[1].position", 5.0f, 0.0f, 10.0f); //creates a light that shines to the right of the table
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.00f, 0.00f, 0.0f); 
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.2f, 0.2f, 0.0f); //makes the light project yellow color
	m_pShaderManager->setVec3Value("pointLights[1].specular", 1.0f, 1.0f, 0.0f); //makes the light appear brighter when coming in contact with an object
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);


	m_pShaderManager->setVec3Value("pointLights[2].position", -5.0f, 0.0f, 10.0f); //creates a light that shines to the left of the table
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.00f, 0.00f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.0f); //makes the light project yellow color
	m_pShaderManager->setVec3Value("pointLights[2].specular", 1.0f, 1.0f, 0.0f); //makes the light appear brighter when coming in contact with an object
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}



/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures(); //load the textures of the scene
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	DefineObjectMaterials();

	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("floor"); //sets the plane to be a metal texture
	// draw the mesh with transformation values
	SetShaderMaterial("gravel");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our table leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f; //rotates the table leg upright
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 1.5f, 3.0f); //places the table leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1); //sets the color to red
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal"); //sets the material to metal for lighting purposes
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our table leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f; //rotates the table leg upright
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 1.5f, 3.0f); //places the table leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1); //sets the color to red
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal"); //sets the material to metal for lighting purposes
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our table leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f; //rotates the table leg upright
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 1.5f, -3.0f); //places the table leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1); //sets the color to red
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal"); //sets the material to metal for lighting purposes
	m_basicMeshes->DrawBoxMesh();


	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our table leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f; //rotates the table leg upright
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 1.5f, -3.0f); //places the table leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1); //sets the color to red
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal"); //sets the material to metal for lighting purposes
	m_basicMeshes->DrawBoxMesh();


	scaleXYZ = glm::vec3(8.0f, 1.0f, 7.0f); //scales our tabletop
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f; 
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.5f, 0.0f); //places the tabletop

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("tabletop"); //applies a wood texture
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 1.0f, 2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 1.0f, 2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 5.0f, 2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 1.0f, -2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 1.0f, -2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 5.0f, -2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.0f, 0.3f, 0.3f); //scales our chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.5f, -2.0f); //places the chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.0f, 0.3f, 0.3f); //scales our chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.5f, 2.0f); //places the chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.5f, 0.7f, 0.5f); //scales our upper chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.9f, 3.5f, -2.0f); //places the upper chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.5f, 0.7f, 0.5f); //scales our upper chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.9f, 3.5f, 2.0f); //places the upper chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.5f, 0.7f, 3.5f); //scales our chair top
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 3.5f, 0.0f); //places the chair top

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("tabletop"); //applies a wood texture
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.7f, 0.5f); //scales our chair bar
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 4.5f, 0.0f); //places the chair bar

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.7f, 0.5f); //scales our chair bar
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 5.5f, 0.0f); //places the chair bar

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.7f, 0.5f); //scales our chair bar
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 6.5f, 0.0f); //places the chair bar

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 1.0f, 2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 1.0f, 2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 5.0f, 2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 1.0f, -2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 1.0f, -2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(5.0f, 0.7f, 0.5f); //scales our chair leg
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 5.0f, -2.0f); //places the chair leg

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.0f, 0.3f, 0.3f); //scales our chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 1.5f, -2.0f); //places the chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.0f, 0.3f, 0.3f); //scales our chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 1.5f, 2.0f); //places the chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.5f, 0.7f, 0.5f); //scales our upper chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.9f, 3.5f, -2.0f); //places the upper chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.5f, 0.7f, 0.5f); //scales our upper chair guard
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.9f, 3.5f, 2.0f); //places the upper chair guard

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(6.5f, 0.7f, 3.5f); //scales our chair top
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 3.5f, 0.0f); //places the chair top

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("tabletop"); //applies a wood texture
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.7f, 0.5f); //scales our chair bar
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 4.5f, 0.0f); //places the chair bar

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.7f, 0.5f); //scales our chair bar
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 5.5f, 0.0f); //places the chair bar

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(4.0f, 0.7f, 0.5f); //scales our chair bar
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 6.5f, 0.0f); //places the chair bar

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("leg"); //applies a metal texture
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.0f, -0.4f, 0.5f); //scales our plate
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 5.4f, 0.0f); //places the plate

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("plate");
	SetShaderMaterial("porcelain");
	m_basicMeshes->DrawTaperedCylinderMesh();

	scaleXYZ = glm::vec3(1.0f, -0.4f, 0.5f); //scales our plate
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 5.4f, 0.0f); //places the plate

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("plate");
	SetShaderMaterial("porcelain");
	m_basicMeshes->DrawTaperedCylinderMesh();

	scaleXYZ = glm::vec3(0.3f, 0.02f, 0.2f); //scales our liquid
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 5.68f, -1.0f); //places the liquid

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.3f, 0.7f, 0.2f); //scales our mug
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 5.0f, -1.0f); //places the bottom of the mug

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("mug");
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.3f, 0.02f, 0.2f); //scales our liquid
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 5.68f, -1.0f); //places the liquid

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.3f, 0.7f, 0.2f); //scales our mug
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 5.0f, -1.0f); //places the bottom of the mug

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 1, 1); //sets the color to blue
	SetShaderTexture("mug");
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.09f, 0.25f, 0.1f); //scales our mug handle
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.3f, 5.35f, -1.0f); //places the mug handle

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1); //sets the color to red
	SetShaderTexture("mug");
	SetShaderMaterial("glass");
	m_basicMeshes->DrawTorusMesh();

	scaleXYZ = glm::vec3(0.09f, 0.25f, 0.1f); //scales our mug handle
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 5.35f, -1.0f); //places the mug handle

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1); //sets the color to red
	SetShaderTexture("mug");
	SetShaderMaterial("glass");
	m_basicMeshes->DrawTorusMesh();

}
