#define LINUX_GAME_CODE_SO_NAME "project_snail.so"

#define GAME_CODE_LOAD(name)   void name(Platform *platform)
#define GAME_CODE_UNLOAD(name) void name(void)
#define GAME_INIT(name)        void name(Platform *platform)
#define GAME_CLEAN_UP(name)    void name(void)
#define GAME_UPDATE(name)      void name(void)

GAME_CODE_LOAD(game_code_load_stub) {}
GAME_CODE_UNLOAD(game_code_unload_stub) {}
GAME_INIT(game_init_stub) {}
GAME_CLEAN_UP(game_clean_up_stub) {}
GAME_UPDATE(game_update_stub) {}

typedef GAME_CODE_LOAD(GameCodeLoad);
typedef GAME_CODE_UNLOAD(GameCodeUnload);
typedef GAME_INIT(GameInit);
typedef GAME_CLEAN_UP(GameCleanUp);
typedef GAME_UPDATE(GameUpdate);

typedef struct LinuxGameCode {
    b32 valid;
    void *dynamic_library_handle;
    GameCodeLoad *game_code_load;
    GameCodeUnload *game_code_unload;
    GameInit *game_init;
    GameCleanUp *game_clean_up;
    GameUpdate *game_update;
#if BUILD_DEVELOPMENT
    struct stat dynamic_library_stat;
#endif
} LinuxGameCode;

internal void
linux_game_code_init(LinuxGameCode *game_code) {
    
    char dl_name[256] = {0};
    snprintf(dl_name, sizeof(dl_name), "%s", global_executable_directory, LINUX_GAME_CODE_SO_NAME);
    
#if BUILD_DEVELOPMENT
    
    char temp_dl_name[256] = {0};
    snprintf(temp_dl_name, 256, "%s.copy", dl_name);
    
    // Copy dynamic library to temporary file
    {
        FILE *source = fopen(dl_name, "r");
        
        if(source) {
            fseek(source, 0, SEEK_END);
            u64 source_size = ftell(source);
            rewind(source);
            void *source_data = malloc(source_size);
            fread(source_data, 1, source_size, source);
            fclose(source);
            
            FILE *dest = fopen(temp_dl_name, "w");
            if(dest) {
                fwrite(source_data, 1, source_size, dest);
                fclose(dest);
            }
            
            free(source_data);
        }
    }
    
    game_code->dynamic_library_handle = dlopen(temp_dl_name, RTLD_LAZY);
    
    sync();
    
#else
    
    game_code->dynamic_library_handle = dlopen(dl_name, RTLD_LAZY);
    
#endif
    
    
    if(game_code->dynamic_library_handle) {
        
        game_code->game_code_load = (GameCodeLoad *)dlsym(game_code->dynamic_library_handle,
                                                          "game_code_load");
        game_code->game_code_unload = (GameCodeUnload *)dlsym(game_code->dynamic_library_handle,
                                                              "game_code_unload");
        game_code->game_init = (GameInit *)dlsym(game_code->dynamic_library_handle,
                                                 "game_init");
        game_code->game_clean_up = (GameCleanUp *)dlsym(game_code->dynamic_library_handle,
                                                        "game_clean_up");
        game_code->game_update = (GameUpdate *)dlsym(game_code->dynamic_library_handle,
                                                     "game_update");
        
        if(game_code->game_code_load &&
           game_code->game_code_unload &&
           game_code->game_init &&
           game_code->game_clean_up &&
           game_code->game_update) {
            game_code->valid = 1;
        }
        else {
            game_code->valid = 0;
            game_code->game_code_load = game_code_load_stub;
            game_code->game_code_unload = game_code_unload_stub;
            game_code->game_init = game_init_stub;
            game_code->game_clean_up = game_clean_up_stub;
            game_code->game_update = game_update_stub;
        }
    }
    else {
        game_code->valid = 0;
        game_code->game_code_load = game_code_load_stub;
        game_code->game_code_unload = game_code_unload_stub;
        game_code->game_init = game_init_stub;
        game_code->game_clean_up = game_clean_up_stub;
        game_code->game_update = game_update_stub;
    }
}

internal void
linux_game_code_clean_up(LinuxGameCode *game_code) {
    if(game_code->dynamic_library_handle) {
        dlclose(game_code->dynamic_library_handle);
    }
    game_code->game_code_load = game_code_load_stub;
    game_code->game_code_unload = game_code_unload_stub;
    game_code->game_init = game_init_stub;
    game_code->game_clean_up = game_clean_up_stub;
    game_code->game_update = game_update_stub;
    game_code->valid = 0;
}

#if BUILD_DEVELOPMENT
internal void
linux_game_code_update(LinuxGameCode *game_code) {
    char dl_name[256] = {0};
    snprintf(dl_name, sizeof(dl_name), "%s%s", global_executable_directory, LINUX_GAME_CODE_SO_NAME);
    
    if(game_code->dynamic_library_handle) {
        struct stat dl_stat;
        stat(dl_name, &dl_stat);
        if(dl_stat.st_mtime != game_code->dynamic_library_stat.st_mtime) {
            game_code->dynamic_library_stat.st_mtime = dl_stat.st_mtime;
            game_code->game_code_unload();
            linux_game_code_clean_up(game_code);
            linux_game_code_init(game_code);
            game_code->game_code_load(&global_platform);
        }
    }
    else {
        linux_game_code_init(game_code);
    }
}
#endif