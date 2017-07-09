#include "ogl_render_device.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <functional>
#include <vector>
#include <map>

#include <iostream>

#include <cassert>

namespace render
{

class OpenGLFunction : public Function
{
public:

	OpenGLFunction(const FunctionType& functionType, const char *code)
	: Function(functionType, code)
	{
		// shader
		shader = glCreateShader(functionType == FUNCTIONTYPE_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
		glShaderSource(shader, 1, &code, NULL);
		glCompileShader(shader);

		// check for shader compile errors
		int success;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::"<< (functionType == FUNCTIONTYPE_VERTEX ? "VERTEX" : "FRAGMENT") << "::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
	}

	~OpenGLFunction() override
	{
		glDeleteShader(shader);
	}

	int shader = 0;
};

class OpenGLVertexDescriptor : public VertexDescriptor
{
public:

	struct OpenGLVertexAttribute
	{
		GLuint index;
		GLint size;
		GLenum type;
		GLboolean normalized;
		GLsizei stride;
		const GLvoid *pointer;
	};

	OpenGLVertexDescriptor(const VertexBufferLayout& vertexBufferLayout) : numVertexAttributes(vertexBufferLayout.attributeCount)
	{
		static GLenum toOpenGLType[] = { GL_BYTE, GL_SHORT, GL_INT, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT,
			GL_BYTE, GL_SHORT, GL_INT, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, GL_HALF_FLOAT, GL_FLOAT, GL_DOUBLE };
		static GLboolean toOpenGLNormalized[] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,
			GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE };

		// Describes a vertex attribute's type
		enum VertexAttributeType
		{
			VERTEXATTRIBUTETYPE_BYTE = 0,
			VERTEXATTRIBUTETYPE_SHORT,
			VERTEXATTRIBUTETYPE_INT,

			VERTEXATTRIBUTETYPE_UNSIGNED_BYTE,
			VERTEXATTRIBUTETYPE_UNSIGNED_SHORT,
			VERTEXATTRIBUTETYPE_UNSIGNED_INT,

			VERTEXATTRIBUTETYPE_BYTE_NORMALIZE,
			VERTEXATTRIBUTETYPE_SHORT_NORMALIZE,
			VERTEXATTRIBUTETYPE_INT_NORMALIZE,

			VERTEXATTRIBUTETYPE_UNSIGNED_BYTE_NORMALIZE,
			VERTEXATTRIBUTETYPE_UNSIGNED_SHORT_NORMALIZE,
			VERTEXATTRIBUTETYPE_UNSIGNED_INT_NORMALIZE,

			VERTEXATTRIBUTETYPE_HALF_FLOAT,
			VERTEXATTRIBUTETYPE_FLOAT,
			VERTEXATTRIBUTETYPE_DOUBLE
		};

		openGLVertexAttributes = new OpenGLVertexAttribute[numVertexAttributes];
		for(unsigned int i = 0; i < numVertexAttributes; i++)
		{
			VertexAttributeType type;
			int size;

			if (vertexBufferLayout.attributes[i].format == VERTEXATTRIBUTEFORMAT_FLOAT32X2) {
				type = VERTEXATTRIBUTETYPE_FLOAT;
				size = 2;
			} else if (vertexBufferLayout.attributes[i].format == VERTEXATTRIBUTEFORMAT_FLOAT32X3) {
				type = VERTEXATTRIBUTETYPE_FLOAT;
				size = 3;
			} else {
				std::cout << "UNSUPPORTED VERTEX ELEMENT" << std::endl;
			}

			openGLVertexAttributes[i].index = vertexBufferLayout.attributes[i].shaderLocation;
			openGLVertexAttributes[i].size = size;
			openGLVertexAttributes[i].type = toOpenGLType[type];
			openGLVertexAttributes[i].normalized = toOpenGLNormalized[type];
			openGLVertexAttributes[i].stride = vertexBufferLayout.arrayStride;
			openGLVertexAttributes[i].pointer = (char *)nullptr + vertexBufferLayout.attributes[i].offset;
		}
	}

	OpenGLVertexDescriptor(const OpenGLVertexDescriptor& other)
	{
		numVertexAttributes = other.numVertexAttributes;
		openGLVertexAttributes = new OpenGLVertexAttribute[numVertexAttributes];
		for (unsigned int i = 0; i < numVertexAttributes; i++)
		{
			openGLVertexAttributes[i] = other.openGLVertexAttributes[i];
		}
	}

	~OpenGLVertexDescriptor() override
	{
		delete[] openGLVertexAttributes;
		numVertexAttributes = 0;
	}

	unsigned int numVertexAttributes = 0;
	OpenGLVertexAttribute *openGLVertexAttributes = nullptr;
};

class OpenGLRenderPipelineState : public RenderPipelineState
{
public:

	OpenGLRenderPipelineState(OpenGLFunction *vertexFunction, OpenGLFunction *fragmentFunction, OpenGLVertexDescriptor *vertexDescriptor, bool cullEnabled = true, Winding frontFace = WINDING_CCW, Face cullFace = FACE_BACK, RasterMode rasterMode = RASTERMODE_FILL)
	{
		// link shaders
		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexFunction->shader);
		glAttachShader(shaderProgram, fragmentFunction->shader);
		glLinkProgram(shaderProgram);

		// check for linking errors
		int success;
		char infoLog[512];
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if(!success)
		{
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}

		glGenVertexArrays(1, &vertexArrayObject);

		this->vertexDescriptor = new OpenGLVertexDescriptor(*vertexDescriptor);

		// Store raster state parameters
		static const GLenum front_face_map[] = { GL_CW, GL_CCW };
		static const GLenum cull_face_map[] = { GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };
		static const GLenum raster_mode_map[] = { GL_POINT, GL_LINE, GL_FILL };

		this->cullEnabled = cullEnabled;
		this->frontFace = front_face_map[frontFace];
		this->cullFace = cull_face_map[cullFace];
		this->polygonMode = raster_mode_map[rasterMode];
	}

	~OpenGLRenderPipelineState() override
	{
		delete vertexDescriptor;

		glDeleteVertexArrays(1, &vertexArrayObject);

		glDeleteProgram(shaderProgram);
	}

	unsigned int shaderProgram = 0;
	unsigned int vertexArrayObject = 0;
	OpenGLVertexDescriptor *vertexDescriptor = nullptr;

	// Raster state parameters
	bool cullEnabled;
	GLenum frontFace;
	GLenum cullFace;
	GLenum polygonMode;
};

class OpenGLBuffer : public Buffer
{
public:

	OpenGLBuffer(const render::BufferType& bufferType, long long size, const void *data)
	: Buffer(bufferType, size, data)
	{
		glGenBuffers(1, &BO);
		glBindBuffer(bufferType == render::BUFFERTYPE_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, BO);
		glBufferData(bufferType == render::BUFFERTYPE_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); // always assuming static, for now
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	~OpenGLBuffer() override
	{
		glDeleteBuffers(1, &BO);
	}

	unsigned int BO = 0;
};

class OpenGLTexture2D : public Texture2D
{
public:

	OpenGLTexture2D(int width, int height, const void *data = nullptr)
	{
		this->width = width;
		this->height = height;
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	~OpenGLTexture2D() override
	{
		glDeleteTextures(1, &texture);
	}

	unsigned int texture = 0;
	int width = 0;
	int height = 0;
};

class OpenGLDepthStencilState : public DepthStencilState
{
public:

	OpenGLDepthStencilState(
		bool			_depthEnabled				= true,
		bool			_depthWriteEnabled			= true,
		float			_depthNear					= 0,
		float			_depthFar					= 1,
		Compare			_depthCompare				= COMPARE_LESS,

		bool			_frontFaceStencilEnabled	= false,
		Compare			_frontFaceStencilCompare	= COMPARE_ALWAYS,
		StencilAction	_frontFaceStencilFail		= STENCIL_KEEP,
		StencilAction	_frontFaceStencilPass		= STENCIL_KEEP,
		StencilAction	_frontFaceDepthFail			= STENCIL_KEEP,
		int				_frontFaceRef				= 0,
		unsigned int	_frontFaceReadMask			= 0xFFFFFFFF,
		unsigned int	_frontFaceWriteMask			= 0xFFFFFFFF,

		bool			_backFaceStencilEnabled		= false,
		Compare			_backFaceStencilCompare		= COMPARE_ALWAYS,
		StencilAction	_backFaceStencilFail		= STENCIL_KEEP,
		StencilAction	_backFaceStencilPass		= STENCIL_KEEP,
		StencilAction	_backFaceDepthFail			= STENCIL_KEEP,
		int				_backFaceRef				= 0,
		unsigned int	_backFaceReadMask			= 0xFFFFFFFF,
		unsigned int	_backFaceWriteMask			= 0xFFFFFFFF)

	{
		static const GLenum compare_map[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
		static const GLenum stencil_map[] = { GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_INCR_WRAP, GL_DECR, GL_DECR_WRAP, GL_INVERT };

		depthEnabled = _depthEnabled;
		depthWriteEnabled = _depthWriteEnabled;
		depthNear = _depthNear;
		depthFar = _depthFar;
		depthFunc = compare_map[_depthCompare];

		frontFaceStencilEnabled = _frontFaceStencilEnabled;
		frontStencilFunc = compare_map[_frontFaceStencilCompare];
		frontFaceStencilFail = stencil_map[_frontFaceStencilFail];
		frontFaceStencilPass = stencil_map[_frontFaceStencilPass];
		frontFaceDepthFail = stencil_map[_frontFaceDepthFail];
		frontFaceRef = _frontFaceRef;
		frontFaceReadMask = _frontFaceReadMask;
		frontFaceWriteMask = _frontFaceWriteMask;

		backFaceStencilEnabled = _backFaceStencilEnabled;
		backStencilFunc = compare_map[_backFaceStencilCompare];
		backFaceStencilFail = stencil_map[_backFaceStencilFail];
		backFaceStencilPass = stencil_map[_backFaceStencilPass];
		backFaceDepthFail = stencil_map[_backFaceDepthFail];
		backFaceRef = _backFaceRef;
		backFaceReadMask = _backFaceReadMask;
		backFaceWriteMask = _backFaceWriteMask;
	}

	~OpenGLDepthStencilState()
	{
		// Do nothing !!
	}

	bool depthEnabled;
	bool depthWriteEnabled;
	float depthNear;
	float depthFar;
	GLenum depthFunc;

	bool frontFaceStencilEnabled;
	GLenum	frontStencilFunc;
	GLenum frontFaceStencilFail;
	GLenum frontFaceStencilPass;
	GLenum frontFaceDepthFail;
	GLint frontFaceRef;
	GLuint frontFaceReadMask;
	GLuint frontFaceWriteMask;

	bool backFaceStencilEnabled;
	GLenum backStencilFunc;
	GLenum backFaceStencilFail;
	GLenum backFaceStencilPass;
	GLenum backFaceDepthFail;
	GLint backFaceRef;
	GLuint backFaceReadMask;
	GLuint backFaceWriteMask;
};

class OpenGLSamplerState : public SamplerState
{
public:

	OpenGLSamplerState(
		Filter _minFilter = render::FILTER_LINEAR,
		Filter _magFilter = render::FILTER_LINEAR,
		AddressMode _sAddressMode = render::ADDRESS_REPEAT,
		AddressMode _tAddressMode = render::ADDRESS_REPEAT)
	{
		glGenSamplers(1, &sampler);
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, _minFilter == FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, _magFilter == FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
		auto toGLAddress = [](AddressMode mode) {
			switch (mode) {
				case ADDRESS_CLAMPTOEDGE: return GL_CLAMP_TO_EDGE;
				case ADDRESS_REPEAT: return GL_REPEAT;
				case ADDRESS_MIRROREDREPEAT: return GL_MIRRORED_REPEAT;
			}
			return GL_REPEAT;
		};
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, toGLAddress(_sAddressMode));
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, toGLAddress(_tAddressMode));
	}

	~OpenGLSamplerState()
	{
		glDeleteSamplers(1, &sampler);
	}

	unsigned int sampler = 0;
};

OpenGLLibrary::OpenGLLibrary(const char *vertexShaderSource, const char *fragmentShaderSource)
: Library(vertexShaderSource, fragmentShaderSource)
{
	this->m_vertexShaderSource = new char[strlen(vertexShaderSource) + 1];
	this->m_fragmentShaderSource = new char[strlen(fragmentShaderSource) + 1];
	strcpy(this->m_vertexShaderSource, vertexShaderSource);
	strcpy(this->m_fragmentShaderSource, fragmentShaderSource);
}

OpenGLLibrary::~OpenGLLibrary()
{
	delete[] m_vertexShaderSource;
	delete[] m_fragmentShaderSource;
}

Function *OpenGLLibrary::CreateFunction(const FunctionType& functionType, const char *name)
{
	switch (functionType)
	{
		case FUNCTIONTYPE_VERTEX:
		{
			return new OpenGLFunction(functionType, m_vertexShaderSource);
		}
		break;
		case FUNCTIONTYPE_FRAGMENT:
		{
			return new OpenGLFunction(functionType, m_fragmentShaderSource);
		}
		break;
		default:
		{
			return nullptr;
		}
		break;
	}
}

void OpenGLLibrary::DestroyFunction(Function *function)
{
	delete function;
}

OpenGLRenderDevice::OpenGLRenderDevice()
{
}

OpenGLRenderDevice::~OpenGLRenderDevice()
{
	// Do nothing !!
}

Library *OpenGLRenderDevice::CreateLibrary(const char *vertexShaderSource, const char *fragmentShaderSource)
{
	return new OpenGLLibrary(vertexShaderSource, fragmentShaderSource);
}

void OpenGLRenderDevice::DestroyLibrary(Library *library)
{
	delete library;
}

RenderPipelineState *OpenGLRenderDevice::CreateRenderPipelineState(Function *vertexShader, Function *fragmentShader, VertexDescriptor *vertexDescriptor, bool cullEnabled, Winding frontFace, Face cullFace, RasterMode rasterMode)
{
	OpenGLFunction *oglVertexShader = static_cast<OpenGLFunction *>(vertexShader);
	OpenGLFunction *oglFragmentShader = static_cast<OpenGLFunction *>(fragmentShader);
	OpenGLVertexDescriptor *oglVertexDescriptor = static_cast<OpenGLVertexDescriptor *>(vertexDescriptor);

	return new OpenGLRenderPipelineState(oglVertexShader, oglFragmentShader, oglVertexDescriptor, cullEnabled, frontFace, cullFace, rasterMode);
}

void OpenGLRenderDevice::DestroyRenderPipelineState(RenderPipelineState *renderPipelineState)
{
	delete static_cast<OpenGLRenderPipelineState *>(renderPipelineState);
}

Buffer *OpenGLRenderDevice::CreateBuffer(const render::BufferType& bufferType, long long size, const void *data)
{
	return new OpenGLBuffer(bufferType, size, data);
}

void OpenGLRenderDevice::DestroyBuffer(Buffer *buffer)
{
	delete buffer;
}

void OpenGLRenderDevice::SetBuffer(Buffer *buffer)
{
	m_VertexBuffer = reinterpret_cast<OpenGLBuffer *>(buffer);
}

VertexDescriptor *OpenGLRenderDevice::CreateVertexDescriptor(const VertexBufferLayout& vertexBufferLayout)
{
	return new OpenGLVertexDescriptor(vertexBufferLayout);
}

void OpenGLRenderDevice::DestroyVertexDescriptor(VertexDescriptor *vertexDescriptor)
{
	delete vertexDescriptor;
}

Texture2D *OpenGLRenderDevice::CreateTexture2D(int width, int height, const void *data)
{
	return new OpenGLTexture2D(width, height, data);
}

void OpenGLRenderDevice::DestroyTexture2D(Texture2D *texture2D)
{
	delete texture2D;
}

void OpenGLRenderDevice::SetTexture2D(unsigned int slot, Texture2D *texture2D)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, texture2D ? reinterpret_cast<OpenGLTexture2D *>(texture2D)->texture : 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

DepthStencilState *OpenGLRenderDevice::CreateDepthStencilState(bool depthEnabled, bool depthWriteEnabled, float depthNear, float depthFar, Compare depthCompare,
		bool frontFaceStencilEnabled, Compare frontFaceStencilCompare, StencilAction frontFaceStencilFail, StencilAction frontFaceStencilPass,
		StencilAction frontFaceDepthFail, int frontFaceRef, unsigned int frontFaceReadMask, unsigned int frontFaceWriteMask, bool backFaceStencilEnabled,
		Compare backFaceStencilCompare, StencilAction backFaceStencilFail, StencilAction backFaceStencilPass, StencilAction backFaceDepthFail,
		int backFaceRef, unsigned int backFaceReadMask, unsigned int backFaceWriteMask)
{
	return new OpenGLDepthStencilState(depthEnabled, depthWriteEnabled, depthNear, depthFar, depthCompare, frontFaceStencilEnabled, frontFaceStencilCompare,
		frontFaceStencilFail, frontFaceStencilPass, frontFaceDepthFail, frontFaceRef, frontFaceReadMask, frontFaceWriteMask, backFaceStencilEnabled,
		backFaceStencilCompare, backFaceStencilFail, backFaceStencilPass, backFaceDepthFail, backFaceRef, backFaceReadMask, backFaceWriteMask);
}

void OpenGLRenderDevice::DestroyDepthStencilState(DepthStencilState *depthStencilState)
{
	delete depthStencilState;
}

void OpenGLRenderDevice::SetDepthStencilState(DepthStencilState *depthStencilState)
{
	DepthStencilState *oldDepthStencilState = m_DepthStencilState;

	m_DepthStencilState = dynamic_cast<OpenGLDepthStencilState *>(depthStencilState);

	if(m_DepthStencilState != oldDepthStencilState && m_DepthStencilState)
	{
		if(m_DepthStencilState->depthEnabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
		glDepthFunc(m_DepthStencilState->depthFunc);
		glDepthMask(m_DepthStencilState->depthWriteEnabled ? GL_TRUE : GL_FALSE);
		glDepthRange(m_DepthStencilState->depthNear, m_DepthStencilState->depthFar);

		if(m_DepthStencilState->frontFaceStencilEnabled || m_DepthStencilState->backFaceStencilEnabled)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);

		// front face
		glStencilFuncSeparate(GL_FRONT, m_DepthStencilState->frontStencilFunc, m_DepthStencilState->frontFaceRef, m_DepthStencilState->frontFaceReadMask);
		glStencilMaskSeparate(GL_FRONT, m_DepthStencilState->frontFaceWriteMask);
		glStencilOpSeparate(GL_FRONT, m_DepthStencilState->frontFaceStencilFail, m_DepthStencilState->frontFaceDepthFail, m_DepthStencilState->frontFaceStencilPass);

		// back face
		glStencilFuncSeparate(GL_BACK, m_DepthStencilState->backStencilFunc, m_DepthStencilState->backFaceRef, m_DepthStencilState->backFaceReadMask);
		glStencilMaskSeparate(GL_BACK, m_DepthStencilState->backFaceWriteMask);
		glStencilOpSeparate(GL_BACK, m_DepthStencilState->backFaceStencilFail, m_DepthStencilState->backFaceDepthFail, m_DepthStencilState->backFaceStencilPass);
	}
}

SamplerState *OpenGLRenderDevice::CreateSamplerState(Filter minFilter, Filter magFilter, AddressMode sAddressMode, AddressMode tAddressMode) {
	return new OpenGLSamplerState(minFilter, magFilter, sAddressMode, tAddressMode);
}

void OpenGLRenderDevice::DestroySamplerState(SamplerState *sampler) {
	delete static_cast<OpenGLSamplerState*>(sampler);
}

void OpenGLRenderDevice::SetSamplerState(unsigned int slot, SamplerState *sampler) {
	if (sampler)
		glBindSampler(slot, static_cast<OpenGLSamplerState*>(sampler)->sampler);
	else
		glBindSampler(slot, 0);
}

void OpenGLRenderDevice::Clear(float red, float green, float blue, float alpha, float depth, int stencil)
{
	glClearColor(red, green, blue, alpha);
	glClearDepth(depth);
	glClearStencil(stencil);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void OpenGLRenderDevice::Draw(const PrimitiveType& primitiveType, int offset, int count)
{
	GLenum mode;
	switch (primitiveType)
	{
		case PRIMITIVETYPE_POINT: {
			mode = GL_POINTS;
		} break;
		case PRIMITIVETYPE_LINE: {
			mode = GL_LINES;
		} break;
		case PRIMITIVETYPE_LINESTRIP: {
			mode = GL_LINE_STRIP;
		} break;
		case PRIMITIVETYPE_TRIANGLE: {
			mode = GL_TRIANGLES;
		} break;
		case PRIMITIVETYPE_TRIANGLESTRIP: {
			mode = GL_TRIANGLE_STRIP;
		} break;
		default: {
			assert(false);
		} break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, reinterpret_cast<OpenGLBuffer *>(m_VertexBuffer)->BO);

	if (m_RenderPipelineState) {
		glUseProgram(m_RenderPipelineState->shaderProgram);
		glBindVertexArray(m_RenderPipelineState->vertexArrayObject);
		OpenGLVertexDescriptor *vertexDescriptor = m_RenderPipelineState->vertexDescriptor;
		for(unsigned int j = 0; j < vertexDescriptor->numVertexAttributes; j++)
		{
			glEnableVertexAttribArray(vertexDescriptor->openGLVertexAttributes[j].index);
			glVertexAttribPointer(vertexDescriptor->openGLVertexAttributes[j].index,
								  vertexDescriptor->openGLVertexAttributes[j].size,
								  vertexDescriptor->openGLVertexAttributes[j].type,
								  vertexDescriptor->openGLVertexAttributes[j].normalized,
								  vertexDescriptor->openGLVertexAttributes[j].stride,
								  vertexDescriptor->openGLVertexAttributes[j].pointer);
		}
	}

	glDrawArrays(mode, offset, count);

	glBindVertexArray(0);
	glUseProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderDevice::DrawIndexed(const PrimitiveType& primitiveType, const IndexType& indexType, Buffer *indexBuffer, long long offset, int count)
{
	GLenum mode;
	switch (primitiveType)
	{
		case PRIMITIVETYPE_POINT: {
			mode = GL_POINTS;
		} break;
		case PRIMITIVETYPE_LINE: {
			mode = GL_LINES;
		} break;
		case PRIMITIVETYPE_LINESTRIP: {
			mode = GL_LINE_STRIP;
		} break;
		case PRIMITIVETYPE_TRIANGLE: {
			mode = GL_TRIANGLES;
		} break;
		case PRIMITIVETYPE_TRIANGLESTRIP: {
			mode = GL_TRIANGLE_STRIP;
		} break;
		default: {
			assert(false);
		} break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, reinterpret_cast<OpenGLBuffer *>(m_VertexBuffer)->BO);

	if (m_RenderPipelineState) {
		glUseProgram(m_RenderPipelineState->shaderProgram);
		glBindVertexArray(m_RenderPipelineState->vertexArrayObject);
		OpenGLVertexDescriptor *vertexDescriptor = m_RenderPipelineState->vertexDescriptor;
		for(unsigned int j = 0; j < vertexDescriptor->numVertexAttributes; j++)
		{
			glEnableVertexAttribArray(vertexDescriptor->openGLVertexAttributes[j].index);
			glVertexAttribPointer(vertexDescriptor->openGLVertexAttributes[j].index,
								  vertexDescriptor->openGLVertexAttributes[j].size,
								  vertexDescriptor->openGLVertexAttributes[j].type,
								  vertexDescriptor->openGLVertexAttributes[j].normalized,
								  vertexDescriptor->openGLVertexAttributes[j].stride,
								  vertexDescriptor->openGLVertexAttributes[j].pointer);
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast<OpenGLBuffer *>(indexBuffer)->BO);
	switch (indexType)
	{
		case INDEXTYPE_UINT16: {
			GLenum type = GL_UNSIGNED_SHORT;
			size_t offsetBytes = offset * sizeof(uint16_t);
			glDrawElementsBaseVertex(mode, count, type, reinterpret_cast<const void*>(offsetBytes), 0);
		}
		break;
		case INDEXTYPE_UINT32: {
			GLenum type = GL_UNSIGNED_INT;
			size_t offsetBytes = offset * sizeof(uint32_t);
			glDrawElementsBaseVertex(mode, count, type, reinterpret_cast<const void*>(offsetBytes), 0);
		}
		break;
		default: {
			assert(false);
		} break;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	glUseProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Texture2D* OpenGLDrawable::GetTexture() { return texture; }

OpenGLCommandQueue::OpenGLCommandQueue(OpenGLRenderDevice* device) : device(device) {}
OpenGLCommandQueue::~OpenGLCommandQueue() {}
CommandBuffer* OpenGLCommandQueue::CreateCommandBuffer() {
    return new OpenGLCommandBuffer(device);
}

OpenGLCommandBuffer::OpenGLCommandBuffer(OpenGLRenderDevice* device) : device(device) {}
OpenGLCommandBuffer::~OpenGLCommandBuffer() {}

RenderCommandEncoder* OpenGLCommandBuffer::CreateRenderCommandEncoder(const RenderPassDescriptor& desc) {
    hasRenderPass = true;
    return new OpenGLRenderCommandEncoder(this, desc);
}

void OpenGLCommandBuffer::Present(Drawable* drawable) {
    auto oglDrawable = static_cast<OpenGLDrawable*>(drawable);
    if (oglDrawable && oglDrawable->window) {
        // Ensure all OpenGL commands are finished before swapping
        glFinish();
        // Swap buffers without changing context
        glfwSwapBuffers(static_cast<GLFWwindow*>(oglDrawable->window));
    }
}

void OpenGLCommandBuffer::Commit() {
    // Execute all recorded commands
    for (auto& command : commands) {
        command();
    }
    commands.clear();
    hasRenderPass = false;
}

OpenGLRenderCommandEncoder::OpenGLRenderCommandEncoder(OpenGLCommandBuffer* commandBuffer, const RenderPassDescriptor& desc)
    : commandBuffer(commandBuffer), renderPassDesc(desc) {

    // Initialize member variables
    currentRenderPipelineState = nullptr;
    currentDepthStencilState = nullptr;
    currentVertexBuffer = nullptr;

    // Add commands to handle the render pass setup
    commands.push_back([this]() {
        // Bind to default framebuffer (0) for the window
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Handle color clear action
        if (renderPassDesc.colorAttachments[0].loadAction == RenderPassDescriptor::ColorAttachment::LoadAction_Clear) {
            glClearColor(
                renderPassDesc.colorAttachments[0].clearColor[0],
                renderPassDesc.colorAttachments[0].clearColor[1],
                renderPassDesc.colorAttachments[0].clearColor[2],
                renderPassDesc.colorAttachments[0].clearColor[3]
            );
        }

        // Handle depth clear action
        if (renderPassDesc.depthAttachment.loadAction == RenderPassDescriptor::DepthAttachment::LoadAction_Clear) {
            glClearDepth(renderPassDesc.depthAttachment.clearDepth);
        }

        // Perform the clear operation
        GLbitfield clearMask = 0;
        if (renderPassDesc.colorAttachments[0].loadAction == RenderPassDescriptor::ColorAttachment::LoadAction_Clear) {
            clearMask |= GL_COLOR_BUFFER_BIT;
        }
        if (renderPassDesc.depthAttachment.loadAction == RenderPassDescriptor::DepthAttachment::LoadAction_Clear) {
            clearMask |= GL_DEPTH_BUFFER_BIT;
        }

        if (clearMask != 0) {
            glClear(clearMask);
        }
    });
}

OpenGLRenderCommandEncoder::~OpenGLRenderCommandEncoder() {}

void OpenGLRenderCommandEncoder::SetRenderPipelineState(RenderPipelineState* renderPipelineState) {
	currentRenderPipelineState = static_cast<OpenGLRenderPipelineState*>(renderPipelineState);

	// Apply the pipeline state immediately
	if (currentRenderPipelineState) {
		glUseProgram(currentRenderPipelineState->shaderProgram);
		glBindVertexArray(currentRenderPipelineState->vertexArrayObject);

		// Apply raster state
		if (currentRenderPipelineState->cullEnabled) {
			glEnable(GL_CULL_FACE);
			glFrontFace(currentRenderPipelineState->frontFace);
			glCullFace(currentRenderPipelineState->cullFace);
		} else {
			glDisable(GL_CULL_FACE);
		}
		glPolygonMode(GL_FRONT_AND_BACK, currentRenderPipelineState->polygonMode);
	}
}

void OpenGLRenderCommandEncoder::SetDepthStencilState(DepthStencilState* depthStencilState) {
    currentDepthStencilState = static_cast<OpenGLDepthStencilState*>(depthStencilState);
    // Apply depth stencil state immediately
    if (currentDepthStencilState) {
        if (currentDepthStencilState->depthEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glDepthFunc(currentDepthStencilState->depthFunc);
        glDepthMask(currentDepthStencilState->depthWriteEnabled ? GL_TRUE : GL_FALSE);
        glDepthRange(currentDepthStencilState->depthNear, currentDepthStencilState->depthFar);

        if (currentDepthStencilState->frontFaceStencilEnabled || currentDepthStencilState->backFaceStencilEnabled)
            glEnable(GL_STENCIL_TEST);
        else
            glDisable(GL_STENCIL_TEST);

        // front face
        glStencilFuncSeparate(GL_FRONT, currentDepthStencilState->frontStencilFunc, currentDepthStencilState->frontFaceRef, currentDepthStencilState->frontFaceReadMask);
        glStencilMaskSeparate(GL_FRONT, currentDepthStencilState->frontFaceWriteMask);
        glStencilOpSeparate(GL_FRONT, currentDepthStencilState->frontFaceStencilFail, currentDepthStencilState->frontFaceDepthFail, currentDepthStencilState->frontFaceStencilPass);

        // back face
        glStencilFuncSeparate(GL_BACK, currentDepthStencilState->backStencilFunc, currentDepthStencilState->backFaceRef, currentDepthStencilState->backFaceReadMask);
        glStencilMaskSeparate(GL_BACK, currentDepthStencilState->backFaceWriteMask);
        glStencilOpSeparate(GL_BACK, currentDepthStencilState->backFaceStencilFail, currentDepthStencilState->backFaceDepthFail, currentDepthStencilState->backFaceStencilPass);
    }
}

void OpenGLRenderCommandEncoder::SetVertexBuffer(Buffer* buffer, unsigned int offset, unsigned int index) {
    currentVertexBuffer = buffer;
    // Don't execute OpenGL calls here - let the draw commands handle it
}

void OpenGLRenderCommandEncoder::SetTexture2D(Texture2D* texture, unsigned int index) {
    boundTextures[index] = texture;
    commands.push_back([texture, index]() {
        if (texture) {
            OpenGLTexture2D* oglTexture = static_cast<OpenGLTexture2D*>(texture);
            glActiveTexture(GL_TEXTURE0 + index);
            glBindTexture(GL_TEXTURE_2D, oglTexture->texture);
        } else {
            glActiveTexture(GL_TEXTURE0 + index);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    });
}

void OpenGLRenderCommandEncoder::SetSamplerState(SamplerState* sampler, unsigned int index) {
    boundSamplers[index] = sampler;
    commands.push_back([sampler, index]() {
        if (sampler) {
            OpenGLSamplerState* oglSampler = static_cast<OpenGLSamplerState*>(sampler);
            glBindSampler(index, oglSampler->sampler);
        } else {
            glBindSampler(index, 0);
        }
    });
}

void OpenGLRenderCommandEncoder::SetVertexBytes(const void* data, size_t size, unsigned int index) {
    vertexBytes[index] = std::make_pair(data, size);
    // For OpenGL, we'll handle this in the draw commands by creating a temporary buffer
    commands.push_back([this, data, size, index]() {
        if (currentRenderPipelineState) {
            glUseProgram(currentRenderPipelineState->shaderProgram);

            // Create a temporary uniform buffer for the data
            GLuint ubo;
            glGenBuffers(1, &ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);

            // Debug: Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                printf("OpenGL error in SetVertexBytes (index %d): %d\n", index, error);
            }

            // Do not delete the UBO immediately; store for deferred deletion
            tempVertexUBOs.push_back(ubo);
        }
    });
}

void OpenGLRenderCommandEncoder::SetFragmentBytes(const void* data, size_t size, unsigned int index) {
    fragmentBytes[index] = std::make_pair(data, size);
    // For OpenGL, we'll handle this in the draw commands by creating a temporary buffer
    commands.push_back([this, data, size, index]() {
        if (currentRenderPipelineState) {
            glUseProgram(currentRenderPipelineState->shaderProgram);

            // Create a temporary uniform buffer for the data
            GLuint ubo;
            glGenBuffers(1, &ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);

            // Debug: Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                printf("OpenGL error in SetVertexBytes (index %d): %d\n", index, error);
            }

            // Do not delete the UBO immediately; store for deferred deletion
            tempFragmentUBOs.push_back(ubo);
        }
    });
}

void OpenGLRenderCommandEncoder::Draw(PrimitiveType primitiveType, unsigned int vertexStart, unsigned int vertexCount) {
    commands.push_back([this, primitiveType, vertexStart, vertexCount]() {

        if (currentRenderPipelineState && currentVertexBuffer) {
            OpenGLVertexDescriptor* vertexDescriptor = currentRenderPipelineState->vertexDescriptor;
            OpenGLBuffer* oglBuffer = static_cast<OpenGLBuffer*>(currentVertexBuffer);

            // Set up pipeline and VAO
            glUseProgram(currentRenderPipelineState->shaderProgram);
            glBindVertexArray(currentRenderPipelineState->vertexArrayObject);

            // Bind vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, oglBuffer->BO);

            // Set up vertex attributes
            for(unsigned int j = 0; j < vertexDescriptor->numVertexAttributes; j++) {
                glEnableVertexAttribArray(vertexDescriptor->openGLVertexAttributes[j].index);
                glVertexAttribPointer(vertexDescriptor->openGLVertexAttributes[j].index,
                                      vertexDescriptor->openGLVertexAttributes[j].size,
                                      vertexDescriptor->openGLVertexAttributes[j].type,
                                      vertexDescriptor->openGLVertexAttributes[j].normalized,
                                      vertexDescriptor->openGLVertexAttributes[j].stride,
                                      vertexDescriptor->openGLVertexAttributes[j].pointer);
            }
        }

        GLenum mode;
        switch (primitiveType) {
            case PRIMITIVETYPE_POINT: mode = GL_POINTS; break;
            case PRIMITIVETYPE_LINE: mode = GL_LINES; break;
            case PRIMITIVETYPE_LINESTRIP: mode = GL_LINE_STRIP; break;
            case PRIMITIVETYPE_TRIANGLE: mode = GL_TRIANGLES; break;
            case PRIMITIVETYPE_TRIANGLESTRIP: mode = GL_TRIANGLE_STRIP; break;
            default: assert(false); break;
        }
        glDrawArrays(mode, vertexStart, vertexCount);

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            printf("OpenGL error in Draw: %d\n", error);
        }
    });
}

void OpenGLRenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, unsigned int indexCount, IndexType indexType, unsigned int indexOffset, unsigned int vertexOffset, Buffer* indexBuffer) {
    commands.push_back([this, primitiveType, indexCount, indexType, indexOffset, vertexOffset, indexBuffer]() {
        if (currentRenderPipelineState && currentVertexBuffer && indexBuffer) {
            OpenGLVertexDescriptor* vertexDescriptor = currentRenderPipelineState->vertexDescriptor;
            OpenGLBuffer* oglBuffer = static_cast<OpenGLBuffer*>(currentVertexBuffer);
            OpenGLBuffer* oglIndexBuffer = static_cast<OpenGLBuffer*>(indexBuffer);

            // Set up pipeline and VAO
            glUseProgram(currentRenderPipelineState->shaderProgram);
            glBindVertexArray(currentRenderPipelineState->vertexArrayObject);

            // Bind buffers
            glBindBuffer(GL_ARRAY_BUFFER, oglBuffer->BO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oglIndexBuffer->BO);

            // Set up vertex attributes
            for(unsigned int j = 0; j < vertexDescriptor->numVertexAttributes; j++) {
                glEnableVertexAttribArray(vertexDescriptor->openGLVertexAttributes[j].index);
                glVertexAttribPointer(vertexDescriptor->openGLVertexAttributes[j].index,
                                      vertexDescriptor->openGLVertexAttributes[j].size,
                                      vertexDescriptor->openGLVertexAttributes[j].type,
                                      vertexDescriptor->openGLVertexAttributes[j].normalized,
                                      vertexDescriptor->openGLVertexAttributes[j].stride,
                                      vertexDescriptor->openGLVertexAttributes[j].pointer);
            }
        }

        GLenum mode;
        switch (primitiveType) {
            case PRIMITIVETYPE_POINT: mode = GL_POINTS; break;
            case PRIMITIVETYPE_LINE: mode = GL_LINES; break;
            case PRIMITIVETYPE_LINESTRIP: mode = GL_LINE_STRIP; break;
            case PRIMITIVETYPE_TRIANGLE: mode = GL_TRIANGLES; break;
            case PRIMITIVETYPE_TRIANGLESTRIP: mode = GL_TRIANGLE_STRIP; break;
            default: assert(false); break;
        }

        GLenum type = (indexType == INDEXTYPE_UINT16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        size_t offsetBytes = indexOffset * (indexType == INDEXTYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
        glDrawElementsBaseVertex(mode, indexCount, type, reinterpret_cast<const void*>(offsetBytes), vertexOffset);

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            printf("OpenGL error in DrawIndexed: %d\n", error);
        }
    });
}

void OpenGLRenderCommandEncoder::EndEncoding() {
    // Transfer all commands to the command buffer
    commandBuffer->commands.insert(commandBuffer->commands.end(), commands.begin(), commands.end());
    commands.clear();
    // Clean up temporary vertex UBOs
    for (GLuint ubo : tempVertexUBOs) {
        glDeleteBuffers(1, &ubo);
    }
    tempVertexUBOs.clear();

    // Clean up temporary fragment UBOs
    for (GLuint ubo : tempFragmentUBOs) {
        glDeleteBuffers(1, &ubo);
    }
    tempFragmentUBOs.clear();
}

// Add the new methods to OpenGLRenderDevice
CommandQueue* OpenGLRenderDevice::CreateCommandQueue() {
    return new OpenGLCommandQueue(this);
}

void OpenGLRenderDevice::DestroyCommandQueue(CommandQueue* queue) {
    delete static_cast<OpenGLCommandQueue*>(queue);
}

Drawable* OpenGLRenderDevice::GetNextDrawable() {
    // For default framebuffer, pass nullptr for texture and the GLFWwindow* for window
    return new OpenGLDrawable(nullptr, glfwGetCurrentContext());
}

void OpenGLRenderDevice::DestroyDrawable(Drawable* drawable) {
    OpenGLDrawable* oglDrawable = static_cast<OpenGLDrawable*>(drawable);
    delete oglDrawable;
}

void OpenGLRenderCommandEncoder::SetViewport(int x, int y, int width, int height) {
    commands.push_back([x, y, width, height]() {
        glViewport(x, y, width, height);
    });
}

void OpenGLDrawable::GetSize(int& width, int& height) {
    if (texture) {
        // For texture drawables, get the texture size
        width = texture->width;
        height = texture->height;
    } else {
        // For default framebuffer (window), get the window size from GLFW
        GLFWwindow* window = glfwGetCurrentContext();
        if (window) {
            glfwGetFramebufferSize(window, &width, &height);
        } else {
            width = 800;  // fallback
            height = 600; // fallback
        }
    }
}

} // end namespace render
