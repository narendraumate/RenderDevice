#pragma once

#include "render_device/render_device.h"
#include <vector>
#include <map>
#include <functional>
#include <glad/gl.h>

namespace render
{

class OpenGLDepthStencilState;
class OpenGLSamplerState;
class OpenGLRenderPipelineState;
class OpenGLBuffer;
class OpenGLTexture2D;

class OpenGLLibrary : public Library
{
public:

	OpenGLLibrary(const char *vertexShaderSource, const char *fragmentShaderSource);

	~OpenGLLibrary();

	Function *CreateFunction(const FunctionType& functionType, const char *name);

	void DestroyFunction(Function *function);

private:

	char* m_vertexShaderSource = nullptr;
	char* m_fragmentShaderSource = nullptr;
};

class OpenGLRenderDevice : public RenderDevice
{
public:

	OpenGLRenderDevice();

	~OpenGLRenderDevice();

	Library *CreateLibrary(const char *vertexShaderSource, const char *fragmentShaderSource);

	void DestroyLibrary(Library *library);

	RenderPipelineState *CreateRenderPipelineState(Function *vertexShader, Function *fragmentShader, VertexDescriptor *vertexDescriptor, bool cullEnabled = true, Winding frontFace = WINDING_CCW, Face cullFace = FACE_BACK, RasterMode rasterMode = RASTERMODE_FILL) override;

	void DestroyRenderPipelineState(RenderPipelineState *renderPipelineState) override;

	Buffer *CreateBuffer(const render::BufferType& bufferType, long long size, const void *data = nullptr) override;

	void DestroyBuffer(Buffer *buffer) override;

	void SetBuffer(Buffer *buffer) override;

	VertexDescriptor *CreateVertexDescriptor(const VertexBufferLayout& vertexBufferLayout) override;

	void DestroyVertexDescriptor(VertexDescriptor *vertexDescriptor) override;

	Texture2D *CreateTexture2D(int width, int height, const void *data = nullptr) override;

	void DestroyTexture2D(Texture2D *texture2D) override;

	void SetTexture2D(unsigned int slot, Texture2D *texture2D) override;

	DepthStencilState *CreateDepthStencilState(bool depthEnabled = true,
		bool			depthWriteEnabled = true,
		float			depthNear = 0,
		float			depthFar = 1,
		Compare			depthCompare = COMPARE_LESS,

		bool			frontFaceStencilEnabled = false,
		Compare			frontFaceStencilCompare = COMPARE_ALWAYS,
		StencilAction	frontFaceStencilFail = STENCIL_KEEP,
		StencilAction	frontFaceStencilPass = STENCIL_KEEP,
		StencilAction	frontFaceDepthFail = STENCIL_KEEP,
		int				frontFaceRef = 0,
		unsigned int	frontFaceReadMask = 0xFFFFFFFF,
		unsigned int	frontFaceWriteMask = 0xFFFFFFFF,

		bool			backFaceStencilEnabled = false,
		Compare			backFaceStencilCompare = COMPARE_ALWAYS,
		StencilAction	backFaceStencilFail = STENCIL_KEEP,
		StencilAction	backFaceStencilPass = STENCIL_KEEP,
		StencilAction	backFaceDepthFail = STENCIL_KEEP,
		int				backFaceRef = 0,
		unsigned int	backFaceReadMask = 0xFFFFFFFF,
		unsigned int	backFaceWriteMask = 0xFFFFFFFF) override;

	void DestroyDepthStencilState(DepthStencilState *depthStencilState) override;

	void SetDepthStencilState(DepthStencilState *depthStencilState) override;

	SamplerState *CreateSamplerState(Filter minFilter = FILTER_LINEAR, Filter magFilter = FILTER_LINEAR, AddressMode sAddressMode = ADDRESS_REPEAT, AddressMode tAddressMode = ADDRESS_REPEAT) override;

	void DestroySamplerState(SamplerState *sampler) override;

	void SetSamplerState(unsigned int slot, SamplerState *sampler) override;

	void Clear(float red = 0.0f, float green = 0.0f, float blue = 0.0f, float alpha = 1.0f, float depth = 1.0f, int stencil = 0) override;

	void Draw(const PrimitiveType& primitiveType, int offset, int count) override;

	void DrawIndexed(const PrimitiveType& primitiveType, const IndexType& indexType, Buffer *indexBuffer, long long offset, int count) override;

	// Command queue creation
	CommandQueue* CreateCommandQueue() override;
	void DestroyCommandQueue(CommandQueue* queue) override;

	// Drawable creation (for presentation)
	Drawable* GetNextDrawable() override;
	void DestroyDrawable(Drawable* drawable) override;

private:
	OpenGLRenderPipelineState *m_RenderPipelineState = nullptr;
	OpenGLDepthStencilState *m_DepthStencilState = nullptr;
	OpenGLBuffer *m_VertexBuffer = nullptr;
};

class OpenGLDrawable : public Drawable
{
public:
	OpenGLDrawable(OpenGLTexture2D* texture, void* window) : texture(texture), window(window) {}
	~OpenGLDrawable() override {}
	Texture2D* GetTexture() override;
	void GetSize(int& width, int& height) override;
	void* window; // Store the window pointer
private:
	OpenGLTexture2D* texture;
};

class OpenGLCommandQueue : public CommandQueue
{
public:
	OpenGLCommandQueue(OpenGLRenderDevice* device);
	~OpenGLCommandQueue() override;
	CommandBuffer* CreateCommandBuffer() override;
private:
	OpenGLRenderDevice* device;
};

class OpenGLCommandBuffer : public CommandBuffer
{
public:
	OpenGLCommandBuffer(OpenGLRenderDevice* device);
	~OpenGLCommandBuffer() override;
	RenderCommandEncoder* CreateRenderCommandEncoder(const RenderPassDescriptor& desc) override;
	void Present(Drawable* drawable) override;
	void Commit() override;
protected:
	OpenGLRenderDevice* device;
	std::vector<std::function<void()>> commands;
	bool hasRenderPass = false;

	friend class OpenGLRenderCommandEncoder;
};

class OpenGLRenderCommandEncoder : public RenderCommandEncoder
{
public:
	OpenGLRenderCommandEncoder(OpenGLCommandBuffer* commandBuffer, const RenderPassDescriptor& desc);
	~OpenGLRenderCommandEncoder() override;

	// Pipeline and state
	void SetRenderPipelineState(RenderPipelineState* renderPipelineState) override;
	void SetDepthStencilState(DepthStencilState* depthStencilState) override;

	// Resource binding
	void SetVertexBuffer(Buffer* buffer, unsigned int offset, unsigned int index) override;
	void SetTexture2D(Texture2D* texture, unsigned int index) override;
	void SetSamplerState(SamplerState* sampler, unsigned int index) override;

	// Metal-style parameter setting
	void SetVertexBytes(const void* data, size_t size, unsigned int index) override;
	void SetFragmentBytes(const void* data, size_t size, unsigned int index) override;

	// Drawing
	void Draw(PrimitiveType primitiveType, unsigned int vertexStart, unsigned int vertexCount) override;
	void DrawIndexed(PrimitiveType primitiveType, unsigned int indexCount, IndexType indexType, unsigned int indexOffset, unsigned int vertexOffset, Buffer* indexBuffer) override;

	// End the encoder
	void EndEncoding() override;

	void SetViewport(int x, int y, int width, int height) override;

private:
	OpenGLCommandBuffer* commandBuffer;
	RenderPassDescriptor renderPassDesc;
	std::vector<std::function<void()>> commands;

	// State tracking
	OpenGLRenderPipelineState* currentRenderPipelineState = nullptr;
	OpenGLDepthStencilState* currentDepthStencilState = nullptr;

	// Resource binding
	Buffer* currentVertexBuffer = nullptr;
	std::map<unsigned int, Texture2D*> boundTextures;
	std::map<unsigned int, SamplerState*> boundSamplers;
	std::map<unsigned int, std::pair<const void*, size_t>> vertexBytes;
	std::map<unsigned int, std::pair<const void*, size_t>> fragmentBytes;

	// Temporary UBOs for deferred deletion
	std::vector<GLuint> tempVertexUBOs;
	std::vector<GLuint> tempFragmentUBOs;
};

} // end namespace render
