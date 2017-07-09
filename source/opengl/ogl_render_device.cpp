#include "ogl_render_device.h"

#include <glad/glad.h>

#include <cstring>

#include <iostream>
#include <string>
#include <map>

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

class OpenGLPipelineParam;

class OpenGLPipeline : public Pipeline
{
public:

	OpenGLPipeline(OpenGLFunction *vertexFunction, OpenGLFunction *fragmentFunction, OpenGLVertexDescriptor *vertexDescriptor)
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
	}

	~OpenGLPipeline() override
	{
		delete vertexDescriptor;

		glDeleteVertexArrays(1, &vertexArrayObject);

		glDeleteProgram(shaderProgram);
	}

	PipelineParam *GetParam(const char *name) override;

	unsigned int shaderProgram = 0;

	std::map<std::string, OpenGLPipelineParam *> paramsByName;

	unsigned int vertexArrayObject = 0;

	OpenGLVertexDescriptor *vertexDescriptor = nullptr;
};

class OpenGLPipelineParam : public PipelineParam
{
public:

	OpenGLPipelineParam(OpenGLPipeline *_pipeline, int _location) : pipeline(_pipeline), location(_location) {}

	void SetAsInt(int value) override
	{
		glUseProgram(pipeline->shaderProgram);
		glUniform1i(location, value);
	}

	void SetAsFloat(float value) override
	{
		glUseProgram(pipeline->shaderProgram);
		glUniform1f(location, value);
	}

	void SetAsMat4(const float *value) override
	{
		glUseProgram(pipeline->shaderProgram);
		glUniformMatrix4fv(location, 1, /*transpose=*/GL_FALSE, value);
	}

	void SetAsIntArray(int count, const int *values) override
	{
		glUseProgram(pipeline->shaderProgram);
		glUniform1iv(location, count, values);
	}

	void SetAsFloatArray(int count, const float *values) override
	{
		glUseProgram(pipeline->shaderProgram);
		glUniform1fv(location, count, values);
	}

	void SetAsMat4Array(int count, const float *values) override
	{
		glUseProgram(pipeline->shaderProgram);
		glUniformMatrix4fv(location, count, /*transpose=*/GL_FALSE, values);
	}

	OpenGLPipeline *pipeline;
	int location;
};

PipelineParam *OpenGLPipeline::GetParam(const char *name)
{
	auto const &iter = paramsByName.find(name);
	if(iter == paramsByName.end())
	{
		int location = glGetUniformLocation(shaderProgram, name);
		if(location < 0) return nullptr;
		OpenGLPipelineParam *param = new OpenGLPipelineParam(this, location);
		paramsByName.insert(iter, std::make_pair(name, param));
		return param;
	}
	return iter->second;
}

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
};

class OpenGLRasterState : public RasterState
{
public:

	OpenGLRasterState(bool _cullEnabled = true, Winding _frontFace = WINDING_CCW, Face _cullFace = FACE_BACK, RasterMode _rasterMode = RASTERMODE_FILL)
	{
		static const GLenum front_face_map[] = { GL_CW, GL_CCW };
		static const GLenum cull_face_map[] = { GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };
		static const GLenum raster_mode_map[] = { GL_POINT, GL_LINE, GL_FILL };

		cullEnabled = _cullEnabled;
		frontFace = front_face_map[_frontFace];
		cullFace = cull_face_map[_cullFace];
		polygonMode = raster_mode_map[_rasterMode];
	}

	bool cullEnabled;
	GLenum frontFace;
	GLenum cullFace;
	GLenum polygonMode;
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
	m_DefaultRasterState = dynamic_cast<OpenGLRasterState *>(CreateRasterState());
	SetRasterState(m_DefaultRasterState);

	m_DefaultDepthStencilState = dynamic_cast<OpenGLDepthStencilState *>(CreateDepthStencilState());
	SetDepthStencilState(m_DefaultDepthStencilState);
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

Pipeline *OpenGLRenderDevice::CreatePipeline(Function *vertexShader, Function *fragmentShader, VertexDescriptor *vertexDescriptor)
{
	return new OpenGLPipeline(reinterpret_cast<OpenGLFunction *>(vertexShader), reinterpret_cast<OpenGLFunction *>(fragmentShader), reinterpret_cast<OpenGLVertexDescriptor *>(vertexDescriptor));
}

void OpenGLRenderDevice::DestroyPipeline(Pipeline *pipeline)
{
	delete pipeline;
}

void OpenGLRenderDevice::SetPipeline(Pipeline *pipeline)
{
	m_Pipeline = reinterpret_cast<OpenGLPipeline *>(pipeline);
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

RasterState *OpenGLRenderDevice::CreateRasterState(bool cullEnabled, Winding frontFace, Face cullFace, RasterMode rasterMode)
{
	return new OpenGLRasterState(cullEnabled, frontFace, cullFace, rasterMode);
}

void OpenGLRenderDevice::DestroyRasterState(RasterState *rasterState)
{
	delete rasterState;
}

void OpenGLRenderDevice::SetRasterState(RasterState *rasterState)
{
	RasterState *oldRasterState = m_RasterState;

	if(rasterState)
		m_RasterState = dynamic_cast<OpenGLRasterState *>(rasterState);
	else
		m_RasterState = m_DefaultRasterState;

	if(m_RasterState != oldRasterState)
	{
		if(m_RasterState->cullEnabled)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
		glFrontFace(m_RasterState->frontFace);
		glCullFace(m_RasterState->cullFace);
		glPolygonMode(GL_FRONT_AND_BACK, m_RasterState->polygonMode);
	}
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

	if (depthStencilState)
		m_DepthStencilState = dynamic_cast<OpenGLDepthStencilState *>(depthStencilState);
	else
		m_DepthStencilState = m_DefaultDepthStencilState;

	if(m_DepthStencilState != oldDepthStencilState)
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
	switch (primitiveType) {
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

	glUseProgram(m_Pipeline->shaderProgram);
	glBindVertexArray(m_Pipeline->vertexArrayObject);
	OpenGLVertexDescriptor *vertexDescriptor = m_Pipeline->vertexDescriptor;
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

	glDrawArrays(mode, offset, count);

	glBindVertexArray(0);
	glUseProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderDevice::DrawIndexed(const PrimitiveType& primitiveType, const IndexType& indexType, Buffer *indexBuffer, long long offset, int count)
{
	GLenum mode;
	switch (primitiveType) {
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

	glUseProgram(m_Pipeline->shaderProgram);
	glBindVertexArray(m_Pipeline->vertexArrayObject);
	OpenGLVertexDescriptor *vertexDescriptor = m_Pipeline->vertexDescriptor;
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

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast<OpenGLBuffer *>(indexBuffer)->BO);
	switch (indexType)
	{
		case INDEXTYPE_UINT16: {
			glDrawElements(mode, count, GL_UNSIGNED_SHORT, reinterpret_cast<const void *>(offset));
		}
		break;
		case INDEXTYPE_UINT32: {
			glDrawElements(mode, count, GL_UNSIGNED_INT, reinterpret_cast<const void *>(offset));
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

} // end namespace render
