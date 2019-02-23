#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "program_options.h"
#include "language_layer.h"
#include "platform.h"

#include "game.c"

global Platform global_platform = {0};
global HWND global_window = {0};

internal LRESULT
Win32WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = {0};
    
    if(message == WM_DESTROY)
    {
        global_platform.quit = 1;
    }
    else
    {
        result = DefWindowProc(window, message, wParam, lParam);
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
    
    // NOTE(rjf): Initialize platform struct
    {
        global_platform.permanent_storage_size = PERMANENT_STORAGE_SIZE;
        global_platform.scratch_storage_size = SCRATCH_STORAGE_SIZE;
        global_platform.permanent_storage = VirtualAlloc(0, global_platform.permanent_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.scratch_storage = VirtualAlloc(0, global_platform.scratch_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        if(!global_platform.permanent_storage ||
           !global_platform.scratch_storage)
        {
            Win32MessageBox("Fatal Error", "Application memory allocation failure.");
            goto quit;
        }
    }
    
    GameInit(&global_platform);
    
    ShowWindow(global_window, command_show);
    
    while(!global_platform.quit)
    {
        MSG message;
        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        
        GameUpdate();
    }
    
    quit:;
    return 0;
}