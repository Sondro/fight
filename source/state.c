enum
{
    STATE_null,
#define StateType(name, type) STATE_##name,
#include "state_type_list.inc"
};

#include "state_title.c"
#include "state_game.c"

internal void
StateInit(i32 state_type, void *state_memory)
{
    switch(state_type)
    {
#define StateType(name, type) case STATE_##name: type##Init(state_memory); break;
#include "state_type_list.inc"
    }
}

internal void
StateUpdate(i32 state_type, void *state_memory)
{
    switch(state_type)
    {
#define StateType(name, type) case STATE_##name: type##Update(state_memory); break;
#include "state_type_list.inc"
    }
}

internal void
StateCleanUp(i32 state_type, void *state_memory)
{
    switch(state_type)
    {
#define StateType(name, type) case STATE_##name: type##CleanUp(state_memory); break;
#include "state_type_list.inc"
    }
}