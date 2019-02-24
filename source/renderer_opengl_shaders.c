
enum
{
    RENDERER_OPENGL_SHADER_filled_rect,
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
};
