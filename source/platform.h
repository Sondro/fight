enum
{
#define Key(name, str) KEY_##name,
#include "platform_key_list.inc"
    KEY_MAX
};

typedef struct Platform
{
    b32 quit;
    
    u32 permanent_storage_size;
    void *permanent_storage;
    u32 scratch_storage_size;
    void *scratch_storage;
    
    u32 backbuffer_width;
    u32 backbuffer_height;
    u8 *backbuffer;
    
    f32 mouse_x;
    f32 mouse_y;
    b32 key_down[KEY_MAX];
    b32 key_pressed[KEY_MAX];
}
Platform;