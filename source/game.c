
typedef struct Core
{
    Platform *platform;
}
Core;

global Core *core = 0;
global Platform *platform = 0;

internal void
GameInit(Platform *platform_)
{
    core = platform_->permanent_storage;
    core->platform = platform_;
    platform = platform_;
}

internal void
GameUpdate(void)
{
    
}