typedef struct Sound
{
    u32 sample_count;
    i16 *samples;
    int channels;
    int sample_rate;
}
Sound;

internal Sound
SoundInitFromOGGData(void *data, u64 len);