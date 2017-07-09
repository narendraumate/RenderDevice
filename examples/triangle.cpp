#include <cstdio>
#include <render_device/platform.h>

#include <render_device/render_device.h>

const char *vertexShaderSource = "#version 410 core\n"
	"layout (location = 0) in vec3 aPos;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	"}\0";
const char *fragmentShaderSource = "#version 410 core\n"
	"out vec4 FragColor;\n"
	"void main()\n"
	"{\n"
	"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
	"}\n\0";

struct Vertex
{
	float x, y, z;
};

#define COUNT_OF(arr)	(sizeof(arr) / sizeof(*arr))

int main()
{
	platform::InitPlatform();

	// window creation
	// --------------------
	platform::PLATFORM_WINDOW_REF window = platform::CreatePlatformWindow(800, 600, "Triangle");
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
	};

	render::VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.arrayStride = sizeof(Vertex);
	vertexBufferLayout.attributeCount = COUNT_OF(vertexAttributes);
	vertexBufferLayout.attributes = vertexAttributes;

	render::VertexDescriptor *vertexDescriptor = renderDevice->CreateVertexDescriptor(vertexBufferLayout);

	// Create render pipeline state (Metal-style: combines shaders, vertex descriptor, and raster state)
	render::RenderPipelineState *renderPipelineState = renderDevice->CreateRenderPipelineState(vertexShader, fragmentShader, vertexDescriptor, false, render::WINDING_CW);

	library->DestroyFunction(vertexShader);
	library->DestroyFunction(fragmentShader);
	renderDevice->DestroyLibrary(library);

	renderDevice->DestroyVertexDescriptor(vertexDescriptor);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	Vertex vertices[] = {
		{ -0.5f, +0.5f, 0.0f },
		{ -0.5f, -0.5f, 0.0f },
		{ +0.5f, +0.5f, 0.0f },
		{ +0.5f, -0.5f, 0.0f },
	};

	render::Buffer *vertexBuffer = renderDevice->CreateBuffer(render::BUFFERTYPE_VERTEX, sizeof(vertices), vertices);

	// render loop
	// -----------
	while(platform::PollPlatformWindow(window))
	{
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

		// Create render command encoder
		render::RenderCommandEncoder *encoder = commandBuffer->CreateRenderCommandEncoder(passDesc);

		// Record rendering commands
		encoder->SetRenderPipelineState(renderPipelineState);
		encoder->SetVertexBuffer(vertexBuffer, 0, 0);

		// Get viewport size from drawable
		int width, height;
		drawable->GetSize(width, height);
		encoder->SetViewport(0, 0, width, height);

		encoder->Draw(render::PRIMITIVETYPE_TRIANGLESTRIP, 0, 4);

		encoder->EndEncoding();

		// Present and commit
		commandBuffer->Present(drawable);
		commandBuffer->Commit();

		// Clean up drawable
		renderDevice->DestroyDrawable(drawable);
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	renderDevice->DestroyBuffer(vertexBuffer);
	renderDevice->DestroyRenderPipelineState(renderPipelineState);
	renderDevice->DestroyCommandQueue(commandQueue);

	platform::TerminatePlatform();

	return 0;
}
