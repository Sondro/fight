#define R_OFFSET 2
#define G_OFFSET 1
#define B_OFFSET 0

internal void
ClearBackbuffer(void)
{
    i32 pixel_index;
    
    for(i32 j = 0; j < platform->backbuffer_height; ++j)
    {
        for(i32 i = 0; i < platform->backbuffer_width; ++i)
        {
            pixel_index = j*platform->backbuffer_width + i;
            platform->backbuffer[pixel_index*3 + R_OFFSET] -= core->delta_t*platform->backbuffer[pixel_index*3 + R_OFFSET]*8.f;
            platform->backbuffer[pixel_index*3 + G_OFFSET] -= core->delta_t*platform->backbuffer[pixel_index*3 + G_OFFSET]*8.f;
            platform->backbuffer[pixel_index*3 + B_OFFSET] -= core->delta_t*platform->backbuffer[pixel_index*3 + B_OFFSET]*8.f;
        }
    }
}

internal void
DrawFilledRectangle(v2 position, v2 size, v3 color)
{
    iv2 lower_bound = {
        ClampI32((i32)position.x, 0, platform->backbuffer_width-1),
        ClampI32((i32)position.y, 0, platform->backbuffer_height-1),
    };
    
    iv2 upper_bound = {
        ClampI32((i32)position.x + size.x, 0, platform->backbuffer_width-1),
        ClampI32((i32)position.y + size.y, 0, platform->backbuffer_height-1),
    };
    
    i32 pixel_index;
    
    for(i32 j = lower_bound.y; j <= upper_bound.y; ++j)
    {
        for(i32 i = lower_bound.x; i <= upper_bound.x; ++i)
        {
            pixel_index = j*platform->backbuffer_width + i;
            platform->backbuffer[pixel_index*3 + R_OFFSET] = color.r * 255.f;
            platform->backbuffer[pixel_index*3 + G_OFFSET] = color.g * 255.f;
            platform->backbuffer[pixel_index*3 + B_OFFSET] = color.b * 255.f;
        }
    }
}

internal void
DrawFilledCircle(v2 position, f32 radius, v3 color)
{
    iv2 lower_bound = {
        ClampI32((i32)position.x - radius, 0, platform->backbuffer_width-1),
        ClampI32((i32)position.y - radius, 0, platform->backbuffer_height-1),
    };
    
    iv2 upper_bound = {
        ClampI32((i32)position.x + radius, 0, platform->backbuffer_width-1),
        ClampI32((i32)position.y + radius, 0, platform->backbuffer_height-1),
    };
    
    i32 pixel_index;
    f32 distance_squared;
    f32 radius_squared = radius*radius;
    
    for(i32 j = lower_bound.y; j <= upper_bound.y; ++j)
    {
        for(i32 i = lower_bound.x; i <= upper_bound.x; ++i)
        {
            distance_squared = (i - position.x)*(i - position.x) + (j - position.y)*(j - position.y);
            if(distance_squared <= radius_squared)
            {
                pixel_index = j*platform->backbuffer_width + i;
                platform->backbuffer[pixel_index*3 + R_OFFSET] = color.r * 255.f;
                platform->backbuffer[pixel_index*3 + G_OFFSET] = color.g * 255.f;
                platform->backbuffer[pixel_index*3 + B_OFFSET] = color.b * 255.f;
            }
        }
    }
}