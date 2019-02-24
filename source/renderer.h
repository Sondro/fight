#define OpenGLProc(type, name) PFNGL##type##PROC gl##name = 0;
#include "opengl_function_list.inc"

#include "renderer_opengl_shaders.c"

#define RENDERER_FLIP_HORIZONTAL 0x01
#define RENDERER_FLIP_VERTICAL 0x02

typedef struct Shader
{
    GLuint id;
}
Shader;

typedef struct Texture
{
    GLuint id;
    int width;
    int height;
}
Texture;

internal Texture
TextureLoad(const char *filename);

enum
{
    RENDERER_REQUEST_null,
    RENDERER_REQUEST_filled_rect,
    RENDERER_REQUEST_texture,
};

typedef struct RendererRequest
{
    i32 type;
    u32 data_offset;
    u32 data_size;
    v4 color;
    b32 additive;
    Texture *texture;
}
RendererRequest;

typedef struct Renderer
{
    RendererRequest active_request;
    
    f32 render_w;
    f32 render_h;
    Mat4x4 projection_matrix;
    
    GLuint filled_rect_vao;
    GLuint filled_rect_buffer;
    GLuint filled_rect_instance_buffer;
    u32 filled_rect_instance_data_alloc_pos;
#define RENDERER_OPENGL_BYTES_PER_FILLED_RECT (sizeof(f32) * 20)
    GLubyte filled_rect_instance_data[RENDERER_MAX_FILLED_RECTS * RENDERER_OPENGL_BYTES_PER_FILLED_RECT];
    
    
    GLuint texture_vao;
    GLuint texture_buffer;
    GLuint texture_instance_buffer;
    u32 texture_instance_data_alloc_pos;
    // NOTE(rjf):                          source, dest      opacity
#define RENDERER_OPENGL_BYTES_PER_TEXTURE (sizeof(f32)*8 + sizeof(f32)*1)
    GLubyte texture_instance_data[RENDERER_MAX_TEXTURES * RENDERER_OPENGL_BYTES_PER_TEXTURE];
    
    u32 request_count;
    RendererRequest requests[RENDERER_MAX_REQUESTS];
    
    Shader shaders[RENDERER_OPENGL_DEFAULT_SHADER_MAX];
}
Renderer;

internal void RendererInit(Renderer *renderer);
internal void RendererBeginFrame(Renderer *renderer, f32 render_w, f32 render_h);
internal void RendererEndFrame(Renderer *renderer);
internal void RendererPushFilledRect(Renderer *renderer, v2 pos, v2 size,
                                     v4 color00, v4 color01, v4 color10, v4 color11);
internal void RendererPushFilledRectS(Renderer *renderer, v2 pos, v2 size, v4 color);
internal void RendererPushTexture(Renderer *renderer, Texture *texture, i32 flags, v4 source, v4 destination, f32 opacity);