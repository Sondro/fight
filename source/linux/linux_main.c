// Linux
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

// Application Headers
#include "program_options.h"
#include "language_layer.h"
#include "platform.h"

#include "game.c"

// Globals
global Platform global_platform = {0};
global char global_executable_directory[256] = {0};
global char global_read_data_path[256] = {0};
global char global_write_data_path[256] = {0};


internal f64
linux_get_current_time(void) {
    struct timespec current_time_spec;
    clock_gettime(CLOCK_MONOTONIC, &current_time_spec);
    i64 seconds = current_time_spec.tv_sec;
    i64 nanoseconds = current_time_spec.tv_nsec;
    f64 result = (f64)seconds + nanoseconds / 1000000000.0;
    return result;
}

internal void
linux_output_error(const char *title, const char *format, ...) {
    fprintf(stderr, "[ERROR] %s: ", title);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

//Initialize OpenGL
internal b32
LinuxInitOpenGL(){
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
    
    result = 1;
    
    
    exit:;
    return result;
}

internal void
LinuxCleanUpOpenGL(){
    
}

int
main(int argument_count, char **arguments) {
    LinuxWindowData  linux_window_data = {0};
    LinuxGameCode    linux_game_code   = {0};
    
    // NOTE(rjf): Find executable directory
    {
        const char *executable_path = arguments[0];
        strncpy(global_executable_directory, executable_path, 255);
        char *one_past_last_slash = global_executable_directory;
        for(u32 i = 0; global_executable_directory[i]; ++i) {
            if(global_executable_directory[i] == '/' ||
               global_executable_directory[i] == '\\') {
                one_past_last_slash = global_executable_directory+i+1;
            }
        }
        *one_past_last_slash = 0;
    }
    
    // NOTE(rjf): Make read/write data paths
    /*{
        snprintf(global_read_data_path, sizeof(global_read_data_path),
                 "%s", global_executable_directory, "data/");
        snprintf(global_write_data_path, sizeof(global_write_data_path),
                 "%s", global_executable_directory, "data/");
    }*/
    
    if(linux_window_data_init(&linux_window_data)) {
        linux_game_code_init(&linux_game_code);
        if(linux_game_code.valid) {
            
            // NOTE(rjf): Initialize platform
            {
                global_platform.permanent_memory_size = PERMANENT_MEMORY_SIZE;
                global_platform.permanent_memory = calloc(1, global_platform.permanent_memory_size);
                global_platform.frame_memory_size = FRAME_MEMORY_SIZE;
                global_platform.frame_memory = calloc(1, global_platform.frame_memory_size);
                global_platform.load_entire_file_from_read_data = linux_load_entire_file_from_read_data;
                global_platform.load_entire_file_from_write_data = linux_load_entire_file_from_write_data;
                global_platform.write_to_file = linux_write_to_file;
                global_platform.get_current_time = linux_get_current_time;
            }
            
            linux_game_code.game_code_load(&global_platform);
            linux_game_code.game_init(&global_platform);
            
            struct timespec begin_frame_time_spec;
            struct timespec end_frame_time_spec;
            i64 desired_frame_nanoseconds = 1;
            clock_gettime(CLOCK_MONOTONIC, &end_frame_time_spec);
            
            while(!global_platform.quit) {
                // NOTE(rjf): Get beginning of frame time
                {
                    clock_gettime(CLOCK_MONOTONIC, &begin_frame_time_spec);
                    global_platform.last_time = global_platform.current_time;
                    global_platform.current_time += 1.f / global_platform.fps;
                    desired_frame_nanoseconds = (i64)((1.f / global_platform.fps) * 1000000000);
                }
                
                linux_window_data_begin_frame(&linux_window_data);
                global_platform.window_w = linux_window_data.window_w;
                global_platform.window_h = linux_window_data.window_h;
                linux_game_code.game_update();
                
                linux_window_data_end_frame(&linux_window_data);
                
                // NOTE(rjf): End frame, and wait if necessary.
                {
                    clock_gettime(CLOCK_MONOTONIC, &end_frame_time_spec);
                    i64 elapsed_seconds = end_frame_time_spec.tv_sec - begin_frame_time_spec.tv_sec;
                    i64 elapsed_nanoseconds = end_frame_time_spec.tv_nsec - begin_frame_time_spec.tv_nsec;
                    i64 frame_nanoseconds = elapsed_seconds * 1000000000 + elapsed_nanoseconds;
                    i64 nanoseconds_to_wait = desired_frame_nanoseconds - frame_nanoseconds;
                    i64 last_step = 0;
                    struct timespec begin_wait_time_spec;
                    struct timespec end_wait_time_spec;
                    clock_gettime(CLOCK_MONOTONIC, &begin_wait_time_spec);
                    while(nanoseconds_to_wait > last_step) {
                        clock_gettime(CLOCK_MONOTONIC, &end_wait_time_spec);
                        i64 wait_seconds = end_wait_time_spec.tv_sec - begin_wait_time_spec.tv_sec;
                        i64 wait_nanoseconds = end_wait_time_spec.tv_nsec - begin_wait_time_spec.tv_nsec;
                        last_step = (wait_seconds * 1000000000 + wait_nanoseconds);
                        nanoseconds_to_wait -= last_step;
                        begin_wait_time_spec = end_wait_time_spec;
                    }
                }
            }
            
        }
        
        linux_window_data_clean_up(&linux_window_data);
    }
    else {
        linux_output_error("Window Creation Failure", "%s window could not be created.",
                           linux_window_data.window_system_name);
    }
    
    
    XMapWindow(window_data->display, window_data->window);
    XStoreName(window_data->display, window_data->window, WINDOW_TITLE);
    XFlush(window_data->display);
    
    return 0;
}