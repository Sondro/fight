
enum
{
    RENDERER_OPENGL_SHADER_filled_rect,
    RENDERER_OPENGL_SHADER_texture,
    RENDERER_OPENGL_DEFAULT_SHADER_MAX
};

struct
{
    const char *info;
    const char *vert;
    const char *frag;
} global_default_opengl_shaders[RENDERER_OPENGL_DEFAULT_SHADER_MAX] = {
    {
        "in vert_position 0\n"
            "in vert_rect_data 1\n"
            "in vert_color00 2\n"
            "in vert_color01 3\n"
            "in vert_color10 4\n"
            "in vert_color11 5\n"
            "out color 0\n"
            "\n"
            "// ============================================================================\n"
            "\n"
            "",
        "#version 330 core\n"
            "in vec2 vert_position;\n"
            "in vec4 vert_rect_data;\n"
            "in vec4 vert_color00;\n"
            "in vec4 vert_color01;\n"
            "in vec4 vert_color10;\n"
            "in vec4 vert_color11;\n"
            "out vec4 rect_color;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    vec4 screen_position = vec4(vert_position, 0, 1);\n"
            "    vec4 destination = vert_rect_data;\n"
            "    screen_position.xy *= destination.zw;\n"
            "    screen_position.xy += destination.xy;\n"
            "    screen_position = projection * screen_position;\n"
            "    gl_Position = screen_position;\n"
            "    \n"
            "    vec4 colors[] = vec4[](vert_color00, vert_color01, vert_color10, vert_color11);\n"
            "    \n"
            "    rect_color = colors[gl_VertexID];\n"
            "}\n"
            "\n"
            "// ============================================================================\n"
            "\n"
            "",
        "#version 330 core\n"
            "in vec4 rect_color;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "    color = rect_color;\n"
            "}\n"
            "\n"
            "// ===========================================================================",
    },
    
    {
        ""
            "in vert_position 0\n"
            "in vert_source 1\n"
            "in vert_dest 2\n"
            "in vert_opacity 3\n"
            "out color 0\n"
            "\n"
            "// ============================================================================\n"
            "\n"
            "",
        
        ""
            "#version 330 core\n"
            "in vec2 vert_position;\n"
            "in vec4 vert_source;\n"
            "in vec4 vert_dest;\n"
            "in float vert_opacity;\n"
            "out vec2 frag_uv;\n"
            "out vec4 frag_source;\n"
            "out vec2 frag_scale;\n"
            "out float frag_opacity;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    vec4 screen_position = vec4(vert_position, 0, 1);\n"
            "    vec4 destination;\n"
            "    destination.zw = vert_source.zw;\n"
            "    destination.zw *= vert_dest.zw;\n"
            "    destination.xy = vert_dest.xy;\n"
            "    screen_position.xy *= destination.zw;\n"
            "    screen_position.xy += destination.xy;\n"
            "    screen_position = projection * screen_position;\n"
            "    gl_Position = screen_position;\n"
            "    frag_uv = vert_position.xy;\n"
            "    frag_source = vert_source;\n"
            "    frag_scale = vert_dest.zw;\n"
            "    frag_opacity = vert_opacity;\n"
            "}\n"
            "\n"
            "// ============================================================================\n"
            "\n"
            "",
        
        ""
            "#version 330 core\n"
            "in vec2 frag_uv;\n"
            "in vec4 frag_source;\n"
            "in vec2 frag_scale;\n"
            "in float frag_opacity;\n"
            "out vec4 color;\n"
            "uniform sampler2D tex;\n"
            "uniform vec2 tex_resolution;\n"
            "void main() {\n"
            "    vec2 uv_offset = frag_source.xy;\n"
            "    vec2 uv_range = frag_source.zw;\n"
            "    float opacity = frag_opacity;\n"
            "    vec2 scale = frag_scale;\n"
            "    \n"
            "    vec2 pixel = (uv_offset + (frag_uv * uv_range));\n"
            "    vec2 sample_uv = floor(pixel) + vec2(0.5, 0.5);\n"
            "    \n"
            "    sample_uv.x += 1.0 - clamp((1.0 - fract(pixel.x)) * abs(scale.x), 0.0, 1.0);\n"
            "    sample_uv.y += 1.0 - clamp((1.0 - fract(pixel.y)) * abs(scale.y), 0.0, 1.0);\n"
            "    \n"
            "    color = texture(tex, sample_uv / tex_resolution);\n"
            "    \n"
            "    if(color.a > 0) {\n"
            "        color.xyz /= color.a;\n"
            "        color *= opacity;\n"
            "    }\n"
            "    else {\n"
            "        discard;\n"
            "    }\n"
            "}\n"
            "\n"
            "// ===========================================================================",
    },
};
