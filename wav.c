#include "jams.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char riff[4];
    int chunk_size;
    char wave[4];
    char sub1_id[4];
    int sub1_size;
    short format;
    short n_channels;
    int sample_rate;
    int byte_rate;
    short block_align;
    short bits_per_sample;
    char sub2_id[4];
    int sub2_size;
} Wav_Header;

Track read_wav_file_as_mono(const char *fname)
{
    Track track = {0};
    FILE *f = fopen(fname, "rb");
    if (!f) {
        printf("wav.c: file not found\n");
        return track;
    }

    fseek(f, 0, SEEK_END);
    int sz = ftell(f);
    rewind(f);

    if (sz <= sizeof(Wav_Header)) {
        printf("wav.c: file too small\n");
        fclose(f);
        return track;
    }

    Wav_Header header;
    fread((void*)&header, 1, sizeof(Wav_Header), f);

    if (memcmp(header.riff, "RIFF", 4) ||
        memcmp(header.wave, "WAVE", 4) ||
        memcmp(header.sub1_id, "fmt ", 4)
    ) {
        printf("wav.c: not recognised as WAV (%c%c%c%c vs RIFF, %c%c%c%c vs WAVE, %c%c%c%c vs fmt )\n",
            header.riff[0], header.riff[1], header.riff[2], header.riff[3],
            header.wave[0], header.wave[1], header.wave[2], header.wave[3],
            header.sub1_id[0], header.sub1_id[1], header.sub1_id[2], header.sub1_id[3]
        );
        fclose(f);
        return track;
    }

    u32 extra[32];
    char *h = &header.sub2_id[0];
    int idx = 0;
    while (h[0] != 'd' || h[1] != 'a' || h[2] != 't' || h[3] != 'a') {
        if ((idx & 31) == 0) {
            if (fread(extra, 1, 128, f) != 128) {
                printf("wav.c: could not find data section (%d)\n", idx);
                return track;
            }
        }
        *(u64*)h = *(u64*)(&extra[idx++ & 31]);
        if ((idx & 31) == 31)
            idx++;
    }

    fseek(f, sizeof(Wav_Header), SEEK_SET);

    if (header.n_channels <= 0 || header.sample_rate <= 0 || header.sub2_size <= 0) {
        printf("wav.c: invalid header params (n_channels=%d, sample_rate=%d, sub2_size=%d)\n",
            header.n_channels, header.sample_rate, header.sub2_size);
        fclose(f);
        return track;
    }

    int data_sz = sz - sizeof(Wav_Header);
    if (data_sz > header.sub2_size)
        data_sz = header.sub2_size;

    int sample_w = 4;
    int is_float = header.format != 1;
    if (!is_float) {
        sample_w = header.bits_per_sample / 8;
        if (sample_w != 1 && sample_w != 2) {
            printf("wav.c: unsupported bits per sample (%d)\n", header.bits_per_sample);
            fclose(f);
            return track;
        }
    }

    u8 *buf = malloc(data_sz);
    fread(buf, 1, data_sz, f);
    fclose(f);

    int n_samples = data_sz / (sample_w * header.n_channels);
    float *out = (float*)malloc(n_samples * sizeof(float));
    float *p = out;

    int off = 0;
    if (is_float) {
        float *in = (float*)buf;
        for (int i = 0; i < n_samples; i++) {
            float sample = 0.0;
            for (int j = 0; j < header.n_channels; j++) {
                sample += *in++;
                off += sizeof(float);
            }
            *p++ = sample / (float)header.n_channels;
        }
    }
    else {
        char *in_8 = (char*)buf;
        short *in_16 = (short*)buf;
        for (int i = 0; i < n_samples; i++) {
            float sample = 0.0;
            for (int j = 0; j < header.n_channels; j++) {
                sample += sample_w == 1 ? (float)*in_8 / 128.0 : (float)*in_16 / 32768.0;
                in_8++;
                in_16++;
            }
            *p++ = sample / (float)header.n_channels;
        }
    }

    free(buf);

    track.buf = out;
    track.len = n_samples;
    track.rate = header.sample_rate;
    return track;
}

void close_track(Track *t)
{
    if (t->buf)
        free(t->buf);

    memset(t, 0, sizeof(Track));
}