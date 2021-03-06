#include "scene.h"
#include <core/log.h>

#include <graphics\Shader.h>
#include <graphics\Texture.h>
#include <graphics\Image.h>
#include "MyMaterials.h"

#include "teapot.h"
#include <graphics\Mesh.h>

graphics::Mesh* createTeapotMeshMap()
{
	graphics::IndexBuffer* ib = new graphics::IndexBuffer(TeapotData::indices, TeapotData::numIndices);

	graphics::VertexArray* va[] =
	{
		new graphics::VertexArrayImpl<slmath::vec3>(
		graphics::ATTRIB_POSITION, (slmath::vec3*)TeapotData::positions, TeapotData::numVertices),

		new graphics::VertexArrayImpl<slmath::vec3>(
		graphics::ATTRIB_NORMAL, (slmath::vec3*)TeapotData::normals, TeapotData::numVertices),

		new graphics::VertexArrayImpl<slmath::vec3>(
		graphics::ATTRIB_UV, (slmath::vec3*)TeapotData::texCoords, TeapotData::numVertices)

	};

	graphics::VertexBuffer* vb = new graphics::VertexBuffer(&va[0], sizeof(va) / sizeof(va[0]));

	return new graphics::Mesh(ib, vb);
}

class CubeMapScene : public Scene
{
public:
	CubeMapScene()
	{

		LOG("CubeMapScene construct");
		checkOpenGL();

		m_count = 0.0f;

		FRM_SHADER_ATTRIBUTE attributes[3] = {
			{ "g_vPositionOS", graphics::ATTRIB_POSITION },
			{ "g_vNormalOS", graphics::ATTRIB_NORMAL },
			{ "g_vTexCoordOS", graphics::ATTRIB_UV }
		};

		m_shader =
			new graphics::Shader("assets/cubeMap.vs", "assets/cubeMap.fs",
			attributes, sizeof(attributes) / sizeof(FRM_SHADER_ATTRIBUTE));

		SimpleMaterialWithTextureUniformsCube* simpleMaterialUniforms = new SimpleMaterialWithTextureUniformsCube(m_shader, &m_sharedValues);

		simpleMaterialUniforms->vAmbient = slmath::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		simpleMaterialUniforms->vDiffuse = slmath::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		simpleMaterialUniforms->vSpecular = slmath::vec4(1.0f, 1.0f, 1.0f, 50.0f);
		simpleMaterialUniforms->vGlossyness = slmath::vec4(0.8f);

		texture = new Texture2D();
		texture2 = new Texture2D();


		image = graphics::Image::loadFromTGA("assets/CheckerBoard.tga");
		texture->setData(image);

		simpleMaterialUniforms->diffuseMap = texture;
		checkOpenGL();

		image2 = graphics::Image::loadFromTGA("assets/CheckerBoardGlossyMap.tga");
		texture2->setData(image2);
		
		simpleMaterialUniforms->glossyMap = texture2;
		checkOpenGL();

		eastl::string name = "BedroomCubeMap";

		core::Ref<graphics::Image> cubeImageRefs[6];
		cubeImageRefs[0] = graphics::Image::loadFromTGA("assets/" + name + "_RT.tga");
		cubeImageRefs[1] = graphics::Image::loadFromTGA("assets/" + name + "_LF.tga");
		cubeImageRefs[2] = graphics::Image::loadFromTGA("assets/" + name + "_DN.tga");
		cubeImageRefs[3] = graphics::Image::loadFromTGA("assets/" + name + "_UP.tga");
		cubeImageRefs[4] = graphics::Image::loadFromTGA("assets/" + name + "_FR.tga");
		cubeImageRefs[5] = graphics::Image::loadFromTGA("assets/" + name + "_BK.tga");

		graphics::Image* cubeImages[6];
		for (int i = 0; i < 6; i++)
		{
			cubeImages[i] = cubeImageRefs[i].ptr();
		}

		core::Ref<graphics::TextureCube> cubeMap = new graphics::TextureCube();
		cubeMap->setData(cubeImages);

	
		simpleMaterialUniforms->cubeMap = cubeMap;
		checkOpenGL();


		m_material = simpleMaterialUniforms;
		checkOpenGL();
		m_mesh = createTeapotMeshMap();

		checkOpenGL();
	}


	virtual ~CubeMapScene()
	{
		glDeleteProgram(m_shader->getProgram());
		LOG("CubeMapScene destruct");
	}


	virtual void update(graphics::ESContext* esContext, float deltaTime)
	{
		m_count += deltaTime / 15.0f;
		m_totalTime += deltaTime;

		if (m_count > 1.0f)
			m_count = 0.0f;

		m_sharedValues.totalTime += deltaTime;

		float fAspect = (float)esContext->width / (float)esContext->height;
		m_matProjection = slmath::perspectiveFovRH(
			slmath::radians(45.0f),
			fAspect,
			0.1f,
			1000.0f);

		// Moving camera
		cameraX = 0.0f;
		cameraY = 0.7f;
		cameraZ = cos(m_totalTime) + 1.01f;


		// Moving light
		//lightX = 5 * cos(m_totalTime);
		//lightY = 5 * sin(m_totalTime);
		lightX = 0.0f;
		lightY = 1.0f;
		lightZ = 2.0f;

		m_matView = slmath::lookAtRH(
			slmath::vec3(cameraX, cameraY, cameraZ),
			slmath::vec3(0.0f, 0.0f, 0.0f),
			slmath::vec3(0.0f, 1.0f, 0.0f));

		m_matModel = slmath::rotationX(-3.1415f*0.5f);
		m_matModel = slmath::rotationY(0) * m_matModel;
		m_matModel = slmath::translation(slmath::vec3(0.0f, 0.0f, 0.0f)) * m_matModel;
		m_matModel = slmath::scaling(0.01f)*m_matModel;

		// sharedShaderValues stuff
		m_sharedValues.matModel = m_matModel;
		m_sharedValues.matView = m_matView;
		m_sharedValues.matProj = m_matProjection;

		slmath::mat4 matModelView = m_matView * m_matModel;
		slmath::mat4 matModelViewProj = m_matProjection * matModelView;
		slmath::mat4 matNormal = slmath::transpose(slmath::inverse(matModelView));

		m_sharedValues.matModelView = matModelView;
		m_sharedValues.matNormal = matNormal;
		m_sharedValues.matModelViewProj = matModelViewProj;

		m_sharedValues.lightPos = slmath::vec3(lightX, lightY, lightZ);
		m_sharedValues.camPos = slmath::vec3(cameraX, cameraY, cameraZ);
	}


	virtual void render(graphics::ESContext* esContext)
	{
		checkOpenGL();

		// Set the viewport
		glViewport(0, 0, esContext->width, esContext->height);
		checkOpenGL();

		// Clear the backbuffer and depth-buffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		checkOpenGL();

		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		checkOpenGL();

		m_material->bind();
		checkOpenGL();

		m_mesh->render();
		checkOpenGL();
	}
private:
	float m_count;
	float m_totalTime;

	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float cameraZ = 0.0f;

	float lightX = 0.0f;
	float lightY = 0.0f;
	float lightZ = 0.0f;

	core::Ref<graphics::Shader> m_shader;

	SharedShaderValues m_sharedValues;
	core::Ref<graphics::ShaderUniforms> m_material;

	slmath::mat4 m_matProjection;
	slmath::mat4 m_matView;
	slmath::mat4 m_matModel;

	core::Ref<Mesh> m_mesh;

	core::Ref<Texture2D> texture;
	core::Ref<Texture2D> texture2;
	core::Ref<Image> image;
	core::Ref<Image> image2;

};


