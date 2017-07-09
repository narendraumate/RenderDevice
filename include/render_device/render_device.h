#pragma once

#include <cstddef> // for size_t

namespace render
{

// Function Type
enum FunctionType
{
	FUNCTIONTYPE_VERTEX,
	FUNCTIONTYPE_FRAGMENT
};

// Encapsulates a function
class Function
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~Function() {}

protected:

	// protected default constructor to ensure these are never created directly
	Function(const FunctionType& functionType, const char *code) {}
};

// Encapsulates a library
class Library
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~Library() {}

	// Create a shader from the supplied code; code is assumed to be GLSL for now.
	virtual Function *CreateFunction(const FunctionType& functionType, const char *code) = 0;

	// Destroy a shader
	virtual void DestroyFunction(Function *function) = 0;

protected:
	// protected default constructor to ensure these are never created directly
	Library(const char *vertexShaderSource, const char *fragmentShaderSource) {}
};

// Encapsulates a vertex buffer semantic description
class VertexDescriptor
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~VertexDescriptor() {}

protected:

	// protected default constructor to ensure these are never created directly
	VertexDescriptor() {}
};

// Encapsulates the render pipeline state (shaders + vertex descriptor + raster state)
class RenderPipelineState
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~RenderPipelineState() {}

protected:

	// protected default constructor to ensure these are never created
	// directly
	RenderPipelineState() {}
};

// Buffer Type
enum BufferType
{
	BUFFERTYPE_VERTEX,
	BUFFERTYPE_INDEX
};

// Encapsulates a buffer
class Buffer
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~Buffer() {}

protected:

	// protected default constructor to ensure these are never created directly
	Buffer(const BufferType& bufferType, long long size, const void *data) {}
 };

// Encapsulates a 2D texture
class Texture2D
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~Texture2D() {}

protected:

	// protected default constructor to ensure these are never created directly
	Texture2D() {}
};

// Describes a vertex attribute's type
enum VertexAttributeFormat
{
	VERTEXATTRIBUTEFORMAT_UNDEFINED = 0x00000000,
	VERTEXATTRIBUTEFORMAT_UINT8X2 = 0x00000001,
	VERTEXATTRIBUTEFORMAT_UINT8X4 = 0x00000002,
	VERTEXATTRIBUTEFORMAT_SINT8X2 = 0x00000003,
	VERTEXATTRIBUTEFORMAT_SINT8X4 = 0x00000004,
	VERTEXATTRIBUTEFORMAT_UNORM8X2 = 0x00000005,
	VERTEXATTRIBUTEFORMAT_UNORM8X4 = 0x00000006,
	VERTEXATTRIBUTEFORMAT_SNORM8X2 = 0x00000007,
	VERTEXATTRIBUTEFORMAT_SNORM8X4 = 0x00000008,
	VERTEXATTRIBUTEFORMAT_UINT16X2 = 0x00000009,
	VERTEXATTRIBUTEFORMAT_UINT16X4 = 0x0000000A,
	VERTEXATTRIBUTEFORMAT_SINT16X2 = 0x0000000B,
	VERTEXATTRIBUTEFORMAT_SINT16X4 = 0x0000000C,
	VERTEXATTRIBUTEFORMAT_UNORM16X2 = 0x0000000D,
	VERTEXATTRIBUTEFORMAT_UNORM16X4 = 0x0000000E,
	VERTEXATTRIBUTEFORMAT_SNORM16X2 = 0x0000000F,
	VERTEXATTRIBUTEFORMAT_SNORM16X4 = 0x00000010,
	VERTEXATTRIBUTEFORMAT_FLOAT16X2 = 0x00000011,
	VERTEXATTRIBUTEFORMAT_FLOAT16X4 = 0x00000012,
	VERTEXATTRIBUTEFORMAT_FLOAT32 = 0x00000013,
	VERTEXATTRIBUTEFORMAT_FLOAT32X2 = 0x00000014,
	VERTEXATTRIBUTEFORMAT_FLOAT32X3 = 0x00000015,
	VERTEXATTRIBUTEFORMAT_FLOAT32X4 = 0x00000016,
	VERTEXATTRIBUTEFORMAT_UINT32 = 0x00000017,
	VERTEXATTRIBUTEFORMAT_UINT32X2 = 0x00000018,
	VERTEXATTRIBUTEFORMAT_UINT32X3 = 0x00000019,
	VERTEXATTRIBUTEFORMAT_UINT32X4 = 0x0000001A,
	VERTEXATTRIBUTEFORMAT_SINT32 = 0x0000001B,
	VERTEXATTRIBUTEFORMAT_SINT32X2 = 0x0000001C,
	VERTEXATTRIBUTEFORMAT_SINT32X3 = 0x0000001D,
	VERTEXATTRIBUTEFORMAT_SINT32X4 = 0x0000001E,
	VERTEXATTRIBUTEFORMAT_FORCE32 = 0x7FFFFFFF
};

// Describes a vertex attribute within a vertex buffer
struct VertexAttribute
{
	VertexAttributeFormat format; //  type of vertex attribute and number of components
	long long offset; // offset where first occurrence of this vertex attribute resides in the buffer
	unsigned int shaderLocation; // location binding for vertex attribute // previously aka index
};

// Describes a vertex buffer layout
struct VertexBufferLayout
{
	unsigned int arrayStride;
	unsigned int attributeCount;
	VertexAttribute const * attributes;
};



enum Winding
{
	WINDING_CW = 0,
	WINDING_CCW,
	WINDING_MAX
};

enum Face
{
	FACE_FRONT = 0,
	FACE_BACK,
	FACE_FRONT_AND_BACK,
	FACE_MAX
};

enum RasterMode
{
	RASTERMODE_POINT = 0,
	RASTERMODE_LINE,
	RASTERMODE_FILL,
	RASTERMODE_MAX
};


enum Compare
{
	// Test comparison never passes
	COMPARE_NEVER = 0,

	// Test comparison passes if the incoming value is less than the stored value.
	COMPARE_LESS,

	// Test comparison passes if the incoming value is equal to the stored value.
	COMPARE_EQUAL,

	// Test comparison passes if the incoming value is less than or equal to the stored value.
	COMPARE_LEQUAL,

	// Test comparison passes if the incoming value is greater than the stored value.
	COMPARE_GREATER,

	// Test comparison passes if the incoming value is not equal to the stored value.
	COMPARE_NOTEQUAL,

	// Test comparison passes if the incoming value is greater than or equal to the stored value.
	COMPARE_GEQUAL,

	// Test comparison always passes.
	COMPARE_ALWAYS,

	COMPARE_MAX
};

enum StencilAction
{
	// Keeps the current value.
	STENCIL_KEEP = 0,

	// Sets the stencil buffer to zero.
	STENCIL_ZERO,

	// Sets the stencil buffer to the reference value masked with the write mask.
	STENCIL_REPLACE,

	// Increments the current stencil buffer value and clamps to maximum unsigned value.
	STENCIL_INCR,

	// Increments the current stencil buffer value and wraps the stencil buffer to zero when passing the maximum representable unsigned value.
	STENCIL_INCR_WRAP,

	// Decrements the current stencil buffer value and clamps to zero.
	STENCIL_DECR,

	// Decrements the current stencil buffer value and wraps the stencil buffer value to the maximum unsigned value.
	STENCIL_DECR_WRAP,

	// Bitwise invert of the current stencil buffer value.
	STENCIL_INVERT,

	STENCIL_MAX
};

// Primitive Type
enum PrimitiveType
{
	PRIMITIVETYPE_POINT = 0,
	PRIMITIVETYPE_LINE = 1,
	PRIMITIVETYPE_LINESTRIP = 2,
	PRIMITIVETYPE_TRIANGLE = 3,
	PRIMITIVETYPE_TRIANGLESTRIP = 4,
};

// Index Type
enum IndexType {
	INDEXTYPE_UINT16 = 0,
	INDEXTYPE_UINT32 = 1,
};

enum Filter {
	FILTER_NEAREST = 0,
	FILTER_LINEAR = 1
};

enum AddressMode {
	ADDRESS_CLAMPTOEDGE = 0,
	ADDRESS_REPEAT = 1,
	ADDRESS_MIRROREDREPEAT = 2
};

// Encapsulates the depth/stencil state
class DepthStencilState
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~DepthStencilState() {}

protected:

	// protected default constructor to ensure these are never created
	// directly
	DepthStencilState() {}
};

class SamplerState
{
public:
	virtual ~SamplerState() {}

protected:
	SamplerState() {}
};

class Drawable
{
public:
	virtual ~Drawable() = default;
	virtual Texture2D* GetTexture() = 0;
	virtual void GetSize(int& width, int& height) = 0;
};

// Forward declarations
class CommandBuffer;
class RenderCommandEncoder;
struct RenderPassDescriptor;

class CommandQueue
{
public:
	virtual ~CommandQueue() {}
	virtual CommandBuffer* CreateCommandBuffer() = 0;
protected:
	CommandQueue() {}
};

class CommandBuffer
{
public:
	virtual ~CommandBuffer() {}
	virtual RenderCommandEncoder* CreateRenderCommandEncoder(const RenderPassDescriptor& desc) = 0;
	virtual void Present(Drawable* drawable) = 0;
	virtual void Commit() = 0;
protected:
	CommandBuffer() {}
};

struct RenderPassDescriptor
{
	struct ColorAttachment
	{
		Texture2D* texture = nullptr;
		enum LoadAction { LoadAction_Clear, LoadAction_Load, LoadAction_DontCare };
		LoadAction loadAction = LoadAction_Clear;
		float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	};

	struct DepthAttachment
	{
		Texture2D* texture = nullptr;
		enum LoadAction { LoadAction_Clear, LoadAction_Load, LoadAction_DontCare };
		LoadAction loadAction = LoadAction_Clear;
		float clearDepth = 1.0f;
	};

	ColorAttachment colorAttachments[8]; // Support up to 8 color attachments
	int colorAttachmentCount = 1;
	DepthAttachment depthAttachment;
};

class RenderCommandEncoder
{
public:
	virtual ~RenderCommandEncoder() {}

	// Pipeline and state
	virtual void SetRenderPipelineState(RenderPipelineState* renderPipelineState) = 0;
	virtual void SetDepthStencilState(DepthStencilState* depthStencilState) = 0;

	// Resource binding
	virtual void SetVertexBuffer(Buffer* buffer, unsigned int offset, unsigned int index) = 0;
	virtual void SetTexture2D(Texture2D* texture, unsigned int index) = 0;
	virtual void SetSamplerState(SamplerState* sampler, unsigned int index) = 0;

	// Metal-style parameter setting (similar to setVertexBytes/setFragmentBytes)
	virtual void SetVertexBytes(const void* data, size_t size, unsigned int index) = 0;
	virtual void SetFragmentBytes(const void* data, size_t size, unsigned int index) = 0;

	// Drawing
	virtual void Draw(PrimitiveType primitiveType, unsigned int vertexStart, unsigned int vertexCount) = 0;
	virtual void DrawIndexed(PrimitiveType primitiveType, unsigned int indexCount, IndexType indexType, unsigned int indexOffset, unsigned int vertexOffset, Buffer* indexBuffer) = 0;

	// End the encoder
	virtual void EndEncoding() = 0;

	virtual void SetViewport(int x, int y, int width, int height) = 0;

protected:
	RenderCommandEncoder() {}
};

// Encapsulates the render device API.
class RenderDevice
{
public:

	// virtual destructor to ensure subclasses have a virtual destructor
	virtual ~RenderDevice() {}

	// Create a library from the supplied code; code is assumed to be GLSL for now.
	virtual Library *CreateLibrary(const char *vertexShaderSource, const char *fragmentShaderSource) = 0;

	// Destroy a library
	virtual void DestroyLibrary(Library *library) = 0;

	// Create a render pipeline state (Metal-style: combines shaders, vertex descriptor, and raster state)
	virtual RenderPipelineState *CreateRenderPipelineState(Function *vertexShader, Function *fragmentShader, VertexDescriptor *vertexDescriptor, bool cullEnabled = true, Winding frontFace = WINDING_CCW, Face cullFace = FACE_BACK, RasterMode rasterMode = RASTERMODE_FILL) = 0;

	// Destroy a render pipeline state
	virtual void DestroyRenderPipelineState(RenderPipelineState *renderPipelineState) = 0;

	// Create a buffer
	virtual Buffer *CreateBuffer(const render::BufferType& bufferType, long long size, const void *data = nullptr) = 0;

	// Destroy a buffer
	virtual void DestroyBuffer(Buffer *buffer) = 0;

	// Set a buffer
	virtual void SetBuffer(Buffer *buffer) = 0;

	// Create a vertex descriptor given a vertex buffer layout
	virtual VertexDescriptor *CreateVertexDescriptor(const VertexBufferLayout& vertexBufferLayout) = 0;

	// Destroy a vertex descriptor
	virtual void DestroyVertexDescriptor(VertexDescriptor *vertexDescriptor) = 0;

	// Create a 2D texture.
	//
	// data is assumed to consist of 32-bit pixel values where
	// 8 bits are used for each of the red, green, and blue
	// components, from lowest to highest byte order. The
	// most significant byte is ignored.
	virtual Texture2D *CreateTexture2D(int width, int height, const void *data = nullptr) = 0;

	// Destroy a 2D texture
	virtual void DestroyTexture2D(Texture2D *texture2D) = 0;

	// Set a 2D texture as active on a slot for subsequent draw commands
	virtual void SetTexture2D(unsigned int slot, Texture2D *texture2D) = 0;

	// Create a depth/stencil state.
	virtual DepthStencilState *CreateDepthStencilState(bool depthEnabled = true,
		bool depthWriteEnabled = true, float depthNear = 0, float depthFar = 1,
		Compare depthCompare = COMPARE_LESS, bool frontFaceStencilEnabled = false,
		Compare frontFaceStencilCompare = COMPARE_ALWAYS,
		StencilAction frontFaceStencilFail = STENCIL_KEEP,
		StencilAction frontFaceStencilPass = STENCIL_KEEP,
		StencilAction frontFaceDepthFail = STENCIL_KEEP,
		int frontFaceRef = 0, unsigned int frontFaceReadMask = 0xFFFFFFFF,
		unsigned int frontFaceWriteMask = 0xFFFFFFFF,
		bool backFaceStencilEnabled = false,
		Compare backFaceStencilCompare = COMPARE_ALWAYS,
		StencilAction backFaceStencilFail = STENCIL_KEEP,
		StencilAction backFaceStencilPass = STENCIL_KEEP,
		StencilAction backFaceDepthFail = STENCIL_KEEP,
		int backFaceRef = 0, unsigned int backFaceReadMask = 0xFFFFFFFF,
		unsigned int backFaceWriteMask = 0xFFFFFFFF) = 0;

	// Destroy a depth/stencil state.
	virtual void DestroyDepthStencilState(DepthStencilState *depthStencilState) = 0;

	// Set a depth/stencil state for subsequent draw commands
	virtual void SetDepthStencilState(DepthStencilState *depthStencilState) = 0;

	// todo
	virtual SamplerState *CreateSamplerState(Filter minFilter = FILTER_LINEAR,
		Filter magFilter = FILTER_LINEAR,
		AddressMode sAddressMode = ADDRESS_REPEAT,
		AddressMode tAddressMode = ADDRESS_REPEAT) = 0;

	// todo
	virtual void DestroySamplerState(SamplerState *sampler) = 0;

	// todo
	virtual void SetSamplerState(unsigned int slot, SamplerState *sampler) = 0;

	// Clear the default render target's color buffer, depth buffer, and stencil buffer to the specified values
	virtual void Clear(float red = 0.0f, float green = 0.0f, float blue = 0.0f, float alpha = 1.0f, float depth = 1.0f, int stencil = 0) = 0;

	// Draw a collection of primitives using the currently active shader pipeline and vertex array data
	virtual void Draw(const PrimitiveType& primitiveType, int offset, int count) = 0;

	// Draw a collection of primitives using the currently active shader pipeline, vertex array data, and index buffer
	virtual void DrawIndexed(const PrimitiveType& primitiveType, const IndexType& indexType, Buffer *indexBuffer, long long offset, int count) = 0;

	// Command queue creation
	virtual CommandQueue* CreateCommandQueue() = 0;
	virtual void DestroyCommandQueue(CommandQueue* queue) = 0;

	// Drawable creation (for presentation)
	virtual Drawable* GetNextDrawable() = 0;
	virtual void DestroyDrawable(Drawable* drawable) = 0;
};

// Creates a RenderDevice
RenderDevice *CreateRenderDevice();

// Destroys a RenderDevice
void DestroyRenderDevice(RenderDevice *renderDevice);

} // end namespace render
