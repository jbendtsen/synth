#pragma once

typedef unsigned char u8;

typedef struct {
    u8 *buf;
    int size;
    int head;
} Temp_Allocator;

void talloc_init(Temp_Allocator *alloc, int max_size);
u8 *talloc_get(Temp_Allocator *alloc, int size);
void talloc_pad_to_next(Temp_Allocator *alloc, int align);
void talloc_close(Temp_Allocator *alloc);

typedef struct {
    int time;
    int size;
    u8 *buf;
} Event;

typedef struct {
    Temp_Allocator alloc;
    Event *events;
    int n_events;
    int sample_rate;
} Playback;

void synthesize(Playback *play, float *out, int n_samples);

