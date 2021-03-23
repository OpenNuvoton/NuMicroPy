#ifndef __MINIMP4_DEMUX_H_
#define __MINIMP4_DEMUX_H_

/*
    https://github.com/aspt/mp4
    https://github.com/lieff/minimp4
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "minimp4_comm.h"

// Support parsing of supplementary information, not necessary for decoding:
// duration, language, bitrate, metadata tags, etc
#ifndef MP4D_INFO_SUPPORTED
#   define MP4D_INFO_SUPPORTED       1
#endif

// Enable code, which prints to stdout supplementary MP4 information:
#ifndef MP4D_PRINT_INFO_SUPPORTED
#   define MP4D_PRINT_INFO_SUPPORTED    MP4D_INFO_SUPPORTED
#endif

#ifndef MP4D_AVC_SUPPORTED
#   define MP4D_AVC_SUPPORTED        1
#endif

#ifndef MP4D_TIMESTAMPS_SUPPORTED
#   define MP4D_TIMESTAMPS_SUPPORTED 1
#endif

// Debug trace
#define MP4D_TRACE_SUPPORTED      0
#define MP4D_TRACE_TIMESTAMPS     1


/************************************************************************/
/*          Some values of MP4D_track_t->handler_type              */
/************************************************************************/
// Video track : 'vide'
#define MP4D_HANDLER_TYPE_VIDE                                  0x76696465
// Audio track : 'soun'
#define MP4D_HANDLER_TYPE_SOUN                                  0x736F756E


typedef struct MP4D_sample_to_chunk_t_tag MP4D_sample_to_chunk_t;

typedef struct
{
    /************************************************************************/
    /*                 mandatory public data                                */
    /************************************************************************/
    // How many 'samples' in the track
    // The 'sample' is MP4 term, denoting audio or video frame
    unsigned sample_count;

    // Decoder-specific info (DSI) data
    unsigned char *dsi;

    // DSI data size
    unsigned dsi_bytes;

    // MP4 object type code
    // case 0x00: return "Forbidden";
    // case 0x01: return "Systems ISO/IEC 14496-1";
    // case 0x02: return "Systems ISO/IEC 14496-1";
    // case 0x20: return "Visual ISO/IEC 14496-2";
    // case 0x40: return "Audio ISO/IEC 14496-3";
    // case 0x60: return "Visual ISO/IEC 13818-2 Simple Profile";
    // case 0x61: return "Visual ISO/IEC 13818-2 Main Profile";
    // case 0x62: return "Visual ISO/IEC 13818-2 SNR Profile";
    // case 0x63: return "Visual ISO/IEC 13818-2 Spatial Profile";
    // case 0x64: return "Visual ISO/IEC 13818-2 High Profile";
    // case 0x65: return "Visual ISO/IEC 13818-2 422 Profile";
    // case 0x66: return "Audio ISO/IEC 13818-7 Main Profile";
    // case 0x67: return "Audio ISO/IEC 13818-7 LC Profile";
    // case 0x68: return "Audio ISO/IEC 13818-7 SSR Profile";
    // case 0x69: return "Audio ISO/IEC 13818-3";
    // case 0x6A: return "Visual ISO/IEC 11172-2";
    // case 0x6B: return "Audio ISO/IEC 11172-3";
    // case 0x6C: return "Visual ISO/IEC 10918-1";
    unsigned object_type_indication;

#if MP4D_INFO_SUPPORTED
    /************************************************************************/
    /*                 informational public data                            */
    /************************************************************************/
    // handler_type when present in a media box, is an integer containing one of
    // the following values, or a value from a derived specification:
    // 'vide' Video track
    // 'soun' Audio track
    // 'hint' Hint track
    unsigned handler_type;

    // Track duration: 64-bit value split into 2 variables
    unsigned duration_hi;
    unsigned duration_lo;

    // duration scale: duration = timescale*seconds
    unsigned timescale;

    // Average bitrate, bits per second
    unsigned avg_bitrate_bps;

    // Track language: 3-char ISO 639-2T code: "und", "eng", "rus", "jpn" etc...
    unsigned char language[4];

    // MP4 stream type
    // case 0x00: return "Forbidden";
    // case 0x01: return "ObjectDescriptorStream";
    // case 0x02: return "ClockReferenceStream";
    // case 0x03: return "SceneDescriptionStream";
    // case 0x04: return "VisualStream";
    // case 0x05: return "AudioStream";
    // case 0x06: return "MPEG7Stream";
    // case 0x07: return "IPMPStream";
    // case 0x08: return "ObjectContentInfoStream";
    // case 0x09: return "MPEGJStream";
    unsigned stream_type;

    union
    {
        // for handler_type == 'soun' tracks
        struct
        {
            unsigned channelcount;
            unsigned samplerate_hz;
        } audio;

        // for handler_type == 'vide' tracks
        struct
        {
            unsigned width;
            unsigned height;
        } video;
    } SampleDescription;
#endif

    /************************************************************************/
    /*                 private data: MP4 indexes                            */
    /************************************************************************/
    unsigned *entry_size;

    unsigned sample_to_chunk_count;
    struct MP4D_sample_to_chunk_t_tag *sample_to_chunk;

    unsigned chunk_count;
    MP4D_file_offset_t *chunk_offset;

#if MP4D_TIMESTAMPS_SUPPORTED
    unsigned *timestamp;
    unsigned *duration;
#endif

} MP4D_track_t;

typedef struct MP4D_demux_tag
{
    /************************************************************************/
    /*                 mandatory public data                                */
    /************************************************************************/
    // number of tracks in the movie
    unsigned track_count;

    // track data (public/private)
    MP4D_track_t *track;

#if MP4D_INFO_SUPPORTED
    /************************************************************************/
    /*                 informational public data                            */
    /************************************************************************/
    // Movie duration: 64-bit value split into 2 variables
    unsigned duration_hi;
    unsigned duration_lo;

    // duration scale: duration = timescale*seconds
    unsigned timescale;

    // Metadata tag (optional)
    // Tags provided 'as-is', without any re-encoding
    struct
    {
        unsigned char *title;
        unsigned char *artist;
        unsigned char *album;
        unsigned char *year;
        unsigned char *comment;
        unsigned char *genre;
    } tag;
#endif

} MP4D_demux_t;

struct MP4D_sample_to_chunk_t_tag
{
    unsigned first_chunk;
    unsigned samples_per_chunk;
};

/************************************************************************/
/*          API                                                         */
/************************************************************************/

/**
*   Parse given file as MP4 file.  Allocate and store data indexes.
*   return 1 on success, 0 on failure
*   Given file rewind()'ed on return.
*   The MP4 indexes may be stored at the end of file, so this
*   function may parse all file, using fseek().
*   It is guaranteed that function will read/seek the file sequentially,
*   and will never jump back.
*/
int MP4D__open(MP4D_demux_t *mp4, int fh);

/**
*   Return position and size for given sample from given track. The 'sample' is a
*   MP4 term for 'frame'
*
*   frame_bytes [OUT]   - return coded frame size in bytes
*   timestamp [OUT]     - return frame timestamp (in mp4->timescale units)
*   duration [OUT]      - return frame duration (in mp4->timescale units)
*
*   function return file offset for the frame
*/
MP4D_file_offset_t MP4D__frame_offset(const MP4D_demux_t *mp4, unsigned int ntrack, unsigned int nsample, unsigned int *frame_bytes, unsigned *timestamp, unsigned *duration);

/**
*   De-allocated memory
*/
void MP4D__close(MP4D_demux_t *mp4);

/**
*   Helper functions to parse mp4.track[ntrack].dsi for H.264 SPS/PPS
*   Return pointer to internal mp4 memory, it must not be free()-ed
*
*   Example: process all SPS in MP4 file:
*       while (sps = MP4D__read_sps(mp4, num_of_avc_track, sps_count, &sps_bytes))
*       {
*           process(sps, sps_bytes);
*           sps_count++;
*       }
*/
const void *MP4D__read_sps(const MP4D_demux_t *mp4, unsigned int ntrack, int nsps, int *sps_bytes);
const void *MP4D__read_pps(const MP4D_demux_t *mp4, unsigned int ntrack, int npps, int *pps_bytes);

#if MP4D_PRINT_INFO_SUPPORTED
/**
*   Print MP4 information to stdout.
*   Uses printf() as well as floating-point functions
*   Given as implementation example and for test purposes
*/
void MP4D__printf_info(const MP4D_demux_t *mp4);
#endif  // #if MP4D_PRINT_INFO_SUPPORTED


#if MP4D_TRACE_SUPPORTED
#   define MP4D_TRACE(x) printf x
#else
#   define MP4D_TRACE(x)
#endif


#define NELEM(x)  (sizeof(x) / sizeof((x)[0]))

static off_t minimp4_fsize(int fh)
{
	struct stat sFileStat;
	
	if(fstat(fh, &sFileStat)!=0)
		return -1;

	return sFileStat.st_size;
}

static char minimp4_getc(int fh, int *eof_flag)
{
	char chBuf;
	int iRet;
	iRet = read(fh, &chBuf, 1);
	
	if(iRet< 0)
		*eof_flag = 1;
	return chBuf;
}


/**
*   Read given number of bytes from the file
*   Used to read box headers
*/
static unsigned minimp4_read(int fh, int nb, int *eof_flag)
{
    uint32_t v = 0;
    switch (nb)
    {
    case 4: v = (v << 8) | minimp4_getc(fh, eof_flag);
    case 3: v = (v << 8) | minimp4_getc(fh, eof_flag);
    case 2: v = (v << 8) | minimp4_getc(fh, eof_flag);
    default:
    case 1: v = (v << 8) | minimp4_getc(fh, eof_flag);
    }

    return v;
}

/**
*   Read given number of bytes, but no more than *payload_bytes specifies...
*   Used to read box payload
*/
static uint32_t read_payload(int fh, unsigned nb, boxsize_t *payload_bytes, int *eof_flag)
{
    if (*payload_bytes < nb)
    {
        *eof_flag = 1;
        nb = (int)*payload_bytes;
    }
    *payload_bytes -= nb;

    return minimp4_read(fh, nb, eof_flag);
}


/**
*   Skips given number of bytes.
*   Avoid math operations with fpos_t
*/
static void my_fseek(int fh, boxsize_t pos, int *eof_flag)
{
    while (pos > 0)
    {
        long lpos = (long)MINIMP4_MIN(pos, (boxsize_t)LONG_MAX);
        if (lseek(fh, lpos, SEEK_CUR) < 0)
        {
            *eof_flag = 1;
            return;
        }
        pos -= lpos;
    }
}

#define MP4D_READ(n) read_payload(fh, n, &payload_bytes, &eof_flag)
#define SKIP(n) { boxsize_t t = MINIMP4_MIN(payload_bytes, n); my_fseek(fh, t, &eof_flag); payload_bytes -= t; }
#define MALLOC(t, p, size) {p = (t)malloc(size); if (!(p)) { NMLOG_ERROR("out of memory"); }}

/*
*   On error: release resources, rewind the file.
*/
#define RETURN_ERROR(mess) {        \
    MP4D_TRACE(("\nMP4 ERROR: " mess));  \
    lseek(fh, 0, SEEK_SET);          \
    MP4D__close(mp4);               \
    return 0;                       \
}

/*
*   Any errors, occurred on top-level hierarchy is passed to exit check: 'if (!mp4->track_count) ... '
*/
#define MP4D_ERROR(mess)                 \
    if (!depth)                     \
        break;                      \
    else                            \
        RETURN_ERROR(mess);


typedef enum {BOX_ATOM, BOX_OD} boxtype_t;

// Exported API function
int MP4D__open(MP4D_demux_t *mp4, int fh)
{
    // box stack size
    int depth = 0;

    off_t file_size = minimp4_fsize(fh);

    struct
    {
        // remaining bytes for box in the stack
        boxsize_t bytes;

        // kind of box children's: OD chunks handled in the same manner as name chunks
        boxtype_t format;

    } stack[MAX_CHUNKS_DEPTH];

#if MP4D_TRACE_SUPPORTED
    // path of current element: List0/List1/... etc
    uint32_t box_path[MAX_CHUNKS_DEPTH];
#endif

    int eof_flag = 0;
    unsigned i;
    MP4D_track_t *tr = NULL;

    if (!fh || !mp4)
    {
        MP4D_TRACE(("\nERROR: invlaid arguments!"));
        return 0;
    }

    if (lseek(fh, 0, SEEK_SET))
    {
        return 0;
    }

    memset(mp4, 0, sizeof(MP4D_demux_t));
    stack[0].format = BOX_ATOM;   // start with atom box
    stack[0].bytes = 0;           // never accessed
		
    do
    {
        // List of boxes, derived from 'FullBox'
        //                ~~~~~~~~~~~~~~~~~~~~~
        // need read version field and check version for these boxes
        static const struct
        {
            uint32_t name;
            unsigned max_version;
            unsigned use_track_flag;
        } g_fullbox[] =
        {
#if MP4D_INFO_SUPPORTED
            {BOX_mdhd, 1, 1},
            {BOX_mvhd, 1, 0},
            {BOX_hdlr, 0, 0},
            {BOX_meta, 0, 0},   // Android can produce meta box without 'FullBox' field, comment this line to simulate the bug
#endif
#if MP4D_TRACE_TIMESTAMPS
            {BOX_stts, 0, 0},
            {BOX_ctts, 0, 0},
#endif
            {BOX_stz2, 0, 1},
            {BOX_stsz, 0, 1},
            {BOX_stsc, 0, 1},
            {BOX_stco, 0, 1},
            {BOX_co64, 0, 1},
            {BOX_stsd, 0, 0},
            {BOX_esds, 0, 1}    // esds does not use track, but switches to OD mode. Check here, to avoid OD check
        };

        // List of boxes, which contains other boxes ('envelopes')
        // Parser will descend down for boxes in this list, otherwise parsing will proceed to
        // the next sibling box
        // OD boxes handled in the same way as atom boxes...
        static const struct
        {
            uint32_t name;
            boxtype_t type;
        } g_envelope_box[] =
        {
            {BOX_esds, BOX_OD},     // TODO: BOX_esds can be used for both audio and video, but this code supports audio only!
            {OD_ESD,   BOX_OD},
            {OD_DCD,   BOX_OD},
            {OD_DSI,   BOX_OD},
            {BOX_trak, BOX_ATOM},
            {BOX_moov, BOX_ATOM},
            {BOX_mdia, BOX_ATOM},
            {BOX_tref, BOX_ATOM},
            {BOX_minf, BOX_ATOM},
            {BOX_dinf, BOX_ATOM},
            {BOX_stbl, BOX_ATOM},
            {BOX_stsd, BOX_ATOM},
            {BOX_mp4a, BOX_ATOM},
            {BOX_mp4s, BOX_ATOM},
#if MP4D_AVC_SUPPORTED
            {BOX_mp4v, BOX_ATOM},
            {BOX_avc1, BOX_ATOM},
//             {BOX_avc2, BOX_ATOM},
//             {BOX_svc1, BOX_ATOM},
#endif
            {BOX_udta, BOX_ATOM},
            {BOX_meta, BOX_ATOM},
            {BOX_ilst, BOX_ATOM}
        };

        uint32_t FullAtomVersionAndFlags = 0;
        boxsize_t payload_bytes;
        boxsize_t box_bytes;
        uint32_t box_name;
#if MP4D_INFO_SUPPORTED
        unsigned char **ptag = NULL;
#endif
        int read_bytes = 0;

        // Read header box type and it's length
        if (stack[depth].format == BOX_ATOM)
        {
            box_bytes = minimp4_read(fh, 4, &eof_flag);
#if FIX_BAD_ANDROID_META_BOX
broken_android_meta_hack:
#endif
            if (eof_flag)
            {
                break;  // normal exit
            }

            if (box_bytes >= 2 && box_bytes < 8)
            {
                MP4D_ERROR("invalid box size (broken file?)");
            }

            box_name  = minimp4_read(fh, 4, &eof_flag);
            read_bytes = 8;

            // Decode box size
            if (box_bytes == 0 ||                         // standard indication of 'till eof' size
                box_bytes == (boxsize_t)0xFFFFFFFFU       // some files uses non-standard 'till eof' signaling
                )
            {
                box_bytes = ~(boxsize_t)0;
            }

            payload_bytes = box_bytes - 8;

            if (box_bytes == 1)           // 64-bit sizes
            {
                MP4D_TRACE(("\n64-bit chunk encountered"));

                box_bytes = minimp4_read(fh, 4, &eof_flag);
#if MP4D_64BIT_SUPPORTED
                box_bytes <<= 32;
                box_bytes |= minimp4_read(f, 4, &eof_flag);
#else
                if (box_bytes)
                {
                    MP4D_ERROR("UNSUPPORTED FEATURE: MP4BoxHeader(): 64-bit boxes not supported!");
                }
                box_bytes = minimp4_read(fh, 4, &eof_flag);
#endif
                if (box_bytes < 16)
                {
                    MP4D_ERROR("invalid box size (broken file?)");
                }
                payload_bytes = box_bytes - 16;
            }

            // Read and check box version for some boxes
            for (i = 0; i < NELEM(g_fullbox); i++)
            {
                if (box_name == g_fullbox[i].name)
                {
                    FullAtomVersionAndFlags = MP4D_READ(4);
                    read_bytes += 4;

#if FIX_BAD_ANDROID_META_BOX
                    // Fix invalid BOX_meta, found in some Android-produced MP4
                    // This branch is optional: bad box would be skipped
                    if (box_name == BOX_meta)
                    {
                        if (FullAtomVersionAndFlags >= 8 &&  FullAtomVersionAndFlags < payload_bytes)
                        {
                            if (box_bytes > stack[depth].bytes)
                            {
                                MP4D_ERROR("broken file structure!");
                            }
                            stack[depth].bytes -= box_bytes;;
                            depth++;
                            stack[depth].bytes = payload_bytes + 4; // +4 need for missing header
                            stack[depth].format = BOX_ATOM;
                            box_bytes = FullAtomVersionAndFlags;
                            MP4D_TRACE(("Bad metadata box detected (Android bug?)!\n"));
                            goto broken_android_meta_hack;
                        }
                    }
#endif // FIX_BAD_ANDROID_META_BOX

                    if ((FullAtomVersionAndFlags >> 24) > g_fullbox[i].max_version)
                    {
                        MP4D_ERROR("unsupported box version!");
                    }
                    if (g_fullbox[i].use_track_flag && !tr)
                    {
                        MP4D_ERROR("broken file structure!");
                    }
                }
            }
        } else // stack[depth].format == BOX_OD
        {
            int val;
            box_name = OD_BASE + minimp4_read(fh, 1, &eof_flag);     // 1-byte box type
            read_bytes += 1;
            if (eof_flag)
            {
                break;
            }

            payload_bytes = 0;
            box_bytes = 1;
            do
            {
                val = minimp4_read(fh, 1, &eof_flag);
                read_bytes += 1;
                if (eof_flag)
                {
                    MP4D_ERROR("premature EOF!");
                }
                payload_bytes = (payload_bytes << 7) | (val & 0x7F);
                box_bytes++;
            } while (val & 0x80);
            box_bytes += payload_bytes;
        }

#if MP4D_TRACE_SUPPORTED
        box_path[depth] = (box_name >> 24) | (box_name << 24) | ((box_name >> 8) & 0x0000FF00) | ((box_name << 8) & 0x00FF0000);
        MP4D_TRACE(("%2d  %8d %.*s  (%d bytes remains for sibilings) \n", depth, (int)box_bytes, depth*4, (char*)box_path, (int)stack[depth].bytes));
#endif

        // Check that box size <= parent size
        if (depth)
        {
            // Skip box with bad size
            assert(box_bytes > 0);
            if (box_bytes > stack[depth].bytes)
            {
                MP4D_TRACE(("Wrong %c%c%c%c box size: broken file?\n", (box_name >> 24)&255, (box_name >> 16)&255, (box_name >> 8)&255, box_name&255));
                box_bytes = stack[depth].bytes;
                box_name = 0;
                payload_bytes = box_bytes - read_bytes;
            }
            stack[depth].bytes -= box_bytes;
        }

        // Read box header
        switch(box_name)
        {
        case BOX_stz2:  //ISO/IEC 14496-1 Page 38. Section 8.17.2 - Sample Size Box.
        case BOX_stsz:
            {
                int size = 0;
                uint32_t sample_size = MP4D_READ(4);
                tr->sample_count = MP4D_READ(4);
                MALLOC(unsigned int*, tr->entry_size, tr->sample_count*4);
                for (i = 0; i < tr->sample_count; i++)
                {
                    if (box_name == BOX_stsz)
                    {
                       tr->entry_size[i] = (sample_size?sample_size:MP4D_READ(4));
                    }
                    else
                    {
                        switch (sample_size & 0xFF)
                        {
                        case 16:
                            tr->entry_size[i] = MP4D_READ(2);
                            break;
                        case  8:
                            tr->entry_size[i] = MP4D_READ(1);
                            break;
                        case  4:
                            if (i & 1)
                            {
                                tr->entry_size[i] = size & 15;
                            } else
                            {
                                size = MP4D_READ(1);
                                tr->entry_size[i] = (size >> 4);
                            }
                            break;
                        }
                    }
                }
            }
            break;

        case BOX_stsc:  //ISO/IEC 14496-12 Page 38. Section 8.18 - Sample To Chunk Box.
            tr->sample_to_chunk_count = MP4D_READ(4);
            MALLOC(MP4D_sample_to_chunk_t*, tr->sample_to_chunk, tr->sample_to_chunk_count*sizeof(tr->sample_to_chunk[0]));
            for (i = 0; i < tr->sample_to_chunk_count; i++)
            {
                tr->sample_to_chunk[i].first_chunk = MP4D_READ(4);
                tr->sample_to_chunk[i].samples_per_chunk = MP4D_READ(4);
                SKIP(4);    // sample_description_index
            }
            break;
#if MP4D_TRACE_TIMESTAMPS || MP4D_TIMESTAMPS_SUPPORTED
        case BOX_stts:
            {
                unsigned count = MP4D_READ(4);
                unsigned j, k = 0, ts = 0, ts_count = count;
#if MP4D_TIMESTAMPS_SUPPORTED
                MALLOC(unsigned int*, tr->timestamp, ts_count*4);
                MALLOC(unsigned int*, tr->duration, ts_count*4);
#endif

                for (i = 0; i < count; i++)
                {
                    unsigned sc = MP4D_READ(4);
                    int d =  MP4D_READ(4);
                    MP4D_TRACE(("sample %8d count %8d duration %8d\n", i, sc, d));
#if MP4D_TIMESTAMPS_SUPPORTED
                    if (k + sc > ts_count)
                    {
                        ts_count = k + sc;
                        tr->timestamp = (unsigned int*)realloc(tr->timestamp, ts_count * sizeof(unsigned));
                        tr->duration  = (unsigned int*)realloc(tr->duration,  ts_count * sizeof(unsigned));
                    }
                    for (j = 0; j < sc; j++)
                    {
                        tr->duration[k] = d;
                        tr->timestamp[k++] = ts;
                        ts += d;
                    }
#endif

                }
            }
            break;
        case BOX_ctts:
            {
                unsigned count = MP4D_READ(4);
                for (i = 0; i < count; i++)
                {
                    int sc = MP4D_READ(4);
                    int d =  MP4D_READ(4);
                    (void)sc;
                    (void)d;
                    MP4D_TRACE(("sample %8d count %8d decoding to composition offset %8d\n", i, sc, d));
                }
            }
            break;
#endif
        case BOX_stco:  //ISO/IEC 14496-12 Page 39. Section 8.19 - Chunk Offset Box.
        case BOX_co64:
            tr->chunk_count = MP4D_READ(4);
            MALLOC(MP4D_file_offset_t*, tr->chunk_offset, tr->chunk_count*sizeof(MP4D_file_offset_t));
            for (i = 0; i < tr->chunk_count; i++)
            {
                tr->chunk_offset[i] = MP4D_READ(4);
                if (box_name == BOX_co64)
                {
#if !MP4D_64BIT_SUPPORTED
                    if (tr->chunk_offset[i])
                    {
                        MP4D_ERROR("UNSUPPORTED FEATURE: 64-bit chunk_offset not supported!");
                    }
#endif
                    tr->chunk_offset[i] <<= 32;
                    tr->chunk_offset[i] |= MP4D_READ(4);
                }
            }
            break;

#if MP4D_INFO_SUPPORTED
        case BOX_mvhd:
            SKIP(((FullAtomVersionAndFlags >> 24) == 1) ? 8 + 8 : 4 + 4);
            mp4->timescale = MP4D_READ(4);
            mp4->duration_hi = ((FullAtomVersionAndFlags >> 24) == 1) ? MP4D_READ(4) : 0;
            mp4->duration_lo = MP4D_READ(4);
            SKIP(4 + 2 + 2 + 4*2 + 4*9 + 4*6 + 4);
            break;

        case BOX_mdhd:
            SKIP(((FullAtomVersionAndFlags >> 24) == 1) ? 8 + 8 : 4 + 4);
            tr->timescale = MP4D_READ(4);
            tr->duration_hi = ((FullAtomVersionAndFlags >> 24) == 1) ? MP4D_READ(4) : 0;
            tr->duration_lo = MP4D_READ(4);

            {
                int ISO_639_2_T = MP4D_READ(2);
                tr->language[2] = (ISO_639_2_T & 31) + 0x60; ISO_639_2_T >>= 5;
                tr->language[1] = (ISO_639_2_T & 31) + 0x60; ISO_639_2_T >>= 5;
                tr->language[0] = (ISO_639_2_T & 31) + 0x60;
            }
            // the rest of this box is skipped by default ...
            break;

        case BOX_hdlr:
            if (tr) // When this box is within 'meta' box, the track may not be avaialable
            {
                SKIP(4); // pre_defined
                tr->handler_type = MP4D_READ(4);
            }
            // typically hdlr box does not contain any useful info.
            // the rest of this box is skipped by default ...
            break;

        case BOX_btrt:
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }

            SKIP(4 + 4);
            tr->avg_bitrate_bps = MP4D_READ(4);
            break;

            // Set pointer to tag to be read...
        case BOX_calb: ptag = &mp4->tag.album;   break;
        case BOX_cART: ptag = &mp4->tag.artist;  break;
        case BOX_cnam: ptag = &mp4->tag.title;   break;
        case BOX_cday: ptag = &mp4->tag.year;    break;
        case BOX_ccmt: ptag = &mp4->tag.comment; break;
        case BOX_cgen: ptag = &mp4->tag.genre;   break;

#endif

        case BOX_stsd:
            SKIP(4); // entry_count, BOX_mp4a & BOX_mp4v boxes follows immediately
            break;

        case BOX_mp4s:  // private stream
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }
            SKIP(6*1 + 2/*Base SampleEntry*/);
            break;

        case BOX_mp4a:
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }
#if MP4D_INFO_SUPPORTED
            SKIP(6*1+2/*Base SampleEntry*/  + 4*2);
            tr->SampleDescription.audio.channelcount = MP4D_READ(2);
            SKIP(2/*samplesize*/ + 2 + 2);
            tr->SampleDescription.audio.samplerate_hz = MP4D_READ(4) >> 16;
#else
            SKIP(28);
#endif
            break;

#if MP4D_AVC_SUPPORTED
        case BOX_avc1:  // AVCSampleEntry extends VisualSampleEntry
//         case BOX_avc2:   - no test
//         case BOX_svc1:   - no test
        case BOX_mp4v:
            if (!tr)
            {
                MP4D_ERROR("broken file structure!");
            }
#if MP4D_INFO_SUPPORTED
            SKIP(6*1 + 2/*Base SampleEntry*/ + 2 + 2 + 4*3);
            tr->SampleDescription.video.width  = MP4D_READ(2);
            tr->SampleDescription.video.height = MP4D_READ(2);
            // frame_count is always 1
            // compressorname is rarely set..
            SKIP(4 + 4 + 4 + 2/*frame_count*/ + 32/*compressorname*/ + 2 + 2);
#else
            SKIP(78);
#endif
            // ^^^ end of VisualSampleEntry
            // now follows for BOX_avc1:
            //      BOX_avcC
            //      BOX_btrt (optional)
            //      BOX_m4ds (optional)
            // for BOX_mp4v:
            //      BOX_esds
            break;

        case BOX_avcC:  // AVCDecoderConfigurationRecord()
            // hack: AAC-specific DSI field reused (for it have same purpoose as sps/pps)
            // TODO: check this hack if BOX_esds co-exist with BOX_avcC
            tr->object_type_indication = MP4_OBJECT_TYPE_AVC;
            tr->dsi = (unsigned char*)malloc((size_t)box_bytes);
            tr->dsi_bytes = (unsigned)box_bytes;
            {
                int spspps;
                unsigned char *p = tr->dsi;
                unsigned int configurationVersion = MP4D_READ(1);
                unsigned int AVCProfileIndication = MP4D_READ(1);
                unsigned int profile_compatibility = MP4D_READ(1);
                unsigned int AVCLevelIndication = MP4D_READ(1);
                //bit(6) reserved =
                unsigned int lengthSizeMinusOne = MP4D_READ(1) & 3;

                (void)configurationVersion;
                (void)AVCProfileIndication;
                (void)profile_compatibility;
                (void)AVCLevelIndication;
                (void)lengthSizeMinusOne;

                for (spspps = 0; spspps < 2; spspps++)
                {
                    unsigned int numOfSequenceParameterSets= MP4D_READ(1);
                    if (!spspps)
                    {
                         numOfSequenceParameterSets &= 31;  // clears 3 msb for SPS
                    }
                    *p++ = numOfSequenceParameterSets;
                    for (i = 0; i < numOfSequenceParameterSets; i++)
                    {
                        unsigned k, sequenceParameterSetLength = MP4D_READ(2);
                        *p++ = sequenceParameterSetLength >> 8;
                        *p++ = sequenceParameterSetLength ;
                        for (k = 0; k < sequenceParameterSetLength; k++)
                        {
                            *p++ = MP4D_READ(1);
                        }
                    }
                }
            }
            break;
#endif  // MP4D_AVC_SUPPORTED

        case OD_ESD:
            {
                unsigned flags = MP4D_READ(3);   // ES_ID(2) + flags(1)

                if (flags & 0x80)       // steamdependflag
                {
                    SKIP(2);            // dependsOnESID
                }
                if (flags & 0x40)       // urlflag
                {
                    unsigned bytecount = MP4D_READ(1);
                    SKIP(bytecount);    // skip URL
                }
                if (flags & 0x20)       // ocrflag (was reserved in MPEG-4 v.1)
                {
                    SKIP(2);            // OCRESID
                }
                break;
            }

        case OD_DCD:        //ISO/IEC 14496-1 Page 28. Section 8.6.5 - DecoderConfigDescriptor.
            assert(tr);     // ensured by g_fullbox[] check
            tr->object_type_indication = MP4D_READ(1);
#if MP4D_INFO_SUPPORTED
            tr->stream_type = MP4D_READ(1) >> 2;
            SKIP(3/*bufferSizeDB*/ + 4/*maxBitrate*/);
            tr->avg_bitrate_bps = MP4D_READ(4);
#else
            SKIP(1+3+4+4);
#endif
            break;

        case OD_DSI:        //ISO/IEC 14496-1 Page 28. Section 8.6.5 - DecoderConfigDescriptor.
            assert(tr);     // ensured by g_fullbox[] check
            if (!tr->dsi && payload_bytes)
            {
                MALLOC(unsigned char*, tr->dsi, (int)payload_bytes);
                for (i = 0; i < payload_bytes; i++)
                {
                    tr->dsi[i] = minimp4_read(fh, 1, &eof_flag);    // These bytes available due to check above
                }
                tr->dsi_bytes = i;
                payload_bytes -= i;
                break;
            }

        default:
            MP4D_TRACE(("[%c%c%c%c]  %d\n", box_name >> 24, box_name >> 16, box_name >> 8, box_name, (int)payload_bytes));
        }

#if MP4D_INFO_SUPPORTED
        // Read tag is tag pointer is set
        if (ptag && !*ptag && payload_bytes > 16)
        {
#if 0
            uint32_t size = MP4D_READ(4);
            uint32_t data = MP4D_READ(4);
            uint32_t class = MP4D_READ(4);
            uint32_t x1 = MP4D_READ(4);
            MP4D_TRACE(("%2d  %2d %2d ", size, class, x1));
#else
            SKIP(4 + 4 + 4 + 4);
#endif
            MALLOC(unsigned char*, *ptag, (unsigned)payload_bytes + 1);
            for (i = 0; payload_bytes != 0; i++)
            {
                (*ptag)[i] = MP4D_READ(1);
            }
            (*ptag)[i] = 0; // zero-terminated string
        }
#endif

        if (box_name == BOX_trak)
        {
            // New track found: allocate memory using realloc()
            // Typically there are 1 audio track for AAC audio file,
            // 4 tracks for movie file,
            // 3-5 tracks for scalable audio (CELP+AAC)
            // and up to 50 tracks for BSAC scalable audio
            void *mem = realloc(mp4->track, (mp4->track_count + 1)*sizeof(MP4D_track_t));
            if (!mem)
            {
                // if realloc fails, it does not deallocate old pointer!
                MP4D_ERROR("out of memory");
            }
            mp4->track = (MP4D_track_t*)mem;
            tr = mp4->track + mp4->track_count++;
            memset(tr, 0, sizeof(MP4D_track_t));
        } else if (box_name == BOX_meta)
        {
            tr = NULL;  // Avoid update of 'hdlr' box, which may contains in the 'meta' box
        }

        // If this box is envelope, save it's size in box stack
        for (i = 0; i < NELEM(g_envelope_box); i++)
        {
            if (box_name == g_envelope_box[i].name)
            {
                if (++depth >= MAX_CHUNKS_DEPTH)
                {
                    MP4D_ERROR("too deep atoms nesting!");
                }
                stack[depth].bytes = payload_bytes;
                stack[depth].format = g_envelope_box[i].type;
                break;
            }
        }

        // if box is not envelope, just skip it
        if (i == NELEM(g_envelope_box))
        {
            if (payload_bytes > file_size)
            {
                eof_flag = 1;
            } else
            {
                SKIP(payload_bytes);
            }
        }

        // remove empty boxes from stack
        // don't touch box with index 0 (which indicates whole file)
        while (depth > 0 && !stack[depth].bytes)
        {
            depth--;
        }

    } while(!eof_flag);

    if (!mp4->track_count)
    {
        RETURN_ERROR("no tracks found");
    }
    lseek(fh, 0, SEEK_SET);
    return 1;
}

/**
*   Find chunk, containing given sample.
*   Returns chunk number, and first sample in this chunk.
*/
static int sample_to_chunk(MP4D_track_t *tr, unsigned nsample, unsigned *nfirst_sample_in_chunk)
{
    unsigned chunk_group = 0, nc;
    unsigned sum = 0;
    *nfirst_sample_in_chunk = 0;
    if (tr->chunk_count <= 1)
    {
        return 0;
    }
    for (nc = 0; nc < tr->chunk_count; nc++)
    {
        if (chunk_group + 1 < tr->sample_to_chunk_count     // stuck at last entry till EOF
            && nc + 1 ==    // Chunks counted starting with '1'
            tr->sample_to_chunk[chunk_group + 1].first_chunk)    // next group?
        {
            chunk_group++;
        }

        sum += tr->sample_to_chunk[chunk_group].samples_per_chunk;
        if (nsample < sum)
            return nc;

        // TODO: this can be calculated once per file
        *nfirst_sample_in_chunk = sum;
    }
    return -1;
}

// Exported API function
MP4D_file_offset_t MP4D__frame_offset(const MP4D_demux_t *mp4, unsigned ntrack, unsigned nsample, unsigned *frame_bytes, unsigned *timestamp, unsigned *duration)
{
    MP4D_track_t *tr = mp4->track + ntrack;
    unsigned ns;
    int nchunk = sample_to_chunk(tr, nsample, &ns);
    MP4D_file_offset_t offset;

    if (nchunk < 0)
    {
        *frame_bytes = 0;
        return 0;
    }

    offset = tr->chunk_offset[nchunk];
    for (; ns < nsample; ns++)
    {
        offset += tr->entry_size[ns];
    }

    *frame_bytes = tr->entry_size[ns];

    if (timestamp)
    {
#if MP4D_TIMESTAMPS_SUPPORTED
        *timestamp = tr->timestamp[ns];
#else
        *timestamp = 0;
#endif
    }
    if (duration)
    {
#if MP4D_TIMESTAMPS_SUPPORTED
        *duration = tr->duration[ns];
#else
        *duration = 0;
#endif
    }

    return offset;
}

#define FREE(x) if (x) {free(x); x = NULL;}

// Exported API function
void MP4D__close(MP4D_demux_t *mp4)
{
    while (mp4->track_count)
    {
        MP4D_track_t *tr = mp4->track + --mp4->track_count;
        FREE(tr->entry_size);
#if MP4D_TIMESTAMPS_SUPPORTED
        FREE(tr->timestamp);
        FREE(tr->duration);
#endif
        FREE(tr->sample_to_chunk);
        FREE(tr->chunk_offset);
        FREE(tr->dsi);
    }
    FREE(mp4->track);
#if MP4D_INFO_SUPPORTED
    FREE(mp4->tag.title);
    FREE(mp4->tag.artist);
    FREE(mp4->tag.album);
    FREE(mp4->tag.year);
    FREE(mp4->tag.comment);
    FREE(mp4->tag.genre);
#endif
}

static int skip_spspps(const unsigned char *p, int nbytes, int nskip)
{
    int i, k = 0;
    for (i = 0; i < nskip; i++)
    {
        unsigned segmbytes;
        if (k > nbytes - 2)
        {
            return -1;
        }
        segmbytes = p[k]*256 + p[k+1];
        k += 2 + segmbytes;
    }
    return k;
}

static const void *MP4D__read_spspps(const MP4D_demux_t *mp4, unsigned int ntrack, int pps_flag, int nsps, int *sps_bytes)
{
    int sps_count, skip_bytes;
    int bytepos = 0;
    unsigned char *p = mp4->track[ntrack].dsi;
    if (ntrack >= mp4->track_count)
    {
        return NULL;
    }
    if (mp4->track[ntrack].object_type_indication != MP4_OBJECT_TYPE_AVC)
    {
        return NULL;    // SPS/PPS are specific for AVC format only
    }

    if (pps_flag)
    {
        // Skip all SPS
        sps_count = p[bytepos++];
        skip_bytes = skip_spspps(p+bytepos, mp4->track[ntrack].dsi_bytes - bytepos, sps_count);
        if (skip_bytes < 0)
        {
            return NULL;
        }
        bytepos += skip_bytes;
    }

    // Skip sps/pps before the given target
    sps_count = p[bytepos++];
    if (nsps >= sps_count)
    {
        return NULL;
    }
    skip_bytes = skip_spspps(p+bytepos, mp4->track[ntrack].dsi_bytes - bytepos, nsps);
    if (skip_bytes < 0)
    {
        return NULL;
    }
    bytepos += skip_bytes;
    *sps_bytes = p[bytepos]*256 + p[bytepos+1];
    return p + bytepos + 2;
}


const void *MP4D__read_sps(const MP4D_demux_t *mp4, unsigned int ntrack, int nsps, int *sps_bytes)
{
    return MP4D__read_spspps(mp4, ntrack, 0, nsps, sps_bytes);
}

const void *MP4D__read_pps(const MP4D_demux_t *mp4, unsigned int ntrack, int npps, int *pps_bytes)
{
    return MP4D__read_spspps(mp4, ntrack, 1, npps, pps_bytes);
}

#if MP4D_PRINT_INFO_SUPPORTED
/************************************************************************/
/*  Purely informational part, may be removed for embedded applications */
/************************************************************************/

//
// Decodes ISO/IEC 14496 MP4 stream type to ASCII string
//
static const char *GetMP4StreamTypeName(int streamType)
{
    switch (streamType)
    {
    case 0x00: return "Forbidden";
    case 0x01: return "ObjectDescriptorStream";
    case 0x02: return "ClockReferenceStream";
    case 0x03: return "SceneDescriptionStream";
    case 0x04: return "VisualStream";
    case 0x05: return "AudioStream";
    case 0x06: return "MPEG7Stream";
    case 0x07: return "IPMPStream";
    case 0x08: return "ObjectContentInfoStream";
    case 0x09: return "MPEGJStream";
    default:
        if (streamType >= 0x20 && streamType <= 0x3F)
        {
            return "User private";
        } else
        {
            return "Reserved for ISO use";
        }
    }
}

//
// Decodes ISO/IEC 14496 MP4 object type to ASCII string
//
static const char *GetMP4ObjectTypeName(int objectTypeIndication)
{
    switch (objectTypeIndication)
    {
    case 0x00: return "Forbidden";
    case 0x01: return "Systems ISO/IEC 14496-1";
    case 0x02: return "Systems ISO/IEC 14496-1";
    case 0x20: return "Visual ISO/IEC 14496-2";
    case 0x40: return "Audio ISO/IEC 14496-3";
    case 0x60: return "Visual ISO/IEC 13818-2 Simple Profile";
    case 0x61: return "Visual ISO/IEC 13818-2 Main Profile";
    case 0x62: return "Visual ISO/IEC 13818-2 SNR Profile";
    case 0x63: return "Visual ISO/IEC 13818-2 Spatial Profile";
    case 0x64: return "Visual ISO/IEC 13818-2 High Profile";
    case 0x65: return "Visual ISO/IEC 13818-2 422 Profile";
    case 0x66: return "Audio ISO/IEC 13818-7 Main Profile";
    case 0x67: return "Audio ISO/IEC 13818-7 LC Profile";
    case 0x68: return "Audio ISO/IEC 13818-7 SSR Profile";
    case 0x69: return "Audio ISO/IEC 13818-3";
    case 0x6A: return "Visual ISO/IEC 11172-2";
    case 0x6B: return "Audio ISO/IEC 11172-3";
    case 0x6C: return "Visual ISO/IEC 10918-1";
    case 0xFF: return "no object type specified";
    default:
        if (objectTypeIndication >= 0xC0 && objectTypeIndication <= 0xFE)
        {
            return "User private";
        } else
        {
            return "Reserved for ISO use";
        }
    }
}

/**
*   Print MP4 information to stdout.
*   Subject for customization to particular application
Output Example #1: movie file
MP4 FILE: 7 tracks found. Movie time 104.12 sec
No|type|lng| duration           | bitrate| Stream type            | Object type
 0|odsm|fre|   0.00 s      1 frm|       0| Forbidden              | Forbidden
 1|sdsm|fre|   0.00 s      1 frm|       0| Forbidden              | Forbidden
 2|vide|```| 104.12 s   2603 frm| 1960559| VisualStream           | Visual ISO/IEC 14496-2   -  720x304
 3|soun|ger| 104.06 s   2439 frm|  191242| AudioStream            | Audio ISO/IEC 14496-3    -  6 ch 24000 hz
 4|soun|eng| 104.06 s   2439 frm|  194171| AudioStream            | Audio ISO/IEC 14496-3    -  6 ch 24000 hz
 5|subp|ger|  71.08 s     25 frm|       0| Forbidden              | Forbidden
 6|subp|eng|  71.08 s     25 frm|       0| Forbidden              | Forbidden
Output Example #2: audio file with tags
MP4 FILE: 1 tracks found. Movie time 92.42 sec
title = 86-Second Blowout
artist = Yo La Tengo
album = May I Sing With Me
year = 1992
No|type|lng| duration           | bitrate| Stream type            | Object type
 0|mdir|und|  92.42 s   3980 frm|  128000| AudioStream            | Audio ISO/IEC 14496-3MP4 FILE: 1 tracks found. Movie time 92.42 sec
*/
void MP4D__printf_info(const MP4D_demux_t *mp4)
{
    unsigned i;
    printf("\nMP4 FILE: %d tracks found. Movie time %.2f sec\n", mp4->track_count, (4294967296.0*mp4->duration_hi + mp4->duration_lo) / mp4->timescale);
#define STR_TAG(name) if (mp4->tag.name)  printf("%10s = %s\n", #name, mp4->tag.name)
    STR_TAG(title);
    STR_TAG(artist);
    STR_TAG(album);
    STR_TAG(year);
    STR_TAG(comment);
    STR_TAG(genre);
    printf("\nNo|type|lng| duration           | bitrate| %-23s| Object type", "Stream type");
    for (i = 0; i < mp4->track_count; i++)
    {
        MP4D_track_t *tr = mp4->track + i;

        printf("\n%2d|%c%c%c%c|%c%c%c|%7.2f s %6d frm| %7d|", i,
            (tr->handler_type >> 24), (tr->handler_type >> 16), (tr->handler_type >> 8), (tr->handler_type >> 0),
            tr->language[0], tr->language[1], tr->language[2],
            (65536.0*65536.0*tr->duration_hi + tr->duration_lo) / tr->timescale,
            tr->sample_count,
            tr->avg_bitrate_bps);

        printf(" %-23s|", GetMP4StreamTypeName(tr->stream_type));
        printf(" %-23s", GetMP4ObjectTypeName(tr->object_type_indication));

        if (tr->handler_type == MP4D_HANDLER_TYPE_SOUN)
        {
            printf("  -  %d ch %d hz", tr->SampleDescription.audio.channelcount, tr->SampleDescription.audio.samplerate_hz);
        } else if (tr->handler_type == MP4D_HANDLER_TYPE_VIDE)
        {
            printf("  -  %dx%d", tr->SampleDescription.video.width, tr->SampleDescription.video.height);
        }
    }
    printf("\n");
}

#endif // MP4D_PRINT_INFO_SUPPORTED

#endif


