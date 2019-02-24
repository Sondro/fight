
internal Shader
ShaderInitFromData(const void *vert, u64 vert_len,
                   const void *frag, u64 frag_len,
                   const void *info, u64 info_len)
{
    
    Shader shader = {0};
    
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    
    GLint result = GL_FALSE;
    GLint code_len = 0;
    i32 info_log_length = 0;
    const char *code = 0;
    
    // Compile vertex shader
    {
        code = (const char *)vert;
        code_len = (GLint)CalculateCStringLength(code) -1;
        Log("Compiling vertex shader");
        glShaderSource(vertex_shader_id, 1, &code, &code_len);
        glCompileShader(vertex_shader_id);
    }
    
    // Get vertex shader errors
    {
        glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
        if(info_log_length > 1)
        {
            char vertex_shader_error[1024] = {0};
            glGetShaderInfoLog(vertex_shader_id, sizeof(vertex_shader_error), 0,
                               vertex_shader_error);
            Log("%s", vertex_shader_error);
        }
    }
    
    // Compile fragment shader
    {
        code = (const char *)frag;
        code_len = (GLint)CalculateCStringLength(code) - 1;
        Log("Compiling fragment shader");
        glShaderSource(fragment_shader_id, 1, &code, &code_len);
        glCompileShader(fragment_shader_id);
    }
    
    // Get fragment shader errors
    {
        glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
        glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
        if(info_log_length > 1)
        {
            char fragment_shader_error[1024] = {0};
            glGetShaderInfoLog(fragment_shader_id, sizeof(fragment_shader_error), 0, fragment_shader_error);
            Log("%s", fragment_shader_error);
        }
    }
    
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    
    // Parse info file
    {
        enum {
            READ_DIRECTION,
            READ_NAME,
            READ_INDEX
        };
        
        i8 read_mode = READ_DIRECTION,
        in = 0;
        u64 read_start = 0;
        char name[64] = {0};
        char index_str[16] = {0};
        
        code = (const char *)info;
        
        for(u32 i = 0; i < info_len; ++i)
        {
            switch(read_mode)
            {
                case READ_DIRECTION:
                {
                    if(!strncmp(code + i, "in", 2))
                    {
                        in = 1;
                        read_mode = READ_NAME;
                        read_start = i + 3;
                        i += 3;
                    }
                    else if(!strncmp(code + i, "out", 3))
                    {
                        in = 0;
                        read_mode = READ_NAME;
                        read_start = i + 4;
                        i += 4;
                    }
                    break;
                }
                case READ_NAME:
                {
                    if(code[i] == ' ')
                    {
                        memset(name, 0, 64);
                        strncpy(name, code + read_start, (i - read_start) < 64 ? i-read_start : 64);
                        read_start = i + 1;
                        read_mode = READ_INDEX;
                    }
                    break;
                }
                case READ_INDEX:
                {
                    if(i == strlen(code) - 1 || code[i] == ' ' || code[i] == '\n')
                    {
                        
                        memset(index_str, 0, 16);
                        strncpy(index_str, code + read_start, (i-read_start) < 16 ? i-read_start : 16);
                        
                        if(in)
                        {
                            Log("Binding \"%s\" for input at index %i", name, atoi(index_str));
                            glBindAttribLocation(program_id, atoi(index_str), name);
                        }
                        else
                        {
                            Log("Binding \"%s\" for output at index %i", name, atoi(index_str));
                            glBindFragDataLocation(program_id, atoi(index_str), name);
                        }
                        
                        read_mode = READ_DIRECTION;
                    }
                    break;
                }
                default: break;
            }
        }
    }
    
    glLinkProgram(program_id);
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    
    shader.id = program_id;
    
    return shader;
}

internal void
ShaderCleanUp(Shader *shader)
{
    if(shader->id)
    {
        glDeleteProgram(shader->id);
        shader->id = 0;
    }
}

internal Texture
TextureInitFromData(void *data, u64 len)
{
    Texture t;
    int w, h;
    u8 *tex_data = stbi_load_from_memory((unsigned char *)data, (int)len,
                                         &w, &h, 0,
                                         STBI_rgb_alpha);
    t.width = w;
    t.height = h;
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.width, t.height, 0, GL_RGBA, 
                 GL_UNSIGNED_BYTE, tex_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(tex_data);
    return t;
}

internal Texture
TextureLoad(const char *filename)
{
    Texture texture = {0};
    void *png_data = 0;
    u64 png_data_len = 0;
    platform->LoadEntireFile(filename, &png_data, &png_data_len, 0);
    if(png_data && png_data_len)
    {
        texture = TextureInitFromData(png_data, png_data_len);
        platform->FreeFileData(png_data);
    }
    return texture;
}

internal void
_RendererFinishActiveRequest(Renderer *renderer)
{
    if(renderer->active_request.type > 0)
    {
        Assert(renderer->request_count < RENDERER_MAX_REQUESTS);
        renderer->requests[renderer->request_count++] = renderer->active_request;
        renderer->active_request.type = 0;
    }
}

internal void
RendererInit(Renderer *renderer)
{
#define OpenGLProc(type, name) gl##name = platform->LoadOpenGLProcedure("gl" #name);
#include "opengl_function_list.inc"
    
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    GLfloat filled_quad_vertices[8] = {0};
    
    // Initialize primitive vertices
    {
        filled_quad_vertices[0] = 0;
        filled_quad_vertices[1] = 0;
        filled_quad_vertices[2] = 0;
        filled_quad_vertices[3] = 1;
        filled_quad_vertices[4] = 1;
        filled_quad_vertices[5] = 0;
        filled_quad_vertices[6] = 1;
        filled_quad_vertices[7] = 1;
    }
    
    
    // Initialize filled rectangle data
    {
        glGenVertexArrays(1, &renderer->filled_rect_vao);
        glBindVertexArray(renderer->filled_rect_vao);
        {
            glGenBuffers(1, &renderer->filled_rect_buffer);
            glBindBuffer(GL_ARRAY_BUFFER, renderer->filled_rect_buffer);
            glBufferData(GL_ARRAY_BUFFER,
                         sizeof(filled_quad_vertices),
                         filled_quad_vertices,
                         GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
            
            glGenBuffers(1, &renderer->filled_rect_instance_buffer);
            glBindBuffer(GL_ARRAY_BUFFER, renderer->filled_rect_instance_buffer);
            glBufferData(GL_ARRAY_BUFFER,
                         sizeof(renderer->filled_rect_instance_data),
                         0,
                         GL_DYNAMIC_DRAW);
            
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, RENDERER_OPENGL_BYTES_PER_FILLED_RECT, 0);
            glVertexAttribDivisor(1, 1);
            
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, RENDERER_OPENGL_BYTES_PER_FILLED_RECT, 
                                  (void *)(sizeof(f32)*4));
            glVertexAttribDivisor(2, 1);
            
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, RENDERER_OPENGL_BYTES_PER_FILLED_RECT, 
                                  (void *)(sizeof(f32)*8));
            glVertexAttribDivisor(3, 1);
            
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, RENDERER_OPENGL_BYTES_PER_FILLED_RECT, 
                                  (void *)(sizeof(f32)*12));
            glVertexAttribDivisor(4, 1);
            
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, RENDERER_OPENGL_BYTES_PER_FILLED_RECT, 
                                  (void *)(sizeof(f32)*16));
            glVertexAttribDivisor(5, 1);
        }
        glBindVertexArray(0);
    }
    
    for(u32 i = 0; i < RENDERER_OPENGL_DEFAULT_SHADER_MAX; ++i)
    {
        renderer->shaders[i] = ShaderInitFromData(global_default_opengl_shaders[i].vert, 0,
                                                  global_default_opengl_shaders[i].frag, 0,
                                                  global_default_opengl_shaders[i].info, 0);
    }
}

internal void
RendererBeginFrame(Renderer *renderer, f32 render_w, f32 render_h)
{
    renderer->render_w = render_w;
    renderer->render_h = render_h;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);
    glViewport(0, 0, (GLsizei)render_w, (GLsizei)render_h);
    renderer->projection_matrix = Mat4x4Orthographic(0, render_w, render_h, 0, 0, 100.f);
    
    renderer->filled_rect_instance_data_alloc_pos = 0;
    renderer->active_request.type = 0;
    renderer->request_count = 0;
}

internal void
RendererEndFrame(Renderer *renderer)
{
    
    _RendererFinishActiveRequest(renderer);
    
    for(u32 i = 0; i < renderer->request_count; ++i)
    {
        RendererRequest *request = renderer->requests + i;
        switch(request->type)
        {
            
            case RENDERER_REQUEST_filled_rect:
            {
                // NOTE(rjf): Upload data
                {
                    glBindBuffer(GL_ARRAY_BUFFER, renderer->filled_rect_instance_buffer);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, request->data_size,
                                    renderer->filled_rect_instance_data + request->data_offset);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
                
                GLuint shader = renderer->shaders[RENDERER_OPENGL_SHADER_filled_rect].id;
                
                glUseProgram(shader);
                glBindVertexArray(renderer->filled_rect_vao);
                {
                    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"),
                                       1, GL_FALSE,
                                       &renderer->projection_matrix.elements[0][0]);
                    
                    GLint first = 0;
                    GLsizei count = 4;
                    GLsizei instance_count = request->data_size / RENDERER_OPENGL_BYTES_PER_FILLED_RECT;
                    
                    glDrawArraysInstanced(GL_TRIANGLE_STRIP,
                                          first,
                                          count,
                                          instance_count);
                }
                glBindVertexArray(0);
                break;
            }
            
            default: break;
        }
    }
    
    glUseProgram(0);
    
    GLenum error = glGetError();
    if(error)
    {
        Log("[OpenGL Error] %i", (int)error);
    }
}

internal void
RendererPushFilledRect(Renderer *renderer, v2 pos, v2 size,
                       v4 color00, v4 color01, v4 color10, v4 color11)
{
    Assert(renderer->filled_rect_instance_data_alloc_pos + RENDERER_OPENGL_BYTES_PER_FILLED_RECT <= sizeof(renderer->filled_rect_instance_data));
    i32 request_type = RENDERER_REQUEST_filled_rect;
    if(renderer->active_request.type != request_type)
    {
        _RendererFinishActiveRequest(renderer);
        renderer->active_request.type = request_type;
        renderer->active_request.data_offset = renderer->filled_rect_instance_data_alloc_pos;
        renderer->active_request.data_size = RENDERER_OPENGL_BYTES_PER_FILLED_RECT;
    }
    else
    {
        renderer->active_request.data_size += RENDERER_OPENGL_BYTES_PER_FILLED_RECT;
    }
    
    GLubyte *data = (renderer->filled_rect_instance_data +
                     renderer->filled_rect_instance_data_alloc_pos);
    ((f32 *)data)[0] = pos.x;
    ((f32 *)data)[1] = pos.y;
    ((f32 *)data)[2] = size.x;
    ((f32 *)data)[3] = size.y;
    ((f32 *)data)[4] = color00.r;
    ((f32 *)data)[5] = color00.g;
    ((f32 *)data)[6] = color00.b;
    ((f32 *)data)[7] = color00.a;
    ((f32 *)data)[8] = color01.r;
    ((f32 *)data)[9] = color01.g;
    ((f32 *)data)[10] = color01.b;
    ((f32 *)data)[11] = color01.a;
    ((f32 *)data)[12] = color10.r;
    ((f32 *)data)[13] = color10.g;
    ((f32 *)data)[14] = color10.b;
    ((f32 *)data)[15] = color10.a;
    ((f32 *)data)[16] = color11.r;
    ((f32 *)data)[17] = color11.g;
    ((f32 *)data)[18] = color11.b;
    ((f32 *)data)[19] = color11.a;
    renderer->filled_rect_instance_data_alloc_pos += RENDERER_OPENGL_BYTES_PER_FILLED_RECT;
}

internal void
RendererPushFilledRectS(Renderer *renderer, v2 pos, v2 size, v4 color)
{
    RendererPushFilledRect(renderer, pos, size, color, color, color, color);
}

internal void
RendererPushTexture(Renderer *renderer, Texture *texture,
                    i32 flags, v4 source, v4 destination,
                    f32 opacity) {
    
    assert(renderer->texture_instance_data_alloc_pos + RENDERER_OPENGL_BYTES_PER_TEXTURE <=
           sizeof(renderer->texture_instance_data));
    i32 request_type = RENDERER_REQUEST_texture;
    
    if(renderer->active_request.type != request_type ||
       renderer->active_request.texture != texture)
    {
        _RendererFinishActiveRequest(renderer);
        renderer->active_request.type = request_type;
        renderer->active_request.data_offset = renderer->texture_instance_data_alloc_pos;
        renderer->active_request.data_size = RENDERER_OPENGL_BYTES_PER_TEXTURE;
        renderer->active_request.texture = texture;
    }
    else
    {
        renderer->active_request.data_size += RENDERER_OPENGL_BYTES_PER_TEXTURE;
    }
    
    if(flags & RENDERER_FLIP_HORIZONTAL)
    {
        source.x += source.z;
        source.z *= -1;
    }
    
    if(flags & RENDERER_FLIP_VERTICAL)
    {
        source.y += source.w;
        source.w *= -1;
    }
    
    v2 scale = {
        destination.z / source.z,
        destination.w / source.w,
    };
    
    GLubyte *data = (renderer->texture_instance_data +
                     renderer->texture_instance_data_alloc_pos);
    ((f32 *)data)[0] = source.x;
    ((f32 *)data)[1] = source.y;
    ((f32 *)data)[2] = source.z;
    ((f32 *)data)[3] = source.w;
    ((f32 *)data)[4] = destination.x;
    ((f32 *)data)[5] = destination.y;
    ((f32 *)data)[6] = scale.x;
    ((f32 *)data)[7] = scale.y;
    ((f32 *)data)[8] = opacity;
    renderer->texture_instance_data_alloc_pos += RENDERER_OPENGL_BYTES_PER_TEXTURE;
}
