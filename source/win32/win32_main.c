#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include "ext/wglext.h"
#include "ext/glext.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "ext/stb_image.h"

#include "program_options.h"
#include "language_layer.h"
#include "platform.h"

#include "game.c"

global Platform global_platform = {0};
global HWND global_window = {0};
global HGLRC global_opengl_render_context = {0};

internal void *
Win32OpenGLProcAddress(const char *name)
{
    void *p = (void *)wglGetProcAddress(name);
    if(!p || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 || p == (void *)-1)
    {
        return 0;
    }
    else
    {
        return p;
    }
}

PFNWGLCHOOSEPIXELFORMATARBPROC     wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC  wglCreateContextAttribsARB;
PFNWGLMAKECONTEXTCURRENTARBPROC    wglMakeContextCurrentARB;
PFNWGLSWAPINTERVALEXTPROC          wglSwapIntervalEXT;

internal void
Win32LoadWGLProcedures(HINSTANCE h_instance)
{
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)Win32OpenGLProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)Win32OpenGLProcAddress("wglCreateContextAttribsARB");
    wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC)Win32OpenGLProcAddress("wglMakeContextCurrentARB");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)Win32OpenGLProcAddress("wglSwapIntervalEXT");
}

// Initialize OpenGL

internal b32
Win32InitOpenGL(HDC *device_context, HINSTANCE h_instance)
{
    
    b32 result = 0;
    
    // NOTE(rjf): Set up pixel format for dummy context
    int pixel_format = 0;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    pixel_format = ChoosePixelFormat(*device_context, &pfd);
    
    if(pixel_format)
    {
        SetPixelFormat(*device_context, pixel_format, &pfd);
        HGLRC gl_dummy_render_context = wglCreateContext(*device_context);
        wglMakeCurrent(*device_context, gl_dummy_render_context);
        
        Win32LoadWGLProcedures(h_instance);
        
        // NOTE(rjf): Set up real pixel format
        {
            int pf_attribs_i[] = {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_DEPTH_BITS_ARB, 24,
                WGL_STENCIL_BITS_ARB, 8,
                0
            };
            
            UINT num_formats = 0;
            wglChoosePixelFormatARB(*device_context,
                                    pf_attribs_i,
                                    0,
                                    1,
                                    &pixel_format,
                                    &num_formats);
        }
        
        if(pixel_format)
        {
            const int context_attribs[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                0
            };
            
            global_opengl_render_context = wglCreateContextAttribsARB(*device_context,
                                                                      gl_dummy_render_context,
                                                                      context_attribs);
            if(global_opengl_render_context)
            {
                wglMakeCurrent(*device_context, 0);
                wglDeleteContext(gl_dummy_render_context);
                wglMakeCurrent(*device_context, global_opengl_render_context);
                wglSwapIntervalEXT(0);
                result = 1;
            }
        }
    }
    
    return result;
}

internal void
Win32CleanUpOpenGL(HDC *device_context) {
    wglMakeCurrent(*device_context, 0);
    wglDeleteContext(global_opengl_render_context);
}

internal LRESULT
Win32WindowProcedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = {0};
    
    if(message == WM_DESTROY)
    {
        global_platform.quit = 1;
    }
    else if(message == WM_MOUSEMOVE)
    {
        global_platform.mouse_x = (f32)(l_param & 0x0000FFFF);
        global_platform.mouse_y = (f32)((l_param & 0xFFFF0000) >> 16);
    }
    else if(message == WM_KEYDOWN || message == WM_KEYUP)
    {
        b32 key_is_down = message == WM_KEYDOWN;
        u32 key_code = w_param;
        u32 key_index = 0;
        if(key_code >= 'A' && key_code <= 'Z')
        {
            key_index = KEY_a + (key_code - 'A');
        }
        else if(key_code == VK_ESCAPE)
        {
            key_index = KEY_escape;
        }
        else if(key_code == VK_LEFT)
        {
            key_index = KEY_left;
        }
        else if(key_code == VK_UP)
        {
            key_index = KEY_up;
        }
        else if(key_code == VK_RIGHT)
        {
            key_index = KEY_right;
        }
        else if(key_code == VK_DOWN)
        {
            key_index = KEY_down;
        }
        else if(key_code == VK_RETURN)
        {
            key_index = KEY_enter;
        }
        else if(key_code == VK_SPACE)
        {
            key_index = KEY_space;
        }
        
        if(key_index > 0)
        {
            global_platform.key_pressed[key_index] = !global_platform.key_down[key_index] ? key_is_down : 0;
            global_platform.key_down[key_index] = key_is_down;
        }
    }
    else
    {
        result = DefWindowProc(window, message, w_param, l_param);
    }
    
    return result;
}

internal void
Win32MessageBox(const char *title, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    u32 required_characters = vsnprintf(0, 0, format, args)+1;
    va_end(args);
    
    local_persist char text[4096] = {0};
    
    if(required_characters > 4096)
    {
        required_characters = 4096;
    }
    
    va_start(args, format);
    vsnprintf(text, required_characters, format, args);
    va_end(args);
    
    text[required_characters-1] = 0;
    
    MessageBoxA(0, text, title, MB_OK);
}

internal void
Win32LoadEntireFile(const char *filename, void **data, u64 *len, b32 error_on_non_existence)
{
    *data = 0;
    *len = 0;
    
    FILE *file = fopen(filename, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        u64 file_size = (u64)ftell(file);
        fseek(file, 0, SEEK_SET);
        *data = malloc(file_size);
        if(*data)
        {
            fread(*data, 1, file_size, file);
            *len = file_size;
        }
        else
        {
            Win32MessageBox("File I/O Error", "Memory to load \"%s\" could not be allocated.", filename);
        }
        fclose(file);
    }
    else if(error_on_non_existence)
    {
        Win32MessageBox("File I/O Error", "\"%s\" could not be opened.", filename);
    }
}

internal void
Win32FreeFileData(void *data)
{
    free(data);
}

int
WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command_line, int command_show)
{
    
    // NOTE(rjf): Create the window class.
    WNDCLASS window_class = {0};
    {
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = Win32WindowProcedure;
        window_class.hInstance = instance;
        window_class.lpszClassName = "FightWindowClass";
        window_class.hCursor = LoadCursor(0, IDC_ARROW);
        window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    }
    
    // NOTE(rjf): Register class
    if(!RegisterClass(&window_class))
    {
        Win32MessageBox("Fatal Error", "Window class registration failure.");
        goto quit;
    }
    
    // NOTE(rjf): Create window
    {
        HWND window = CreateWindow(window_class.lpszClassName,
                                   WINDOW_TITLE,
                                   WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   DEFAULT_WINDOW_WIDTH,
                                   DEFAULT_WINDOW_HEIGHT,
                                   0, 0, instance, 0);
        
        if(!window)
        {
            Win32MessageBox("Fatal Error", "Window creation failure.");
            goto quit;
        }
        
        global_window = window;
    }
    
    // NOTE(rjf): Initialize platform struct
    {
        global_platform.permanent_storage_size = PERMANENT_STORAGE_SIZE;
        global_platform.scratch_storage_size = SCRATCH_STORAGE_SIZE;
        global_platform.permanent_storage = VirtualAlloc(0, global_platform.permanent_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.scratch_storage = VirtualAlloc(0, global_platform.scratch_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.LoadOpenGLProcedure = Win32OpenGLProcAddress;
        global_platform.OutputError = Win32MessageBox;
        global_platform.LoadEntireFile = Win32LoadEntireFile;
        
        if(!global_platform.permanent_storage || !global_platform.scratch_storage)
        {
            Win32MessageBox("Fatal Error", "Application memory allocation failure.");
            goto quit;
        }
    }
    
    HDC window_device_context = GetDC(global_window);
    
    if(!Win32InitOpenGL(&window_device_context, instance))
    {
        Win32MessageBox("Fatal Error", "OpenGL initialization failure.");
        goto quit;
    }
    
    GameInit(&global_platform);
    
    ShowWindow(global_window, command_show);
    UpdateWindow(global_window);
    
    LARGE_INTEGER performance_counter_frequency;
    LARGE_INTEGER start_frame_time_data;
    LARGE_INTEGER end_frame_time_data;
    
    QueryPerformanceFrequency(&performance_counter_frequency);
    
    while(!global_platform.quit)
    {
        QueryPerformanceCounter(&start_frame_time_data);
        
        MSG message;
        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        
        GameUpdate();
        wglSwapLayerBuffers(window_device_context, WGL_SWAP_MAIN_PLANE);
        
        QueryPerformanceCounter(&end_frame_time_data);
        
        // NOTE(rjf): Wait, if necessary.
        {
            i64 frame_count = end_frame_time_data.QuadPart - start_frame_time_data.QuadPart;
            i64 desired_frame_count = (f32)performance_counter_frequency.QuadPart / FRAMES_PER_SECOND;
            i64 counts_to_wait = desired_frame_count - frame_count;
            
            LARGE_INTEGER begin_wait_time_data;
            LARGE_INTEGER end_wait_time_data;
            
            QueryPerformanceCounter(&begin_wait_time_data);
            
            while(counts_to_wait > 0)
            {
                QueryPerformanceCounter(&end_wait_time_data);
                counts_to_wait -= (end_wait_time_data.QuadPart - begin_wait_time_data.QuadPart);
                begin_wait_time_data = end_wait_time_data;
            }
        }
    }
    
    quit:;
    return 0;
}