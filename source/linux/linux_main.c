#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include <time.h>
#include <GL/glx.h>
#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

#include "program_options.h"
#include "language_layer.h"
#include "platform.h"

#include "game.c"

global Platform global_platform = {0};

internal void
LinuxMessage(const char *title, const char *format, ...)
{
    fprintf(stderr, "%s: ", title);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

internal void *
LinuxLoadOpenGLProc(const char *name)
{
    void *addr = glXGetProcAddressARB((const GLubyte *)name);
    return addr;
}

int
main(int argument_count, char **arguments)
{
    
    Display *display;
    Window window;
    Window root;
    GLXContext gl_context;
    
    // NOTE(rjf): Window initialization
    {
        display = XOpenDisplay(0);
        root = DefaultRootWindow(display);
        if(!display)
        {
            LinuxMessage("Fatal Error", "Could not open Xlib display.");
            goto quit;
        }
        
        GLint gl_attributes[] = {
            GLX_RENDER_TYPE     , GLX_RGBA_BIT,
            GLX_RED_SIZE        , 8,
            GLX_GREEN_SIZE      , 8,
            GLX_BLUE_SIZE       , 8,
            GLX_ALPHA_SIZE      , 8,
            GLX_DEPTH_SIZE      , 24,
            GLX_DOUBLEBUFFER    , True,
            None
        };
        
        int screen = DefaultScreen(display);
        
        int element_count;
        GLXFBConfig *framebuffer_config = glXChooseFBConfig(display, screen, gl_attributes, &element_count);
        if(!framebuffer_config)
        {
            goto quit;
        }
        
        XVisualInfo *visual_info = glXChooseVisual(display, screen, gl_attributes);
        
        if(!visual_info)
        {
            goto quit;
        }
        
        Colormap color_map = XCreateColormap(display,
                                             root,
                                             visual_info->visual,
                                             AllocNone);
        
        XSetWindowAttributes set_window_attributes = {0};
        set_window_attributes.colormap = color_map;
        set_window_attributes.event_mask = (KeyPressMask      |
                                            KeyReleaseMask    |
                                            PointerMotionMask |
                                            ButtonPressMask   |
                                            ButtonReleaseMask);
        
        const char *glx_extensions = glXQueryExtensionsString(display, screen);
        
        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        glXCreateContextAttribsARB = 
            (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
        
        window = XCreateWindow(display,
                               root,
                               0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, visual_info->depth, 
                               InputOutput, visual_info->visual, CWColormap | CWEventMask, &set_window_attributes);
        
        
        GLint gl3_attributes[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
        };
        
        gl_context = glXCreateContextAttribsARB(display, *framebuffer_config,
                                                0, GL_TRUE, gl3_attributes);
        
        if(!gl_context)
        {
            goto quit;
        }
        
        XFree(visual_info);
        
        if(!glXMakeCurrent(display,
                           window,
                           gl_context))
        {
            goto quit;
        }
        
        XMapWindow(display, window);
        XStoreName(display, window, WINDOW_TITLE);
        XFlush(display);
    }
    
    // NOTE(rjf): Initialize platform struct
    {
        global_platform.permanent_storage_size = PERMANENT_STORAGE_SIZE;
        global_platform.scratch_storage_size = SCRATCH_STORAGE_SIZE;
        global_platform.permanent_storage = calloc(1, global_platform.permanent_storage_size);
        global_platform.scratch_storage = calloc(1, global_platform.scratch_storage_size);
        global_platform.LoadOpenGLProcedure = LinuxLoadOpenGLProc;
        global_platform.OutputError = LinuxMessage;
        
        if(!global_platform.permanent_storage || !global_platform.scratch_storage)
        {
            LinuxMessage("Fatal Error", "Application memory allocation failure.");
            goto quit;
        }
    }
    
    GameInit(&global_platform);
    
    struct timespec begin_frame_time_spec;
    struct timespec end_frame_time_spec;
    i64 desired_frame_nanoseconds = 1;
    clock_gettime(CLOCK_MONOTONIC, &end_frame_time_spec);
    
    while(!global_platform.quit)
    {
        // NOTE(rjf): Get beginning of frame time
        {
            clock_gettime(CLOCK_MONOTONIC, &begin_frame_time_spec);
            desired_frame_nanoseconds = (i64)((1.f / FRAMES_PER_SECOND) * 1000000000);
        }
        
        
        // NOTE(rjf): Handle window events
        {
            XEvent event;
            XLockDisplay(display);
            // NOTE(rjf): Update X11 events
            while(XPending(display) > 0)
            {
                XNextEvent(display, &event);
                {
                    switch(event.type)
                    {
                        case ButtonPress:
                        {
                            if(event.xbutton.button == Button1)
                            {
                                // global_platform.left_mouse_state |= 1<<31;
                            }
                            else if(event.xbutton.button == Button3)
                            {
                                // global_platform.right_mouse_state |= 1<<31;
                            }
                            else if(event.xbutton.button == Button4)
                            {
                                // global_platform.mouse_z = -1;
                            }
                            else if(event.xbutton.button == Button5)
                            {
                                // global_platform.mouse_z = 1;
                            }
                            break;
                        }
                        case ButtonRelease:
                        {
                            if(event.xbutton.button == Button1)
                            {
                                // global_platform.left_mouse_state &= ~(1<<31);
                            }
                            else if(event.xbutton.button == Button3)
                            {
                                // global_platform.right_mouse_state &= ~(1<<31);
                            }
                            break;
                        }
                        case MotionNotify:
                        {
                            global_platform.mouse_x = (f32)event.xmotion.x;
                            global_platform.mouse_y = (f32)event.xmotion.y;
                            break;
                        }
                        case KeyPress:
                        case KeyRelease:
                        {
                            
                            b32 down = event.type == KeyPress;
                            
                            KeySym keysym = XLookupKeysym(&event.xkey, 0);
                            
                            i32 key_index = -1;
                            
                            if(keysym >= XK_a && keysym <= XK_z) {
                                key_index = KEY_a + (keysym-XK_a);
                            }
                            else if(keysym == XK_space) {
                                key_index = KEY_space;
                            }
                            else if(keysym == XK_Shift_L || keysym == XK_Shift_R) {
                                key_index = KEY_shift;
                            }
                            else if(keysym == XK_Control_L || keysym == XK_Control_R) {
                                key_index = KEY_ctrl;
                            }
                            else if(keysym == XK_Escape) {
                                key_index = KEY_escape;
                            }
                            else if(keysym == XK_Return) {
                                key_index = KEY_enter;
                            }
                            else if(keysym == XK_Up) {
                                key_index = KEY_up;
                            }
                            else if(keysym == XK_Down) {
                                key_index = KEY_down;
                            }
                            else if(keysym == XK_Left) {
                                key_index = KEY_left;
                            }
                            else if(keysym == XK_Right) {
                                key_index = KEY_right;
                            }
                            
                            if(key_index >= 0 && key_index < KEY_MAX) {
                                if(down) {
                                    global_platform.key_pressed[key_index] = (!global_platform.key_down[key_index]);
                                }
                                else {
                                    global_platform.key_pressed[key_index] = 0;
                                }
                                
                                global_platform.key_down[key_index] = down;
                            }
                            
                            break;
                        }
                        
                        default: break;
                    }
                }
            }
            XUnlockDisplay(display);
        }
        
        GameUpdate();
        glXSwapBuffers(display, window);
        
        clock_gettime(CLOCK_MONOTONIC, &end_frame_time_spec);
        
        // NOTE(rjf): End frame, and wait if necessary.
        {
            i64 elapsed_seconds = end_frame_time_spec.tv_sec - begin_frame_time_spec.tv_sec;
            i64 elapsed_nanoseconds = end_frame_time_spec.tv_nsec - begin_frame_time_spec.tv_nsec;
            i64 frame_nanoseconds = elapsed_seconds * 1000000000 + elapsed_nanoseconds;
            i64 nanoseconds_to_wait = desired_frame_nanoseconds - frame_nanoseconds;
            i64 last_step = 0;
            struct timespec begin_wait_time_spec;
            struct timespec end_wait_time_spec;
            clock_gettime(CLOCK_MONOTONIC, &begin_wait_time_spec);
            while(nanoseconds_to_wait > last_step)
            {
                clock_gettime(CLOCK_MONOTONIC, &end_wait_time_spec);
                i64 wait_seconds = end_wait_time_spec.tv_sec - begin_wait_time_spec.tv_sec;
                i64 wait_nanoseconds = end_wait_time_spec.tv_nsec - begin_wait_time_spec.tv_nsec;
                last_step = (wait_seconds * 1000000000 + wait_nanoseconds);
                nanoseconds_to_wait -= last_step;
                begin_wait_time_spec = end_wait_time_spec;
            }
        }
    }
    
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    
    quit:;
    return 0;
}