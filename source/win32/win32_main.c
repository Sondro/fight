#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <windowsx.h>
#include <xinput.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <gl/gl.h>
#include "ext/wglext.h"
#include "ext/glext.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "ext/stb_image.h"
#include "ext/stb_vorbis.c"

#include "program_options.h"
#include "language_layer.h"
#include "platform.h"

#include "game.c"

global Platform global_platform = {0};
global HWND global_window = {0};
global HGLRC global_opengl_render_context = {0};

static const GUID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2};
static const GUID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2};
static const GUID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E};
static const GUID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6};
static const GUID PcmSubformatGuid = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};

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

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif

#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

#define SOUND_LATENCY_FPS 15
#define REFTIMES_PER_SEC 10000000

#define CO_CREATE_INSTANCE(name) HRESULT name(REFCLSID rclsid, LPUNKNOWN *pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
typedef CO_CREATE_INSTANCE(CoCreateInstance_);
CO_CREATE_INSTANCE(CoCreateInstanceStub)
{
    return 1;
}
global CoCreateInstance_ *CoCreateInstanceProc = CoCreateInstanceStub;

#define CO_INITIALIZE_EX(name) HRESULT name(LPVOID pvReserved, DWORD dwCoInit)
typedef CO_INITIALIZE_EX(CoInitializeEx_);
CO_INITIALIZE_EX(CoInitializeExStub)
{
    return 1;
}
global CoInitializeEx_ *CoInitializeExProc = CoInitializeExStub;

typedef struct Win32SoundOutput
{
    b32 initialized;
    
    IMMDeviceEnumerator *device_enum;
    IMMDevice *device;
    IAudioClient *audio_client;
    IAudioRenderClient *audio_render_client;
    REFERENCE_TIME sound_buffer_duration;
    u32 buffer_frame_count;
    u32 channels;
    u32 samples_per_second;
    u32 latency_frame_count;
}
Win32SoundOutput;

internal void
Win32LoadWASAPI()
{
    HMODULE wasapi_lib = LoadLibraryA("ole32.dll");
    if(wasapi_lib)
    {
        CoCreateInstanceProc = (CoCreateInstance_ *)GetProcAddress(wasapi_lib, "CoCreateInstance");
        CoInitializeExProc = (CoInitializeEx_ *)GetProcAddress(wasapi_lib, "CoInitializeEx");
    }
    else
    {
        CoCreateInstanceProc = CoCreateInstanceStub;
        CoInitializeExProc = CoInitializeExStub;
    }
}

internal void
Win32InitWASAPI(Win32SoundOutput *output)
{
    CoInitializeExProc(0, COINIT_SPEED_OVER_MEMORY);
    
    REFERENCE_TIME requested_sound_duration = REFTIMES_PER_SEC * 2;
    
    HRESULT result;
    
    result = CoCreateInstanceProc(&CLSID_MMDeviceEnumerator,
                                  0,
                                  CLSCTX_ALL,
                                  &IID_IMMDeviceEnumerator,
                                  (LPVOID *)(&output->device_enum));
    if(result == S_OK)
    {
        
        output->device_enum->lpVtbl->GetDefaultAudioEndpoint(output->device_enum,
                                                             eRender,
                                                             eConsole,
                                                             &output->device);
        if(result == S_OK)
        {
            
            result = output->device->lpVtbl->Activate(output->device,
                                                      &IID_IAudioClient,
                                                      CLSCTX_ALL,
                                                      0,
                                                      (void **)&output->audio_client);
            if(result == S_OK)
            {
                WAVEFORMATEX *wave_format = 0;
                
                output->audio_client->lpVtbl->GetMixFormat(output->audio_client, &wave_format);
                
                output->samples_per_second = 44100;//wave_format->nSamplesPerSec;
                WORD bits_per_sample = sizeof(i16)*8;
                WORD block_align = (output->channels * bits_per_sample) / 8;
                DWORD average_bytes_per_second = block_align * output->samples_per_second;
                WORD cb_size = 0;
                
                WAVEFORMATEX new_wave_format = {
                    WAVE_FORMAT_PCM,
                    (WORD)output->channels,
                    output->samples_per_second,
                    average_bytes_per_second,
                    block_align,
                    bits_per_sample,
                    cb_size,
                };
                
                result = output->audio_client->lpVtbl->Initialize(output->audio_client,
                                                                  AUDCLNT_SHAREMODE_SHARED,
                                                                  AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                                                  requested_sound_duration,
                                                                  0,
                                                                  &new_wave_format,
                                                                  0);
                
                output->latency_frame_count = output->samples_per_second / SOUND_LATENCY_FPS;
                
                if(result == S_OK)
                {
                    
                    result = output->audio_client->lpVtbl->GetService(output->audio_client,
                                                                      &IID_IAudioRenderClient,
                                                                      (void **)&output->audio_render_client);
                    
                    if(result == S_OK)
                    {
                        // NOTE(Ryan): Audio initialization was successful
                        
                        output->audio_client->lpVtbl->GetBufferSize(output->audio_client, &output->buffer_frame_count);
                        
                        output->sound_buffer_duration = (REFERENCE_TIME)((f64)REFTIMES_PER_SEC *
                                                                         output->buffer_frame_count / output->samples_per_second);
                        
                        output->audio_client->lpVtbl->Start(output->audio_client);
                        
                        output->initialized = 1;
                    }
                    else
                    {
                        Win32MessageBox("WASAPI Error", "Request for audio render service failed.");
                    }
                }
                else
                {
                    Win32MessageBox("WASAPI Error",
                                    "Audio client initialization failed.");
                }
            }
            else
            {
                Win32MessageBox("WASAPI Error", "Could not activate audio device.");
            }
        }
        else
        {
            Win32MessageBox("WASAPI Error", "Default audio endpoint was not found.");
        }
    }
    else
    {
        Win32MessageBox("WASAPI Error", "Device enumerator retrieval failed.");
    }
}

internal void
Win32CleanUpWASAPI(Win32SoundOutput *output)
{
    if(output->initialized)
    {
        output->audio_client->lpVtbl->Stop(output->audio_client);
        
        output->device_enum->lpVtbl->Release(output->device_enum);
        output->device->lpVtbl->Release(output->device);
        output->audio_client->lpVtbl->Release(output->audio_client);
        output->audio_render_client->lpVtbl->Release(output->audio_render_client);
    }
}

internal void
Win32FillSoundBuffer(u32 samples_to_write, i16 *samples, Win32SoundOutput *output)
{
    if(samples_to_write)
    {
        BYTE *data = 0;
        DWORD flags = 0;
        
        output->audio_render_client->lpVtbl->GetBuffer(output->audio_render_client, samples_to_write, &data);
        if(data)
        {
            i16 *destination = (i16 *)data;
            i16 *source = samples;
            for(UINT32 i = 0; i < samples_to_write; ++i)
            {
                *destination++ = *source++; // left sample
                *destination++ = *source++; // right sample
            }
        }
        output->audio_render_client->lpVtbl->ReleaseBuffer(output->audio_render_client, samples_to_write, flags);
    }
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
    
    // NOTE(rjf): Initialize sound output
    Win32SoundOutput sound_output = {0};
    {
        sound_output.channels = 2;
        sound_output.samples_per_second = 48000;
        sound_output.latency_frame_count = 48000;
        Win32LoadWASAPI();
        Win32InitWASAPI(&sound_output);
    }
    
    // NOTE(rjf): Initialize platform struct
    {
        global_platform.sample_out = VirtualAlloc(0, sound_output.samples_per_second * sizeof(i16) * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.permanent_storage_size = PERMANENT_STORAGE_SIZE;
        global_platform.scratch_storage_size = SCRATCH_STORAGE_SIZE;
        global_platform.permanent_storage = VirtualAlloc(0, global_platform.permanent_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.scratch_storage = VirtualAlloc(0, global_platform.scratch_storage_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        global_platform.LoadOpenGLProcedure = Win32OpenGLProcAddress;
        global_platform.OutputError = Win32MessageBox;
        global_platform.LoadEntireFile = Win32LoadEntireFile;
        global_platform.FreeFileData = Win32FreeFileData;
        
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
        
        // NOTE(rjf): Find how much sound to write and where
        if(sound_output.initialized)
        {
            global_platform.sample_count_to_output = 0;
            UINT32 sound_padding_size;
            if(SUCCEEDED(sound_output.audio_client->lpVtbl->GetCurrentPadding(sound_output.audio_client,
                                                                              &sound_padding_size)))
            {
                
                global_platform.samples_per_second = sound_output.samples_per_second;
                
                global_platform.sample_count_to_output = 
                    (u32)(sound_output.latency_frame_count - sound_padding_size);
                
                if(global_platform.sample_count_to_output > sound_output.latency_frame_count)
                {
                    global_platform.sample_count_to_output = sound_output.latency_frame_count;
                }
            }
            
            for(u32 i = 0; i < sound_output.buffer_frame_count; ++i)
            {
                global_platform.sample_out[i] = 0;
            }
        }
        
        GameUpdate();
        wglSwapLayerBuffers(window_device_context, WGL_SWAP_MAIN_PLANE);
        
        // Fill sound buffer with game sound
        if(sound_output.initialized)
        {
            Win32FillSoundBuffer(global_platform.sample_count_to_output,
                                 global_platform.sample_out,
                                 &sound_output);
        }
        
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