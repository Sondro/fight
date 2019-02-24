#if BUILD_LINUX_XLIB

// Xlib
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/keysym.h>

// Xlib + OpenGL Includes
#if RENDERER_BACKEND_OPENGL
#include <GL/glx.h>
#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
#endif

typedef struct LinuxWindowData {
    const char *window_system_name;
    i32 window_w;
    i32 window_h;
    
    Display *display;
    Window window;
    Window root;
    GLXContext gl_context;
} LinuxWindowData;

internal void *linux_window_data_load_gl_proc(const char *name) {
    void *addr = glXGetProcAddressARB((const GLubyte *)name);
    return addr;
}

internal int
linux_window_data_init(LinuxWindowData *window_data) {
    window_data->window_system_name = "Xlib";
    window_data->window_w = DEFAULT_WINDOW_WIDTH;
    window_data->window_h = DEFAULT_WINDOW_HEIGHT;
    
    int result = 0;
    window_data->display = XOpenDisplay(0);
    window_data->root = DefaultRootWindow(window_data->display);
    if(window_data->display) {
        
#if RENDERER_BACKEND_OPENGL
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
        
        global_platform.load_gl_proc = linux_window_data_load_gl_proc;
        
        int screen = DefaultScreen(window_data->display);
        
        int element_count;
        GLXFBConfig *framebuffer_config = glXChooseFBConfig(window_data->display, screen, gl_attributes, &element_count);
        if(!framebuffer_config) {
            goto exit;
        }
        
        XVisualInfo *visual_info = glXChooseVisual(window_data->display, screen, gl_attributes);
        
        if(!visual_info) {
            goto exit;
        }
        
        Colormap color_map = XCreateColormap(window_data->display,
                                             window_data->root,
                                             visual_info->visual,
                                             AllocNone);
        
        XSetWindowAttributes set_window_attributes = {0};
        set_window_attributes.colormap = color_map;
        set_window_attributes.event_mask = (KeyPressMask      |
                                            KeyReleaseMask    |
                                            PointerMotionMask |
                                            ButtonPressMask   |
                                            ButtonReleaseMask);
        
        const char *glx_extensions = glXQueryExtensionsString(window_data->display, screen);
        
        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        glXCreateContextAttribsARB = 
            (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
        
        window_data->window = XCreateWindow(window_data->display,
                                            window_data->root,
                                            0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, visual_info->depth, 
                                            InputOutput, visual_info->visual, CWColormap | CWEventMask, &set_window_attributes);
        
        
        GLint gl3_attributes[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
        };
        
        window_data->gl_context = glXCreateContextAttribsARB(window_data->display, *framebuffer_config,
                                                             0, GL_TRUE, gl3_attributes);
        
        if(!window_data->gl_context) {
            goto exit;
        }
        
        XFree(visual_info);
        
        if(!glXMakeCurrent(window_data->display,
                           window_data->window,
                           window_data->gl_context)) {
            goto exit;
        }
        
#else
#error "Unsupported Xlib renderer backend."
#endif
        
        XMapWindow(window_data->display, window_data->window);
        XStoreName(window_data->display, window_data->window, WINDOW_TITLE);
        XFlush(window_data->display);
        
        result = 1;
    }
    
    exit:;
    return result;
}

internal void
linux_window_data_clean_up(LinuxWindowData *window_data) {
    XDestroyWindow(window_data->display, window_data->window);
    XCloseDisplay(window_data->display);
}

internal void
linux_window_data_begin_frame(LinuxWindowData *window_data) {
    XEvent event;
    XLockDisplay(window_data->display);
    // NOTE(rjf): Update X11 events
    while(XPending(window_data->display) > 0) {
        XNextEvent(window_data->display, &event);
        {
            switch(event.type) {
                case ButtonPress: {
                    if(event.xbutton.button == Button1) {
                        global_platform.left_mouse_state |= 1<<31;
                    }
                    else if(event.xbutton.button == Button3) {
                        global_platform.right_mouse_state |= 1<<31;
                    }
                    else if(event.xbutton.button == Button4) {
                        global_platform.mouse_z = -1;
                    }
                    else if(event.xbutton.button == Button5) {
                        global_platform.mouse_z = 1;
                    }
                    break;
                }
                case ButtonRelease: {
                    if(event.xbutton.button == Button1) {
                        global_platform.left_mouse_state &= ~(1<<31);
                    }
                    else if(event.xbutton.button == Button3) {
                        global_platform.right_mouse_state &= ~(1<<31);
                    }
                    break;
                }
                case MotionNotify: {
                    global_platform.mouse_x = (f32)event.xmotion.x;
                    global_platform.mouse_y = (f32)event.xmotion.y;
                    break;
                }
                case KeyPress:
                case KeyRelease: {
                    
                    b32 down = event.type == KeyPress;
                    
                    KeySym keysym = XLookupKeysym(&event.xkey, 0);
                    
                    i32 key_index = -1;
                    
                    if(keysym >= XK_a &&
                       keysym <= XK_z) {
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
                    else if(keysym == XK_BackSpace) {
                        key_index = KEY_backspace;
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
                    else if(keysym == XK_Tab) {
                        key_index = KEY_tab;
                    }
                    
                    if(key_index >= 0 && key_index < KEY_MAX) {
                        if(down) {
                            global_platform.key_pressed[key_index] = (!global_platform.key_down[key_index]);
                        }
                        else {
                            global_platform.key_pressed[key_index] = 0;
                        }
                        
                        global_platform.key_down[key_index] = down;
                        
                        global_platform.last_key_pressed = (i32)key_index;
                    }
                    
                    if(global_platform.target_input_text && down) {
                        char input_text[8] = {0};
                        int length;
                        KeySym keysym_return;
                        length = XLookupString(&event.xkey, input_text, sizeof(input_text), &keysym_return, 0);
                        
                        char *text = global_platform.target_input_text;
                        u32 edit_position = strlen(text);
                        u32 cap = global_platform.target_input_text_cap;
                        if(key_index == KEY_backspace) {
                            if(global_platform.key_down[KEY_ctrl]) {
                                for(i32 i = (i32)edit_position-2; i >= 0; --i) {
                                    if(!i) {
                                        text[i] = 0;
                                    }
                                    else {
                                        if(text[i] == ' ') {
                                            text[i+1] = 0;
                                            break;
                                        }
                                    }
                                }
                            }
                            else {
                                if(edit_position > 0) {
                                    text[edit_position-1] = 0;
                                }
                            }
                        }
                        else {
                            for(u32 i = 0; i < sizeof(input_text) && edit_position < cap; ++i) {
                                if(input_text[i] >= 32 && input_text[i] <= 126) {
                                    text[edit_position++] = input_text[i];
                                    text[edit_position] = 0;
                                }
                            }
                        }
                    }
                    
                    break;
                }
                
                default: break;
            }
        }
    }
    XUnlockDisplay(window_data->display);
    
}

internal void
linux_window_data_end_frame(LinuxWindowData *window_data) {
    glXSwapBuffers(window_data->display, window_data->window);
}


#endif
