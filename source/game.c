
typedef struct Core
{
    Platform *platform;
}
Core;

global Core *core = 0;

internal void
GameInit(Platform *platform)
{
    core = platform->permanent_storage;
    core->platform = platform;
}

internal void
GameUpdate(void)
{
    
}