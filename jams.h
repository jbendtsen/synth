#pragma once

#define N_NOTES 128

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

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
    u64 start;
    u64 end;
    u32 ins_data;
    u16 instrument;
    u8 in_vel;
    u8 out_vel;
} Note;

typedef struct {
    int time;
    int size;
    u8 *buf;
} Event;

typedef struct {
    Temp_Allocator alloc;
    Note notes[N_NOTES];
    Event *events;
    int n_events;
    int sample_rate;
    u64 position;
    u64 bend_pos;
    u16 bend;
    u8 ctrl_value;
} Playback;

typedef struct {
    float sample;
    float volume;
} Sample_Info;

typedef void (*Instrument)(Playback*, u64, int, Sample_Info*);

void init_synthesizer(Playback *play);
void synthesize(Playback *play, float *out, int n_samples);

Instrument *load_instruments(Playback *play, int *n_instruments);