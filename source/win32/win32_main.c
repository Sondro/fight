#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "program_options.h"
#include "language_layer.h"
#include "platform.h"

#include "game.c"

global Platform global_platform = {0};
global HWND global_window = {0};

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
    
    struct
    {
        u32 width;
        u32 height;
        u8 *memory;
        struct {
            BITMAPINFOHEADER bmiHeader;
            void *bmi_colors_pointer;
        } bitmap_info;
    } backbuffer = {0};
    
    // NOTE(rjf): Initialize backbuffer
    {
        backbuffer.width = BACKBUFFER_WIDTH;
        backbuffer.height = BACKBUFFER_HEIGHT;
        u32 size_of_backbuffer = backbuffer.width*backbuffer.height*3;
        backbuffer.memory = VirtualAlloc(0,
                                         size_of_backbuffer,
                                         MEM_COMMIT | MEM_RESERVE,
                                         PAGE_READWRITE);
        
        backbuffer.bitmap_info.bmiHeader.biSize = sizeof(backbuffer.bitmap_info);
        backbuffer.bitmap_info.bmiHeader.biWidth = backbuffer.width;
        backbuffer.bitmap_info.bmiHeader.biHeight = -backbuffer.height;
        backbuffer.bitmap_info.bmiHeader.biPlanes = 1;
        backbuffer.bitmap_info.bmiHeader.biBitCount = 24;
        backbuffer.bitmap_info.bmiHeader.biCompression = BI_RGB;
        backbuffer.bitmap_info.bmiHeader.biSizeImage = 0;
        backbuffer.bitmap_info.bmiHeader.biXPelsPerMeter = 0;
        backbuffer.bitmap_info.bmiHeader.biYPelsPerMeter = 0;
        backbuffer.bitmap_info.bmiHeader.biClrUsed = 0;
        backbuffer.bitmap_info.bmiHeader.biClrImportant = 0;
        backbuffer.bitmap_info.bmi_colors_pointer = 0;
    }
    
    // NOTE(rjf): Initialize platform struct
    {
        global_platform.permanent_storage_size = PERMANENT_STORAGE_SIZE;
        global_platform.scratch_storage_size = SCRATCH_STORAGE_SIZE;
        global_platform.permanent_storage = VirtualAlloc(0, global_platform.permanent_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.scratch_storage = VirtualAlloc(0, global_platform.scratch_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.backbuffer_width = backbuffer.width;
        global_platform.backbuffer_height = backbuffer.height;
        global_platform.backbuffer = backbuffer.memory;
        
        if(!global_platform.permanent_storage ||
           !global_platform.scratch_storage)
        {
            Win32MessageBox("Fatal Error", "Application memory allocation failure.");
            goto quit;
        }
    }
    
    GameInit(&global_platform);
    
    ShowWindow(global_window, command_show);
    UpdateWindow(global_window);
    
    HDC window_device_context = GetDC(global_window);
    
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
        
        StretchDIBits(window_device_context,
                      0, 0, global_platform.backbuffer_width, global_platform.backbuffer_height,
                      0, 0, backbuffer.width, backbuffer.height,
                      backbuffer.memory,
                      (const BITMAPINFO *)&backbuffer.bitmap_info,
                      DIB_RGB_COLORS, SRCCOPY);
        
        QueryPerformanceCounter(&end_frame_time_data);
        
        // NOTE(rjf): Wait, if necessary.
        {
            i64 frame_count = end_frame_time_data.QuadPart - start_frame_time_data.QuadPart;
            i64 desired_frame_count = (1 / performance_counter_frequency.QuadPart) / FRAMES_PER_SECOND;
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