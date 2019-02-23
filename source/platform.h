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
}
Platform;