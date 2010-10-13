////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wv_filter.c

// This is the main module for the demonstration WavPack command-line
// decoder filter. It uses the tiny "hardware" version of the decoder and
// accepts WavPack files on stdin and outputs a standard MS wav file to
// stdout. Note that this involves converting the data to little-endian
// (if the executing processor is not), possibly packing the data into
// fewer bytes per sample, and generating an appropriate riff wav header.
// Note that this is NOT the copy of the RIFF header that might be stored
// in the file, and any additional RIFF information and tags are lost.
// See wputils.c for further limitations.

#include "wavpack.h"

#if defined(WIN32)
#include <io.h>
#include <fcntl.h>
#endif

#include <string.h>

// These structures are used to place a wav riff header at the beginning of
// the output.

typedef struct {
    char ckID [4];
    uint32_t ckSize;
    char formType [4];
} RiffChunkHeader;

typedef struct {
    char ckID [4];
    uint32_t ckSize;
} ChunkHeader;

#define ChunkHeaderFormat "4L"

typedef struct {
    ushort FormatTag, NumChannels;
    uint32_t SampleRate, BytesPerSecond;
    ushort BlockAlign, BitsPerSample;
} WaveHeader;

#define WaveHeaderFormat "SSLLSS"

static uchar *format_samples (int bps, uchar *dst, int32_t *src, uint32_t samcnt);
static int32_t read_bytes (void *buff, int32_t bcount);
static int32_t temp_buffer [256];

int main ()
{
    ChunkHeader FormatChunkHeader, DataChunkHeader;
    RiffChunkHeader RiffChunkHeader;
    WaveHeader WaveHeader;

    uint32_t total_unpacked_samples = 0, total_samples;
    int num_channels, bps;
    WavpackContext *wpc;
    char error [80];

#if defined(WIN32)
    setmode (fileno (stdin), O_BINARY);
    setmode (fileno (stdout), O_BINARY);
#endif

    wpc = WavpackOpenFileInput (read_bytes, error);

    if (!wpc) {
        fputs (error, stderr);
        fputs ("\n", stderr);
        return 1;
    }

    num_channels = WavpackGetReducedChannels (wpc);
    total_samples = WavpackGetNumSamples (wpc);
    bps = WavpackGetBytesPerSample (wpc);

    strncpy (RiffChunkHeader.ckID, "RIFF", sizeof (RiffChunkHeader.ckID));
    RiffChunkHeader.ckSize = total_samples * num_channels * bps + sizeof (ChunkHeader) * 2 + sizeof (WaveHeader) + 4;
    strncpy (RiffChunkHeader.formType, "WAVE", sizeof (RiffChunkHeader.formType));

    strncpy (FormatChunkHeader.ckID, "fmt ", sizeof (FormatChunkHeader.ckID));
    FormatChunkHeader.ckSize = sizeof (WaveHeader);

    WaveHeader.FormatTag = 1;
    WaveHeader.NumChannels = num_channels;
    WaveHeader.SampleRate = WavpackGetSampleRate (wpc);
    WaveHeader.BlockAlign = num_channels * bps;
    WaveHeader.BytesPerSecond = WaveHeader.SampleRate * WaveHeader.BlockAlign;
    WaveHeader.BitsPerSample = WavpackGetBitsPerSample (wpc);

    strncpy (DataChunkHeader.ckID, "data", sizeof (DataChunkHeader.ckID));
    DataChunkHeader.ckSize = total_samples * num_channels * bps;

    native_to_little_endian (&RiffChunkHeader, ChunkHeaderFormat);
    native_to_little_endian (&FormatChunkHeader, ChunkHeaderFormat);
    native_to_little_endian (&WaveHeader, WaveHeaderFormat);
    native_to_little_endian (&DataChunkHeader, ChunkHeaderFormat);

    if (!fwrite (&RiffChunkHeader, sizeof (RiffChunkHeader), 1, stdout) ||
        !fwrite (&FormatChunkHeader, sizeof (FormatChunkHeader), 1, stdout) ||
        !fwrite (&WaveHeader, sizeof (WaveHeader), 1, stdout) ||
        !fwrite (&DataChunkHeader, sizeof (DataChunkHeader), 1, stdout)) {
            fputs ("can't write .WAV data, disk probably full!\n", stderr);
            return 1;
        }

    while (1) {
        uint32_t samples_unpacked;

        samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, 256 / num_channels);
        total_unpacked_samples += samples_unpacked;

        if (samples_unpacked) {
            format_samples (bps, (uchar *) temp_buffer, temp_buffer, samples_unpacked *= num_channels);

            if (fwrite (temp_buffer, bps, samples_unpacked, stdout) != samples_unpacked) {
                fputs ("can't write .WAV data, disk probably full!\n", stderr);
                return 1;
            }
        }

        if (!samples_unpacked)
            break;
    }

    fflush (stdout);

    if (WavpackGetNumSamples (wpc) != (uint32_t) -1 &&
        total_unpacked_samples != WavpackGetNumSamples (wpc)) {
            fputs ("incorrect number of samples!\n", stderr);
            return 1;
    }

    if (WavpackGetNumErrors (wpc)) {
        fputs ("crc errors detected!\n", stderr);
        return 1;
    }

    return 0;
}

static int32_t read_bytes (void *buff, int32_t bcount)
{
    return fread (buff, 1, bcount, stdin);
}

// Reformat samples from longs in processor's native endian mode to
// little-endian data with (possibly) less than 4 bytes / sample.

static uchar *format_samples (int bps, uchar *dst, int32_t *src, uint32_t samcnt)
{
    int32_t temp;

    switch (bps) {

        case 1:
            while (samcnt--)
                *dst++ = *src++ + 128;

            break;

        case 2:
            while (samcnt--) {
                *dst++ = (uchar)(temp = *src++);
                *dst++ = (uchar)(temp >> 8);
            }

            break;

        case 3:
            while (samcnt--) {
                *dst++ = (uchar)(temp = *src++);
                *dst++ = (uchar)(temp >> 8);
                *dst++ = (uchar)(temp >> 16);
            }

            break;

        case 4:
            while (samcnt--) {
                *dst++ = (uchar)(temp = *src++);
                *dst++ = (uchar)(temp >> 8);
                *dst++ = (uchar)(temp >> 16);
                *dst++ = (uchar)(temp >> 24);
            }

            break;
    }

    return dst;
}
