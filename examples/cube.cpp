#include <render_device/platform.h>

#include <render_device/render_device.h>

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "image888.h"

const char *vertexShaderSource = "#version 430 core\n"
	"layout(std140, binding = 0) uniform ModelBuffer {\n"
	"   mat4 uModel;\n"
	"};\n"
	"layout(std140, binding = 1) uniform ViewBuffer {\n"
	"   mat4 uView;\n"
	"};\n"
	"layout(std140, binding = 2) uniform ProjectionBuffer {\n"
	"   mat4 uProjection;\n"
	"};\n"
	"layout (location = 0) in vec3 aPos;\n"
	"layout (location = 1) in vec2 aTexCoord;\n"
	"out vec2 FragTexCoord;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = uProjection * uView * uModel * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	"   FragTexCoord = aTexCoord;\n"
	"}\n";
const char *fragmentShaderSource = "#version 430 core\n"
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

	// Create command queue
	render::CommandQueue *commandQueue = renderDevice->CreateCommandQueue();

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

	// Create render pipeline state (Metal-style: combines shaders, vertex descriptor, and raster state)
	render::RenderPipelineState *renderPipelineState = renderDevice->CreateRenderPipelineState(vertexShader, fragmentShader, vertexDescriptor, true, render::WINDING_CCW, render::FACE_BACK, render::RASTERMODE_FILL);

	// Create custom depth stencil state for proper depth testing (LEQUAL, clearDepth=1)
	render::DepthStencilState *depthStencilState = renderDevice->CreateDepthStencilState(true, true, 0.0f, 1.0f, render::COMPARE_LEQUAL);

	library->DestroyFunction(vertexShader);
	library->DestroyFunction(fragmentShader);
	renderDevice->DestroyLibrary(library);

	renderDevice->DestroyVertexDescriptor(vertexDescriptor);

	// Our vertices have 2D texture coordinates
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
                // front
                0, 1, 2, 0, 2, 3,

                // right
                4, 5, 6, 4, 6, 7,

                // top
                8, 9, 10, 8, 10, 11,

                // back
                12, 13, 14, 12, 14, 15,

                // left
                16, 17, 18, 16, 18, 19,

                // bottom
                20, 21, 22, 20, 22, 23
	};

	render::Buffer *indexBuffer = renderDevice->CreateBuffer(render::BUFFERTYPE_INDEX, sizeof(indices), indices);

	// create texture
	render::Texture2D *texture2D = renderDevice->CreateTexture2D(BMPWIDTH, BMPHEIGHT, image32);

	// create sampler state
	render::SamplerState *sampler = renderDevice->CreateSamplerState(render::FILTER_LINEAR, render::FILTER_LINEAR, render::ADDRESS_REPEAT, render::ADDRESS_REPEAT);

	while(platform::PollPlatformWindow(window))
	{
		glm::mat4 model(1.0f), view(1.0f), projection(1.0f);
		platform::GetPlatformViewport(model, view, projection);

		// Get next drawable
		render::Drawable *drawable = renderDevice->GetNextDrawable();

		// Create command buffer
		render::CommandBuffer *commandBuffer = commandQueue->CreateCommandBuffer();

		// Set up render pass descriptor
		render::RenderPassDescriptor passDesc;
		passDesc.colorAttachments[0].texture = drawable->GetTexture();
		passDesc.colorAttachments[0].loadAction = render::RenderPassDescriptor::ColorAttachment::LoadAction_Clear;
		passDesc.colorAttachments[0].clearColor[0] = 0.5f; // red
		passDesc.colorAttachments[0].clearColor[1] = 0.5f; // green
		passDesc.colorAttachments[0].clearColor[2] = 0.5f; // blue
		passDesc.colorAttachments[0].clearColor[3] = 1.0f; // alpha

		// Set up depth attachment for proper depth buffer clearing
		passDesc.depthAttachment.loadAction = render::RenderPassDescriptor::DepthAttachment::LoadAction_Clear;
		passDesc.depthAttachment.clearDepth = 1.0f; // Far plane for OpenGL's -1 to +1 depth range

		// Create render command encoder
		render::RenderCommandEncoder *encoder = commandBuffer->CreateRenderCommandEncoder(passDesc);

		// Record rendering commands
		encoder->SetRenderPipelineState(renderPipelineState);
		encoder->SetDepthStencilState(depthStencilState);
		encoder->SetVertexBuffer(vertexBuffer, 0, 0);
		encoder->SetTexture2D(texture2D, 0);
		encoder->SetSamplerState(sampler, 0);

		// Set matrices using Metal-style API
		encoder->SetVertexBytes(glm::value_ptr(model), sizeof(glm::mat4), 0);
		encoder->SetVertexBytes(glm::value_ptr(view), sizeof(glm::mat4), 1);
		encoder->SetVertexBytes(glm::value_ptr(projection), sizeof(glm::mat4), 2);

		// Get viewport size from drawable
		int width, height;
		drawable->GetSize(width, height);
		encoder->SetViewport(0, 0, width, height);

		encoder->DrawIndexed(render::PRIMITIVETYPE_TRIANGLE, COUNT_OF(indices), render::INDEXTYPE_UINT32, 0, 0, indexBuffer);

		encoder->EndEncoding();

		// Commit and present

		commandBuffer->Present(drawable);

		commandBuffer->Commit();

		// Clean up frame resources
		renderDevice->DestroyDrawable(drawable);
		delete commandBuffer;
		delete encoder;
	}

	renderDevice->DestroySamplerState(sampler);
	renderDevice->DestroyTexture2D(texture2D);
	renderDevice->DestroyBuffer(indexBuffer);
	renderDevice->DestroyBuffer(vertexBuffer);
	renderDevice->DestroyRenderPipelineState(renderPipelineState);
	renderDevice->DestroyDepthStencilState(depthStencilState);
	renderDevice->DestroyCommandQueue(commandQueue);

	platform::TerminatePlatform();

	return 0;
}
