#include <render_device/platform.h>

#include <render_device/render_device.h>

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "image888.h"

const char *vertexShaderSource = "#version 410 core\n"
	"uniform mat4 uModel;\n"
	"uniform mat4 uView;\n"
	"uniform mat4 uProjection;\n"
	"layout (location = 0) in vec3 aPos;\n"
	"layout (location = 1) in vec2 aTexCoord;\n"
	"out vec2 FragTexCoord;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = uProjection * uView * uModel * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	"   FragTexCoord = aTexCoord;\n"
	"}";
const char *fragmentShaderSource = "#version 410 core\n"
	"uniform sampler2D uTextureSampler;\n"
	"in vec2 FragTexCoord;\n"
	"out vec4 FragColor;\n"
	"void main()\n"
	"{\n"
	"   FragColor = vec4(texture(uTextureSampler, FragTexCoord).rgb, 1);\n"
	"}\n";

struct Vertex
{
	float x, y, z;
	float u, v;
};

#define COUNT_OF(arr)	(sizeof(arr) / sizeof(*arr))

int main()
{
	platform::InitPlatform();

	platform::PLATFORM_WINDOW_REF window =
		platform::CreatePlatformWindow(800, 600, "Cube");
	if(!window)
	{
		platform::TerminatePlatform();
		return -1;
	}

	render::RenderDevice *renderDevice = render::CreateRenderDevice();

	// build and compile our shader program
	// ------------------------------------

	render::Library *library = renderDevice->CreateLibrary(vertexShaderSource, fragmentShaderSource);

	// vertex shader
	render::Function *vertexShader = library->CreateFunction(render::FUNCTIONTYPE_VERTEX, "main");

	// fragment shader
	render::Function *fragmentShader = library->CreateFunction(render::FUNCTIONTYPE_FRAGMENT, "main");

	render::VertexAttribute vertexAttributes[] = {
		{ .format = render::VERTEXATTRIBUTEFORMAT_FLOAT32X3, .offset = 0, .shaderLocation = 0 },
		{ .format = render::VERTEXATTRIBUTEFORMAT_FLOAT32X2, .offset = 12, .shaderLocation = 1 },
	};

	render::VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.arrayStride = sizeof(Vertex);
	vertexBufferLayout.attributeCount = COUNT_OF(vertexAttributes);
	vertexBufferLayout.attributes = vertexAttributes;

	render::VertexDescriptor *vertexDescriptor = renderDevice->CreateVertexDescriptor(vertexBufferLayout);

	render::Pipeline *pipeline = renderDevice->CreatePipeline(vertexShader, fragmentShader, vertexDescriptor);

	library->DestroyFunction(vertexShader);
	library->DestroyFunction(fragmentShader);
	renderDevice->DestroyLibrary(library);

	renderDevice->DestroyVertexDescriptor(vertexDescriptor);

	// For Sampler2D objects, we bind integers representing the texture
	// slot number to use
	render::PipelineParam *param = pipeline->GetParam("uTextureSampler");
	if (param) {
		param->SetAsInt(0);
	}

	// Get shader parameter for model matrix; we will set it every frame
	render::PipelineParam *uModelParam = pipeline->GetParam("uModel");

	// Get shader parameter for view matrix; we will set it every frame
	render::PipelineParam *uViewParam = pipeline->GetParam("uView");

	// Get shader parameter for projection matrix; we will set it every frame
	render::PipelineParam *uProjectionParam = pipeline->GetParam("uProjection");

	// Our vertices now have 2D texture coordinates
	Vertex vertices[] = {
		// front
		{-0.5f, -0.5f,  0.5f, 0, 1},
		{ 0.5f, -0.5f,  0.5f, 1, 1},
		{ 0.5f,  0.5f,  0.5f, 1, 0},
		{-0.5f,  0.5f,  0.5f, 0, 0},
		// right
		{ 0.5f, -0.5f,  0.5f, 0, 1},
		{ 0.5f, -0.5f, -0.5f, 1, 1},
		{ 0.5f,  0.5f, -0.5f, 1, 0},
		{ 0.5f,  0.5f,  0.5f, 0, 0},
		// top
		{-0.5f,  0.5f,  0.5f, 0, 1},
		{ 0.5f,  0.5f,  0.5f, 1, 1},
		{ 0.5f,  0.5f, -0.5f, 1, 0},
		{-0.5f,  0.5f, -0.5f, 0, 0},
		// back
		{ 0.5f, -0.5f, -0.5f, 0, 1},
		{-0.5f, -0.5f, -0.5f, 1, 1},
		{-0.5f,  0.5f, -0.5f, 1, 0},
		{ 0.5f,  0.5f, -0.5f, 0, 0},
		// left
		{-0.5f, -0.5f, -0.5f, 0, 1},
		{-0.5f, -0.5f,  0.5f, 1, 1},
		{-0.5f,  0.5f,  0.5f, 1, 0},
		{-0.5f,  0.5f, -0.5f, 0, 0},
		// bottom
		{-0.5f, -0.5f,  0.5f, 0, 1},
		{-0.5f, -0.5f, -0.5f, 1, 1},
		{ 0.5f, -0.5f, -0.5f, 1, 0},
		{ 0.5f, -0.5f,  0.5f, 0, 0}
	};

	render::Buffer *vertexBuffer = renderDevice->CreateBuffer(render::BUFFERTYPE_VERTEX, sizeof(vertices), vertices);

	// Setup indices and create index buffer
	uint32_t indices[] = {
		0, 1, 2, 0, 2, 3,		// front
		4, 5, 6, 4, 6, 7,		// right
		8, 9, 10, 8, 10, 11,	// top
		12, 13, 14, 12, 14, 15,	// back
		16, 17, 18, 16, 18, 19,	// left
		20, 21, 22, 20, 22, 23,	// bottom
	};

	render::Buffer *indexBuffer = renderDevice->CreateBuffer(render::BUFFERTYPE_INDEX, sizeof(indices), indices);

	// create texture
	render::Texture2D *texture2D = renderDevice->CreateTexture2D(BMPWIDTH, BMPHEIGHT, image32);

	while(platform::PollPlatformWindow(window))
	{
		glm::mat4 model(glm::uninitialize), view(glm::uninitialize), projection(glm::uninitialize);
		platform::GetPlatformViewport(model, view, projection);

		uModelParam->SetAsMat4(glm::value_ptr(model));
		uViewParam->SetAsMat4(glm::value_ptr(view));
		uProjectionParam->SetAsMat4(glm::value_ptr(projection));

		renderDevice->Clear(0.5f, 0.5f, 0.5f);

		renderDevice->SetPipeline(pipeline);

		// Set the texture for slot 0
		renderDevice->SetTexture2D(0, texture2D);

		renderDevice->SetBuffer(vertexBuffer);
		renderDevice->DrawIndexed(render::PRIMITIVETYPE_TRIANGLE, render::INDEXTYPE_UINT32, indexBuffer, 0, COUNT_OF(indices));

		platform::PresentPlatformWindow(window);
	}

	renderDevice->DestroyTexture2D(texture2D);
	renderDevice->DestroyBuffer(indexBuffer);
	renderDevice->DestroyBuffer(vertexBuffer);
	renderDevice->DestroyPipeline(pipeline);

	platform::TerminatePlatform();

	return 0;
}
