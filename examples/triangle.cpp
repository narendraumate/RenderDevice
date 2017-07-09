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

	render::Pipeline *pipeline = renderDevice->CreatePipeline(vertexShader, fragmentShader, vertexDescriptor);

	library->DestroyFunction(vertexShader);
	library->DestroyFunction(fragmentShader);
	renderDevice->DestroyLibrary(library);

	renderDevice->DestroyVertexDescriptor(vertexDescriptor);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	Vertex verticesOne[] = {
		{ -0.50f, -0.5f, 0.0f }, // left
		{ 0.000f, -0.5f, 0.0f }, // right (shifted to the left)
		{ -0.25f, +0.5f, 0.0f }, // top (shifted to the left)
	};
	Vertex verticesTwo[] = {
		{ 0.000f, -0.5f, 0.0f }, // left (shifted to the right)
		{ +0.50f, -0.5f, 0.0f }, // right
		{ +0.25f, +0.5f, 0.0f }, // top (shifted to the right)
	};

	render::Buffer *vertexBufferOne = renderDevice->CreateBuffer(render::BUFFERTYPE_VERTEX, sizeof(verticesOne), verticesOne);
	render::Buffer *vertexBufferTwo = renderDevice->CreateBuffer(render::BUFFERTYPE_VERTEX, sizeof(verticesTwo), verticesTwo);

	// render loop
	// -----------
	while(platform::PollPlatformWindow(window))
	{
		// render
		// ------
		renderDevice->Clear(0.5f, 0.5f, 0.5f, 1.0f, 1.0f);

		renderDevice->SetPipeline(pipeline);

		// draw first triangle
		renderDevice->SetBuffer(vertexBufferOne);
		renderDevice->Draw(render::PRIMITIVETYPE_TRIANGLE, 0, 3);

		// draw second triangle
		renderDevice->SetBuffer(vertexBufferTwo);
		renderDevice->Draw(render::PRIMITIVETYPE_TRIANGLE, 0, 3);

		platform::PresentPlatformWindow(window);
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	renderDevice->DestroyBuffer(vertexBufferTwo);
	renderDevice->DestroyBuffer(vertexBufferOne);
	renderDevice->DestroyPipeline(pipeline);

	platform::TerminatePlatform();

	return 0;
}
