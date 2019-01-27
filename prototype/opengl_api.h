#ifndef _API_H_
#define _API_H_

#include <GL/glew.h>
#include "int.h"

struct ogl_buffer
{
    uint32 ID;

    void
    Create(void);

    void
    Destroy(void);

    void
    Bind(GLenum BindingPoint) const;

    void
    Fill(uint32 Size
	 , void *Data
	 , GLenum WritePattern
	 , GLenum BindingPoint);

    void
    PartialFill(uint32 Offset
		 , uint32 Size
		 , void *Data
		 , GLenum BindingPoint);

    void *
    Map(GLenum BindingPoint
	      , GLenum Access);

    void
    BindBase(GLenum BindingPoint) const;
};

struct ogl_vbo : ogl_buffer
{
    uint32 Count;
};

struct ogl_ibo : ogl_buffer
{
    uint32 Type;
    uint32 Count;
    void *Start;
};

struct ogl_ubo : ogl_buffer
{
    uint32 Index{ 0xffffffff };

    void
    BindBase(GLenum BindingPoint) const;
};

struct ogl_attribute_create_info
{
    uint32 AttributeNumber;
    uint32 SizeInTypes;
    uint32 Type;
    GLenum Normalized;
    uint32 StrideBytes;
    void *StartPointer;
};

struct ogl_vertex_array
{
    uint32 ID;

    void
    Create(void);

    void
    Destroy(void);

    void
    Bind(void) const;
    
    void
    Attach(ogl_vbo & Buffer
	   , ogl_attribute_create_info &Attribute);

    void
    AddDivisor(ogl_vbo &Buffer
	       , uint32 Attribute
	       , uint32 Value);
};

struct ogl_framebuffer
{
    uint32 ID;
    uint32 Width, Height;

    void
    Create(void);

    void
    Destroy(void);

    void
    Bind(GLenum BindingPoint) const;

    void
    SetDrawBuffers(GLenum *Buffers
		   , uint32 Size);

    void
    Blit(ogl_framebuffer &Other);

    void
    AttachTexture(struct ogl_texture &Texture
		  , uint32 Attachment
		  , uint32 Level
		  , GLenum FBOBindingPoint);

    void 
    AttachRenderbuffer(struct ogl_renderbuffer &Renderbuffer
		       , uint32 Attachment
		       , GLenum FBOBindingPoint);

    void
    SelectColorBuffer(uint32 Point);
    
    bool
    Status(GLenum BindingPoint);
};

enum : bool
{
    COMPILATION_SUCCESS = true
    , COMPILATION_ERROR = false
	
    , LINK_SUCCESS = true
    , LINK_ERROR = false
};

struct ogl_shader_compilation_info
{
    bool Status;
    const char *Message;
};

struct ogl_shader
{
    uint32 ID;
    GLenum ShaderType{ 0xffffffff };

    void
    Create(void);

    void 
    Destroy(void);

    ogl_shader_compilation_info
    Compile(const char **Sources
	    , uint32 Size);
};

struct ogl_program
{
    uint32 ID;

    void
    Create(void);
    
    void
    Bind(void) const;

    void
    AttachShader(ogl_shader &Shader);

    ogl_shader_compilation_info
    Link(void);
};

struct ogl_texture
{
    uint32 ID;
    uint32 Width, Height;

    void
    Create(void);

    void 
    Bind(GLenum BindingPoint) const;

    void
    Activate(GLenum ActivationPoint) const;

    void
    Fill(GLenum Target
	 , GLenum InternalFormat
	 , GLenum Format
	 , GLenum Type
	 , const void *Data);

    void
    EnableMipmap(GLenum Target);

    void
    FParam(GLenum Target
	   , GLenum Mode
	   , float Factor);

    void
    IParam(GLenum Target
	   , GLenum Mode
	   , GLenum Factor);
};

// TODO Finish Renderbuffer struct
struct ogl_renderbuffer
{
    uint32 ID;
    uint32 Width, Height;

    void
    Create(void);

    void
    Destroy(void);

    void
    Bind(GLenum BindingPoint) const;

    //void
    //SetStorage(GLenum Component
    //	       ,  );
};

#endif
