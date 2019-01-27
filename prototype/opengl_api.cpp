#include <cassert>
#include "opengl_api.h"

void
ogl_buffer::Create(void)
{
    glGenBuffers(1, &ID);
}

void
ogl_buffer::Destroy(void)
{
    glDeleteBuffers(1, &ID);
}

void
ogl_buffer::Bind(GLenum BindingPoint) const
{
    glBindBuffer(BindingPoint, ID);
}

void
ogl_buffer::Fill(uint32 Size
		 , void *Data
		 , GLenum WritePattern
		 , GLenum BindingPoint)
{
    glBufferData(BindingPoint
		 , Size
		 , Data
		 , WritePattern);
}

void
ogl_buffer::PartialFill(uint32 Offset
			, uint32 Size
			, void *Data
			, GLenum BindingPoint)
{
    glBufferSubData(BindingPoint
		    , Offset
		    , Size
		    , Data);
}

void *
ogl_buffer::Map(GLenum BindingPoint
	  , GLenum Access)
{
    return glMapBuffer(BindingPoint
		       , Access);
}

void
ogl_ubo::BindBase(GLenum BindingPoint) const
{
#if DEBUG
    assert(Index != 0xffffffff);
#endif
    
    glBindBufferBase(BindingPoint
		     , Index
		     , ID);
}

void
ogl_vertex_array::Create(void)
{
    glGenVertexArrays(1, &ID);
}

void
ogl_vertex_array::Destroy(void)
{
    glDeleteVertexArrays(1, &ID);
}

void
ogl_vertex_array::Bind(void) const
{
    glBindVertexArray(ID);
}

void
ogl_vertex_array::Attach(ogl_vbo &Buffer
			 , ogl_attribute_create_info &Attribute)
{
    Bind();

    glEnableVertexAttribArray(Attribute.AttributeNumber);

    Buffer.Bind(GL_ARRAY_BUFFER);

    glVertexAttribPointer(Attribute.AttributeNumber
			  , Attribute.SizeInTypes
			  , Attribute.Type
			  , Attribute.Normalized
			  , Attribute.StrideBytes
			  , Attribute.StartPointer);
}

void
ogl_vertex_array::AddDivisor(ogl_vbo &Buffer
			     , uint32 Attribute
			     , uint32 Value)
{
    Buffer.Bind(GL_ARRAY_BUFFER);
    glVertexAttribDivisor(Attribute
			  , Value);
}

void
ogl_framebuffer::Create(void)
{
    glGenFramebuffers(1, &ID);
}

void
ogl_framebuffer::Destroy(void)
{
    glDeleteFramebuffers(1, &ID);
}

void
ogl_framebuffer::Bind(GLenum BindingPoint) const
{
    glBindFramebuffer(BindingPoint
		      , ID);
}

void
ogl_framebuffer::SetDrawBuffers(GLenum *Buffers
				, uint32 Size)
{
    glDrawBuffers(Size
		  , Buffers);
}

void
ogl_framebuffer::Blit(ogl_framebuffer &Other)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER
		      , Other.ID);
    glBindFramebuffer(GL_READ_FRAMEBUFFER
		      , ID);

    glBlitFramebuffer(0
		      , 0
		      , Width
		      , Height
		      , 0
		      , 0
		      , Other.Width
		      , Other.Height
		      , GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
		      , GL_NEAREST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER
		      , 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER
		      , 0);
}

void
ogl_framebuffer::AttachTexture(struct ogl_texture &Texture
			       , uint32 Attachment
			       , uint32 Level
			       , GLenum FBOBindingPoint)
{
    glFramebufferTexture(FBOBindingPoint
			 , Attachment
			 , Texture.ID
			 , Level);
}

void 
ogl_framebuffer::AttachRenderbuffer(struct ogl_renderbuffer &Renderbuffer
				    , uint32 Attachment
				    , GLenum FBOBindingPoint)
{
    glFramebufferRenderbuffer(FBOBindingPoint
			      , Attachment
			      , GL_RENDERBUFFER
			      , Renderbuffer.ID);
}

void
ogl_framebuffer::SelectColorBuffer(uint32 Point)
{
    glDrawBuffer(Point);
}
    
bool
ogl_framebuffer::Status(GLenum BindingPoint)
{
    return(glCheckFramebufferStatus(BindingPoint) == GL_FRAMEBUFFER_COMPLETE);
}

void
ogl_shader::Create(void)
{
#if DEBUG
    assert(ShaderType != 0xffffffff);
#endif

    ID = glCreateShader(ShaderType);
}

void 
ogl_shader::Destroy(void)
{
    glDeleteShader(ID);
}

internal ogl_shader_compilation_info
CheckOGLShaderStatus(uint32 ID
		     , PFNGLGETSHADERIVPROC GetIVProc
		     , PFNGLGETSHADERINFOLOGPROC GetLogProc
		     , GLenum StatusType)
{
    ogl_shader_compilation_info Info = {};

    int32 Status;
    GetIVProc(ID
	      , StatusType
	      , &Status);

    Info.Status = true;

    if (Status != GL_TRUE)
    {
	int32 InfoLogLength;
	GetIVProc(ID
		  , GL_INFO_LOG_LENGTH
		  , &InfoLogLength);

	char *MessageBuffer = new char[InfoLogLength];

	int32 Size;
	GetLogProc(ID
		   , InfoLogLength
		   , &Size
		   , MessageBuffer);

	Info.Status = false;
	Info.Message = MessageBuffer;
    }

    return Info;
}

ogl_shader_compilation_info
ogl_shader::Compile(const char **Sources
	, uint32 Size)
{
    glShaderSource(ID
		   , Size
		   , Sources
		   , 0);

    glCompileShader(ID);

    return CheckOGLShaderStatus(ID
				, glGetShaderiv
				, glGetShaderInfoLog
				, GL_COMPILE_STATUS);
}

void
ogl_program::Create(void)
{
    ID = glCreateProgram();
}
    
void
ogl_program::Bind(void) const
{
    glUseProgram(ID);
}

void
ogl_program::AttachShader(ogl_shader &Shader)
{
    glAttachShader(ID
		   , Shader.ID);
}

ogl_shader_compilation_info
ogl_program::Link(void)
{
    glLinkProgram(ID);
    
    return CheckOGLShaderStatus(ID
				, glGetProgramiv
				, glGetProgramInfoLog
				, GL_LINK_STATUS);
}

void
ogl_texture::Create(void)
{
    glGenTextures(1, &ID);
}

void 
ogl_texture::Bind(GLenum BindingPoint) const
{
    glBindTexture(BindingPoint
		  , ID);
}

void
ogl_texture::Activate(GLenum ActivationPoint) const
{
    glActiveTexture(ActivationPoint);
}

void
ogl_texture::Fill(GLenum Target
     , GLenum InternalFormat
     , GLenum Format
     , GLenum Type
     , const void *Data)
{
    glTexImage2D(Target
		 , 0
		 , InternalFormat
		 , Width
		 , Height
		 , 0
		 , Format
		 , Type
		 , Data);
}

void
ogl_texture::EnableMipmap(GLenum Target)
{
    glGenerateMipmap(Target);
    IParam(Target
	   , GL_TEXTURE_MIN_FILTER
	   , GL_LINEAR_MIPMAP_LINEAR);
    FParam(Target
	   , GL_TEXTURE_LOD_BIAS
	   , -1);
}

void
ogl_texture::FParam(GLenum Target
       , GLenum Mode
       , float Factor)
{
    glTexParameterf(Target
		    , Mode
		    , Factor);
}

void
ogl_texture::IParam(GLenum Target
       , GLenum Mode
       , GLenum Factor)
{
    glTexParameteri(Target
		    , Mode
		    , Factor);
}
