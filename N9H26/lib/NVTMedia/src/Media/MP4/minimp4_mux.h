#ifndef __MINIMP4_MUX_H_
#define __MINIMP4_MUX_H_

/*
    https://github.com/aspt/mp4
    https://github.com/lieff/minimp4
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "minimp4_comm.h"

/************************************************************************/
/*          API error codes                                             */
/************************************************************************/
#define MP4E_STATUS_OK                       0
#define MP4E_STATUS_BAD_ARGUMENTS           -1
#define MP4E_STATUS_NO_MEMORY               -2
#define MP4E_STATUS_FILE_WRITE_ERROR        -3
#define MP4E_STATUS_ONLY_ONE_DSI_ALLOWED    -4

/************************************************************************/
/*          Sample kind for MP4E_put_sample()                           */
/************************************************************************/
#define MP4E_SAMPLE_DEFAULT             0   // (beginning of) audio or video frame
#define MP4E_SAMPLE_RANDOM_ACCESS       1   // mark sample as random access point (key frame)
#define MP4E_SAMPLE_CONTINUATION        2   // Not a sample, but continuation of previous sample (new slice)

/************************************************************************/
/*          Data structures                                             */
/************************************************************************/

typedef struct MP4E_mux_tag MP4E_mux_t;

typedef enum
{
    e_audio,
    e_video,
    e_private
} track_media_kind_t;

typedef struct
{
    // MP4 object type code, which defined codec class for the track.
    // See MP4E_OBJECT_TYPE_* values for some codecs
    unsigned object_type_indication;

    // Track language: 3-char ISO 639-2T code: "und", "eng", "rus", "jpn" etc...
    unsigned char language[4];

    track_media_kind_t track_media_kind;

    // 90000 for video, sample rate for audio
    unsigned time_scale;
    unsigned default_duration;

    union
    {
        struct
        {
            // number of channels in the audio track.
            unsigned channelcount;
        } a;

        struct
        {
            int width;
            int height;
        } v;
    } u;

} MP4E_track_t;

typedef struct
{
#if MINIMP4_TRANSCODE_SPS_ID
    h264_sps_id_patcher_t sps_patcher;
#endif
    MP4E_mux_t *mux;
    int mux_track_id, is_hevc, need_vps, need_sps, need_pps, need_idr;
} mp4_h26x_writer_t;

int mp4_h26x_write_init(mp4_h26x_writer_t *h, MP4E_mux_t *mux, int width, int height, int is_hevc);
void mp4_h26x_write_close(mp4_h26x_writer_t *h);
int mp4_h26x_write_nal(mp4_h26x_writer_t *h, const unsigned char *nal, int length, unsigned timeStamp90kHz_next);

/************************************************************************/
/*          API                                                         */
/************************************************************************/


/**
*   Allocates and initialize mp4 multiplexor
*   Given file handler is transparent to the MP4 library, and used only as
*   argument for given fwrite_callback() function.  By appropriate definition
*   of callback function application may use any other file output API (for
*   example C++ streams, or Win32 file functions)
*
*   return multiplexor handle on success; NULL on failure
*/
MP4E_mux_t *MP4E_open(int sequential_mode_flag, int enable_fragmentation, void *token,
    int (*write_callback)(int64_t offset, const void *buffer, size_t size, void *token));

/**
*   Add new track
*   The track_data parameter does not referred by the multiplexer after function
*   return, and may be allocated in short-time memory. The dsi member of
*   track_data parameter is mandatory.
*
*   return ID of added track, or error code MP4E_STATUS_*
*/
int MP4E_add_track(MP4E_mux_t *mux, const MP4E_track_t *track_data);

/**
*   Add new sample to specified track
*   The tracks numbered starting with 0, according to order of MP4E_add_track() calls
*   'kind' is one of MP4E_SAMPLE_... defines
*
*   return error code MP4E_STATUS_*
*
*   Example:
*       MP4E_put_sample(mux, 0, data, data_bytes, duration, MP4E_SAMPLE_DEFAULT);
*/
int MP4E_put_sample(MP4E_mux_t *mux, int track_num, const void *data, int data_bytes, int duration, int kind);

/**
*   Finalize MP4 file, de-allocated memory, and closes MP4 multiplexer.
*   The close operation takes a time and disk space, since it writes MP4 file
*   indexes.  Please note that this function does not closes file handle,
*   which was passed to open function.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_close(MP4E_mux_t *mux);

/**
*   Set Decoder Specific Info (DSI)
*   Can be used for audio and private tracks.
*   MUST be used for AAC track.
*   Only one DSI can be set. It is an error to set DSI again
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_dsi(MP4E_mux_t *mux, int track_id, const void *dsi, int bytes);

/**
*   Set VPS data. MUST be used for HEVC (H.265) track.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_vps(MP4E_mux_t *mux, int track_id, const void *vps, int bytes);

/**
*   Set SPS data. MUST be used for AVC (H.264) track. Up to 32 different SPS can be used in one track.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_sps(MP4E_mux_t *mux, int track_id, const void *sps, int bytes);

/**
*   Set PPS data. MUST be used for AVC (H.264) track. Up to 256 different PPS can be used in one track.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_pps(MP4E_mux_t *mux, int track_id, const void *pps, int bytes);

/**
*   Set or replace ASCII test comment for the file. Set comment to NULL to remove comment.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_text_comment(MP4E_mux_t *mux, const char *comment);

// Video track : 'vide'
#define MP4E_HANDLER_TYPE_VIDE                                  0x76696465
// Audio track : 'soun'
#define MP4E_HANDLER_TYPE_SOUN                                  0x736F756E
// General MPEG-4 systems streams (without specific handler).
// Used for private stream, as suggested in http://www.mp4ra.org/handler.html
#define MP4E_HANDLER_TYPE_GESM                                  0x6765736D

typedef struct
{
    boxsize_t size;
    boxsize_t offset;
    unsigned duration;
    unsigned flag_random_access;
} sample_t;

typedef struct {
    unsigned char *data;
    int bytes;
    int capacity;
} minimp4_vector_t;

typedef struct
{
    MP4E_track_t info;
    minimp4_vector_t smpl;  // sample descriptor
    minimp4_vector_t pending_sample;

    minimp4_vector_t vsps;  // or dsi for audio
    minimp4_vector_t vpps;  // not used for audio
    minimp4_vector_t vvps;  // used for HEVC

} track_t;

typedef struct MP4E_mux_tag
{
    minimp4_vector_t tracks;

    int64_t write_pos;
    int (*write_callback)(int64_t offset, const void *buffer, size_t size, void *token);
    void *token;
    char *text_comment;

    int sequential_mode_flag;
    int enable_fragmentation; // flag, indicating streaming-friendly 'fragmentation' mode
    int fragments_count;      // # of fragments in 'fragmentation' mode

} MP4E_mux_t;

static const unsigned char box_ftyp[] = {
#if 1
    0,0,0,0x18,'f','t','y','p',
    'm','p','4','2',0,0,0,0,
    'm','p','4','2','i','s','o','m',
#else
    // as in ffmpeg
    0,0,0,0x20,'f','t','y','p',
    'i','s','o','m',0,0,2,0,
    'm','p','4','1','i','s','o','m',
    'i','s','o','2','a','v','c','1',
#endif
};

/**
*   Endian-independent byte-write macros
*/
#define WR(x, n) *p++ = (unsigned char)((x) >> 8*n)
#define WRITE_1(x) WR(x, 0);
#define WRITE_2(x) WR(x, 1); WR(x, 0);
#define WRITE_3(x) WR(x, 2); WR(x, 1); WR(x, 0);
#define WRITE_4(x) WR(x, 3); WR(x, 2); WR(x, 1); WR(x, 0);
#define WR4(p, x) (p)[0] = (char)((x) >> 8*3); (p)[1] = (char)((x) >> 8*2); (p)[2] = (char)((x) >> 8*1); (p)[3] = (char)((x));

// Finish atom: update atom size field
#define END_ATOM --stack; WR4((unsigned char*)*stack, p - *stack);

// Initiate atom: save position of size field on stack
#define ATOM(x)  *stack++ = p; p += 4; WRITE_4(x);

// Atom with 'FullAtomVersionFlags' field
#define ATOM_FULL(x, flag)  ATOM(x); WRITE_4(flag);

#define ERR(func) { int err = func; if (err) return err; }

/**
    Allocate vector with given size, return 1 on success, 0 on fail
*/
static int minimp4_vector_init(minimp4_vector_t *h, int capacity)
{
    h->bytes = 0;
    h->capacity = capacity;
    h->data = capacity ? (unsigned char *)malloc(capacity) : NULL;
    return !capacity || !!h->data;
}

/**
    Deallocates vector memory
*/
static void minimp4_vector_reset(minimp4_vector_t *h)
{
    if (h->data)
        free(h->data);
    memset(h, 0, sizeof(minimp4_vector_t));
}

/**
    Reallocate vector memory to the given size
*/
static int minimp4_vector_grow(minimp4_vector_t *h, int bytes)
{
    void *p;
    int new_size = h->capacity*2 + 1024;
    if (new_size < h->capacity + bytes)
        new_size = h->capacity + bytes + 1024;
    p = realloc(h->data, new_size);
    if (!p)
        return 0;
    h->data = (unsigned char*)p;
    h->capacity = new_size;
    return 1;
}

/**
    Allocates given number of bytes at the end of vector data, increasing
    vector memory if necessary.
    Return allocated memory.
*/
static unsigned char *minimp4_vector_alloc_tail(minimp4_vector_t *h, int bytes)
{
    unsigned char *p;
    if (!h->data && !minimp4_vector_init(h, 2*bytes + 1024))
        return NULL;
    if ((h->capacity - h->bytes) < bytes && !minimp4_vector_grow(h, bytes))
        return NULL;
    assert(h->data);
    assert((h->capacity - h->bytes) >= bytes);
    p = h->data + h->bytes;
    h->bytes += bytes;
    return p;
}

/**
    Append data to the end of the vector (accumulate ot enqueue)
*/
static unsigned char *minimp4_vector_put(minimp4_vector_t *h, const void *buf, int bytes)
{
    unsigned char *tail = minimp4_vector_alloc_tail(h, bytes);
    if (tail)
        memcpy(tail, buf, bytes);
    return tail;
}

/**
*   Allocates and initialize mp4 multiplexer
*   return multiplexor handle on success; NULL on failure
*/
MP4E_mux_t *MP4E_open(int sequential_mode_flag, int enable_fragmentation, void *token,
    int (*write_callback)(int64_t offset, const void *buffer, size_t size, void *token))
{
    if (write_callback(0, box_ftyp, sizeof(box_ftyp), token)) // Write fixed header: 'ftyp' box
        return 0;
    MP4E_mux_t *mux = (MP4E_mux_t*)malloc(sizeof(MP4E_mux_t));
    if (!mux)
        return mux;
    mux->sequential_mode_flag = sequential_mode_flag || enable_fragmentation;
    mux->enable_fragmentation = enable_fragmentation;
    mux->fragments_count = 0;
    mux->write_callback = write_callback;
    mux->token = token;
    mux->text_comment = NULL;
    mux->write_pos = sizeof(box_ftyp);

    if (!mux->sequential_mode_flag)
    {   // Write filler, which would be updated later
        if (mux->write_callback(mux->write_pos, box_ftyp, 8, mux->token))
        {
            free(mux);
            return 0;
        }
        mux->write_pos += 16; // box_ftyp + box_free for 32bit or 64bit size encoding
    }
    minimp4_vector_init(&mux->tracks, 2*sizeof(track_t));
    return mux;
}

/**
*   Add new track
*/
int MP4E_add_track(MP4E_mux_t *mux, const MP4E_track_t *track_data)
{
    track_t *tr;
    int ntr = mux->tracks.bytes / sizeof(track_t);

    if (!mux || !track_data)
        return MP4E_STATUS_BAD_ARGUMENTS;

    tr = (track_t*)minimp4_vector_alloc_tail(&mux->tracks, sizeof(track_t));
    if (!tr)
        return MP4E_STATUS_NO_MEMORY;
    memset(tr, 0, sizeof(track_t));
    memcpy(&tr->info, track_data, sizeof(*track_data));
    if (!minimp4_vector_init(&tr->smpl, 256))
        return MP4E_STATUS_NO_MEMORY;
    minimp4_vector_init(&tr->vsps, 0);
    minimp4_vector_init(&tr->vpps, 0);
    minimp4_vector_init(&tr->pending_sample, 0);
    return ntr;
}

static const unsigned char *next_dsi(const unsigned char *p, const unsigned char *end, int *bytes)
{
    if (p < end + 2)
    {
        *bytes = p[0]*256 + p[1];
        return p + 2;
    } else
        return NULL;
}

static int append_mem(minimp4_vector_t *v, const void *mem, int bytes)
{
    int i;
    unsigned char size[2];
    const unsigned char *p = v->data;
    for (i = 0; i + 2 < v->bytes;)
    {
        int cb = p[i]*256 + p[i + 1];
        if (cb == bytes && !memcmp(p + i + 2, mem, cb))
            return 1;
        i += 2 + cb;
    }
    size[0] = bytes >> 8;
    size[1] = bytes;
    return minimp4_vector_put(v, size, 2) && minimp4_vector_put(v, mem, bytes);
}

static int items_count(minimp4_vector_t *v)
{
    int i, count = 0;
    const unsigned char *p = v->data;
    for (i = 0; i + 2 < v->bytes;)
    {
        int cb = p[i]*256 + p[i + 1];
        count++;
        i += 2 + cb;
    }
    return count;
}

int MP4E_set_dsi(MP4E_mux_t *mux, int track_id, const void *dsi, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_audio ||
           tr->info.track_media_kind == e_private);
    if (tr->vsps.bytes)
        return MP4E_STATUS_ONLY_ONE_DSI_ALLOWED;   // only one DSI allowed
    return append_mem(&tr->vsps, dsi, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

int MP4E_set_vps(MP4E_mux_t *mux, int track_id, const void *vps, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_video);
    return append_mem(&tr->vvps, vps, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

int MP4E_set_sps(MP4E_mux_t *mux, int track_id, const void *sps, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_video);
    return append_mem(&tr->vsps, sps, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

int MP4E_set_pps(MP4E_mux_t *mux, int track_id, const void *pps, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_video);
    return append_mem(&tr->vpps, pps, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

static unsigned get_duration(const track_t *tr)
{
    unsigned i, sum_duration = 0;
    const sample_t *s = (const sample_t *)tr->smpl.data;
    for (i = 0; i < tr->smpl.bytes/sizeof(sample_t); i++)
    {
        sum_duration += s[i].duration;
    }
    return sum_duration;
}

static int write_pending_data(MP4E_mux_t *mux, track_t *tr)
{
    // if have pending sample && have at least one sample in the index
    if (tr->pending_sample.bytes > 0 && tr->smpl.bytes >= sizeof(sample_t))
    {
        // Complete pending sample
        sample_t *smpl_desc;
        unsigned char base[8], *p = base;

        assert(mux->sequential_mode_flag);

        // Write each sample to a separate atom
        assert(mux->sequential_mode_flag);      // Separate atom needed for sequential_mode only
        WRITE_4(tr->pending_sample.bytes + 8);
        WRITE_4(BOX_mdat);
        ERR(mux->write_callback(mux->write_pos, base, p - base, mux->token));
        mux->write_pos += p - base;

        // Update sample descriptor with size and offset
        smpl_desc = ((sample_t*)minimp4_vector_alloc_tail(&tr->smpl, 0)) - 1;
        smpl_desc->size = tr->pending_sample.bytes;
        smpl_desc->offset = (boxsize_t)mux->write_pos;

        // Write data
        ERR(mux->write_callback(mux->write_pos, tr->pending_sample.data, tr->pending_sample.bytes, mux->token));
        mux->write_pos += tr->pending_sample.bytes;

        // reset buffer
        tr->pending_sample.bytes = 0;
    }
    return MP4E_STATUS_OK;
}

static int add_sample_descriptor(MP4E_mux_t *mux, track_t *tr, int data_bytes, int duration, int kind)
{
    sample_t smp;
    smp.size = data_bytes;
    smp.offset = (boxsize_t)mux->write_pos;
    smp.duration = (duration ? duration : tr->info.default_duration);
    smp.flag_random_access = (kind == MP4E_SAMPLE_RANDOM_ACCESS);
    return NULL != minimp4_vector_put(&tr->smpl, &smp, sizeof(sample_t));
}

static int mp4e_flush_index(MP4E_mux_t *mux);

/**
*   Write Movie Fragment: 'moof' box
*/
static int mp4e_write_fragment_header(MP4E_mux_t *mux, int track_num, int data_bytes, int duration, int kind)
{
    unsigned char base[888], *p = base;
    unsigned char *stack_base[20]; // atoms nesting stack
    unsigned char **stack = stack_base;
    unsigned char *pdata_offset;
    unsigned flags;
    enum
    {
        default_sample_duration_present = 0x000008,
        default_sample_flags_present = 0x000020,
    } e;

    track_t *tr = ((track_t*)mux->tracks.data) + track_num;

    ATOM(BOX_moof)
        ATOM_FULL(BOX_mfhd, 0)
            WRITE_4(mux->fragments_count);  // start from 1
        END_ATOM
        ATOM(BOX_traf)
            flags = 0;
            if (tr->info.track_media_kind == e_video)
                flags |= 0x20;          // default-sample-flags-present
            else
                flags |= 0x08;          // default-sample-duration-present
            flags =  (tr->info.track_media_kind == e_video) ? 0x20020 : 0x20008;

            ATOM_FULL(BOX_tfhd, flags)
                WRITE_4(track_num + 1); // track_ID
                if (tr->info.track_media_kind == e_video)
                {
                    flags  = 0x001;     // data-offset-present
                    flags |= 0x100;     // sample-duration-present
                    WRITE_4(0x1010000); // default_sample_flags
                } else
                {
                    WRITE_4(duration);
                }
            END_ATOM
            if (tr->info.track_media_kind == e_audio)
            {
                flags  = 0;
                flags |= 0x001;         // data-offset-present
                flags |= 0x200;         // sample-size-present
                ATOM_FULL(BOX_trun, flags)
                    WRITE_4(1);         // sample_count
                    pdata_offset = p; p += 4;  // save ptr to data_offset
                    WRITE_4(duration);  // sample_duration
                END_ATOM
            } else if (kind == MP4E_SAMPLE_RANDOM_ACCESS)
            {
                flags  = 0;
                flags |= 0x001;         // data-offset-present
                flags |= 0x004;         // first-sample-flags-present
                flags |= 0x100;         // sample-duration-present
                flags |= 0x200;         // sample-size-present
                ATOM_FULL(BOX_trun, flags)
                    WRITE_4(1);         // sample_count
                    pdata_offset = p; p += 4;   // save ptr to data_offset
                    WRITE_4(0x2000000); // first_sample_flags
                    WRITE_4(duration);  // sample_duration
                    WRITE_4(data_bytes);// sample_size
                END_ATOM
            } else
            {
                flags  = 0;
                flags |= 0x001;         // data-offset-present
                flags |= 0x100;         // sample-duration-present
                flags |= 0x200;         // sample-size-present
                ATOM_FULL(BOX_trun, flags)
                    WRITE_4(1);         // sample_count
                    pdata_offset = p; p += 4;   // save ptr to data_offset
                    WRITE_4(duration);  // sample_duration
                    WRITE_4(data_bytes);// sample_size
                END_ATOM
            }
        END_ATOM
    END_ATOM
    WR4(pdata_offset, (p - base) + 8);

    ERR(mux->write_callback(mux->write_pos, base, p - base, mux->token));
    mux->write_pos += p - base;
    return MP4E_STATUS_OK;
}


static int mp4e_write_mdat_box(MP4E_mux_t *mux, uint32_t size)
{
    unsigned char base[8], *p = base;
    WRITE_4(size);
    WRITE_4(BOX_mdat);
    ERR(mux->write_callback(mux->write_pos, base, p - base, mux->token));
    mux->write_pos += p - base;
    return MP4E_STATUS_OK;
}

/**
*   Add new sample to specified track
*/
#if 0
int MP4E_put_sample(MP4E_mux_t *mux, int track_num, const void *data, int data_bytes, int duration, int kind)
{
    track_t *tr;
    if (!mux || !data)
        return MP4E_STATUS_BAD_ARGUMENTS;
    tr = ((track_t*)mux->tracks.data) + track_num;

    if (mux->enable_fragmentation)
    {
        if (!mux->fragments_count++)
            ERR(mp4e_flush_index(mux)); // write file headers before 1st sample
        // write MOOF + MDAT + sample data
        ERR(mp4e_write_fragment_header(mux, track_num, data_bytes, duration, kind));
        // write MDAT box for each sample
        ERR(mp4e_write_mdat_box(mux, data_bytes + 8));
        ERR(mux->write_callback(mux->write_pos, data, data_bytes, mux->token));
        mux->write_pos += data_bytes;
        return MP4E_STATUS_OK;
    }

    if (kind != MP4E_SAMPLE_CONTINUATION)
    {
        if (mux->sequential_mode_flag)
            ERR(write_pending_data(mux, tr));
        if (!add_sample_descriptor(mux, tr, data_bytes, duration, kind))
            return MP4E_STATUS_NO_MEMORY;
    } else
    {
        if (!mux->sequential_mode_flag)
        {
            sample_t *smpl_desc;
            if (tr->smpl.bytes < sizeof(sample_t))
                return MP4E_STATUS_NO_MEMORY; // write continuation, but there are no samples in the index
            // Accumulate size of the continuation in the sample descriptor
            smpl_desc = (sample_t*)(tr->smpl.data + tr->smpl.bytes) - 1;
            smpl_desc->size += data_bytes;
        }
    }

    if (mux->sequential_mode_flag)
    {
        if (!minimp4_vector_put(&tr->pending_sample, data, data_bytes))
            return MP4E_STATUS_NO_MEMORY;
    } else
    {
        ERR(mux->write_callback(mux->write_pos, data, data_bytes, mux->token));
        mux->write_pos += data_bytes;
    }
    return MP4E_STATUS_OK;
}

#else
int MP4E_put_sample(MP4E_mux_t *mux, int track_num, const void *data, int data_bytes, int duration, int kind)
{
    track_t *tr;
    if (!mux || !data)
        return MP4E_STATUS_BAD_ARGUMENTS;
    tr = ((track_t*)mux->tracks.data) + track_num;

    if (mux->enable_fragmentation)
    {
        if (!mux->fragments_count++)
            ERR(mp4e_flush_index(mux)); // write file headers before 1st sample
        // write MOOF + MDAT + sample data
        ERR(mp4e_write_fragment_header(mux, track_num, data_bytes, duration, kind));
        // write MDAT box for each sample
        ERR(mp4e_write_mdat_box(mux, data_bytes + 8));

        if (!add_sample_descriptor(mux, tr, data_bytes, duration, kind))
            return MP4E_STATUS_NO_MEMORY;

        ERR(mux->write_callback(mux->write_pos, data, data_bytes, mux->token));
        mux->write_pos += data_bytes;
        return MP4E_STATUS_OK;
    }
		
    if (mux->sequential_mode_flag == 0){
        ERR(mp4e_write_mdat_box(mux, data_bytes + 8));
		}
		
		if (!add_sample_descriptor(mux, tr, data_bytes, duration, kind))
				return MP4E_STATUS_NO_MEMORY;

    if (mux->sequential_mode_flag)
    {
        if (!minimp4_vector_put(&tr->pending_sample, data, data_bytes))
            return MP4E_STATUS_NO_MEMORY;
    } else
    {
        ERR(mux->write_callback(mux->write_pos, data, data_bytes, mux->token));
        mux->write_pos += data_bytes;
    }
    return MP4E_STATUS_OK;
}

#endif


/**
*   calculate size of length field of OD box
*/
static int od_size_of_size(int size)
{
    int i, size_of_size = 1;
    for (i = size; i > 0x7F; i -= 0x7F)
        size_of_size++;
    return size_of_size;
}

/**
*   Add or remove MP4 file text comment according to Apple specs:
*   https://developer.apple.com/library/mac/documentation/QuickTime/QTFF/Metadata/Metadata.html#//apple_ref/doc/uid/TP40000939-CH1-SW1
*   http://atomicparsley.sourceforge.net/mpeg-4files.html
*   note that ISO did not specify comment format.
*/
int MP4E_set_text_comment(MP4E_mux_t *mux, const char *comment)
{
    if (!mux || !comment)
        return MP4E_STATUS_BAD_ARGUMENTS;
    if (mux->text_comment)
        free(mux->text_comment);
    mux->text_comment = NMUtil_strdup(comment);
    if (!mux->text_comment)
        return MP4E_STATUS_NO_MEMORY;
    return MP4E_STATUS_OK;
}

/**
*   Write file index 'moov' box with all its boxes and indexes
*/
static int mp4e_flush_index(MP4E_mux_t *mux)
{
    unsigned char *stack_base[20]; // atoms nesting stack
    unsigned char **stack = stack_base;
    unsigned char *base, *p;
    unsigned int ntr, index_bytes, ntracks = mux->tracks.bytes / sizeof(track_t);
    int i, err;

    // How much memory needed for indexes
    // Experimental data:
    // file with 1 track = 560 bytes
    // file with 2 tracks = 972 bytes
    // track size = 412 bytes;
    // file header size = 148 bytes
#define FILE_HEADER_BYTES 256
#define TRACK_HEADER_BYTES 512
    index_bytes = FILE_HEADER_BYTES;
    if (mux->text_comment)
        index_bytes += 128 + strlen(mux->text_comment);
    for (ntr = 0; ntr < ntracks; ntr++)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + ntr;
        index_bytes += TRACK_HEADER_BYTES;          // fixed amount (implementation-dependent)
        // may need extra 4 bytes for duration field + 4 bytes for worst-case random access box
        index_bytes += tr->smpl.bytes * (sizeof(sample_t) + 4 + 4) / sizeof(sample_t);
        index_bytes += tr->vsps.bytes;
        index_bytes += tr->vpps.bytes;

        ERR(write_pending_data(mux, tr));
    }

    base = (unsigned char*)malloc(index_bytes);
    if (!base)
        return MP4E_STATUS_NO_MEMORY;
    p = base;

    if (!mux->sequential_mode_flag)
    {
        // update size of mdat box.
        // One of 2 points, which requires random file access.
        // Second is optonal duration update at beginning of file in fragmenatation mode.
        // This can be avoided using "till eof" size code, but in this case indexes must be
        // written before the mdat....
        int64_t size = mux->write_pos - sizeof(box_ftyp);
        const int64_t size_limit = (int64_t)(uint64_t)0xfffffffe;
        if (size > size_limit)
        {
            WRITE_4(1);
            WRITE_4(BOX_mdat);
            WRITE_4((size >> 32) & 0xffffffff);
            WRITE_4(size & 0xffffffff);
        } else
        {
            WRITE_4(8);
            WRITE_4(BOX_free);
            WRITE_4(size - 8);
            WRITE_4(BOX_mdat);
        }
        ERR(mux->write_callback(sizeof(box_ftyp), base, p - base, mux->token));
        p = base;
    }

    // Write index atoms; order taken from Table 1 of [1]
#define MOOV_TIMESCALE 1000
    ATOM(BOX_moov);
        ATOM_FULL(BOX_mvhd, 0);
        WRITE_4(0); // creation_time
        WRITE_4(0); // modification_time

        if (ntracks)
        {
            track_t *tr = ((track_t*)mux->tracks.data) + 0;    // take 1st track
            unsigned duration = get_duration(tr);
            duration = (unsigned)(duration * 1LL * MOOV_TIMESCALE / tr->info.time_scale);
            WRITE_4(MOOV_TIMESCALE); // duration
            WRITE_4(duration); // duration
        }

        WRITE_4(0x00010000); // rate
        WRITE_2(0x0100); // volume
        WRITE_2(0); // reserved
        WRITE_4(0); // reserved
        WRITE_4(0); // reserved

        // matrix[9]
        WRITE_4(0x00010000); WRITE_4(0); WRITE_4(0);
        WRITE_4(0); WRITE_4(0x00010000); WRITE_4(0);
        WRITE_4(0); WRITE_4(0); WRITE_4(0x40000000);

        // pre_defined[6]
        WRITE_4(0); WRITE_4(0); WRITE_4(0);
        WRITE_4(0); WRITE_4(0); WRITE_4(0);

        //next_track_ID is a non-zero integer that indicates a value to use for the track ID of the next track to be
        //added to this presentation. Zero is not a valid track ID value. The value of next_track_ID shall be
        //larger than the largest track-ID in use.
        WRITE_4(ntracks + 1);
        END_ATOM;

    for (ntr = 0; ntr < ntracks; ntr++)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + ntr;
        unsigned duration = get_duration(tr);
        int samples_count = tr->smpl.bytes / sizeof(sample_t);
        const sample_t *sample = (const sample_t *)tr->smpl.data;
        unsigned handler_type;
        const char *handler_ascii = NULL;

        if (mux->enable_fragmentation)
            samples_count = 0;
        else if (samples_count <= 0)
            continue;   // skip empty track

        switch (tr->info.track_media_kind)
        {
            case e_audio:
                handler_type = MP4E_HANDLER_TYPE_SOUN;
                handler_ascii = "SoundHandler";
                break;
            case e_video:
                handler_type = MP4E_HANDLER_TYPE_VIDE;
                handler_ascii = "VideoHandler";
                break;
            case e_private:
                handler_type = MP4E_HANDLER_TYPE_GESM;
                break;
            default:
                return MP4E_STATUS_BAD_ARGUMENTS;
        }

        ATOM(BOX_trak);
            ATOM_FULL(BOX_tkhd, 7); // flag: 1=trak enabled; 2=track in movie; 4=track in preview
            WRITE_4(0);             // creation_time
            WRITE_4(0);             // modification_time
            WRITE_4(ntr + 1);       // track_ID
            WRITE_4(0);             // reserved
            WRITE_4((unsigned)(duration * 1LL * MOOV_TIMESCALE / tr->info.time_scale));
            WRITE_4(0); WRITE_4(0); // reserved[2]
            WRITE_2(0);             // layer
            WRITE_2(0);             // alternate_group
            WRITE_2(0x0100);        // volume {if track_is_audio 0x0100 else 0};
            WRITE_2(0);             // reserved

            // matrix[9]
            WRITE_4(0x00010000); WRITE_4(0); WRITE_4(0);
            WRITE_4(0); WRITE_4(0x00010000); WRITE_4(0);
            WRITE_4(0); WRITE_4(0); WRITE_4(0x40000000);

            if (tr->info.track_media_kind == e_audio || tr->info.track_media_kind == e_private)
            {
                WRITE_4(0); // width
                WRITE_4(0); // height
            } else
            {
                WRITE_4(tr->info.u.v.width*0x10000);  // width
                WRITE_4(tr->info.u.v.height*0x10000); // height
            }
            END_ATOM;

            ATOM(BOX_mdia);
                ATOM_FULL(BOX_mdhd, 0);
                WRITE_4(0); // creation_time
                WRITE_4(0); // modification_time
                WRITE_4(tr->info.time_scale);
                WRITE_4(duration); // duration
                {
                    int lang_code = ((tr->info.language[0] & 31) << 10) | ((tr->info.language[1] & 31) << 5) | (tr->info.language[2] & 31);
                    WRITE_2(lang_code); // language
                }
                WRITE_2(0); // pre_defined
                END_ATOM;

                ATOM_FULL(BOX_hdlr, 0);
                WRITE_4(0); // pre_defined
                WRITE_4(handler_type); // handler_type
                WRITE_4(0); WRITE_4(0); WRITE_4(0); // reserved[3]
                // name is a null-terminated string in UTF-8 characters which gives a human-readable name for the track type (for debugging and inspection purposes).
                // set mdia hdlr name field to what quicktime uses.
                // Sony smartphone may fail to decode short files w/o handler name
                if (handler_ascii)
                {
                    for (i = 0; i < (int)strlen(handler_ascii) + 1; i++)
                    {
                        WRITE_1(handler_ascii[i]);
                    }
                } else
                {
                    WRITE_4(0);
                }
                END_ATOM;

                ATOM(BOX_minf);

                    if (tr->info.track_media_kind == e_audio)
                    {
                        // Sound Media Header Box
                        ATOM_FULL(BOX_smhd, 0);
                        WRITE_2(0);   // balance
                        WRITE_2(0);   // reserved
                        END_ATOM;
                    }
                    if (tr->info.track_media_kind == e_video)
                    {
                        // mandatory Video Media Header Box
                        ATOM_FULL(BOX_vmhd, 1);
                        WRITE_2(0); // graphicsmode
                        WRITE_2(0); WRITE_2(0); WRITE_2(0); // opcolor[3]
                        END_ATOM;
                    }

                    ATOM(BOX_dinf);
                        ATOM_FULL(BOX_dref, 0);
                        WRITE_4(1); // entry_count
                            // If the flag is set indicating that the data is in the same file as this box, then no string (not even an empty one)
                            // shall be supplied in the entry field.

                            // ASP the correct way to avoid supply the string, is to use flag 1
                            // otherwise ISO reference demux crashes
                            ATOM_FULL(BOX_url, 1);
                            END_ATOM;
                        END_ATOM;
                    END_ATOM;

                    ATOM(BOX_stbl);
                        ATOM_FULL(BOX_stsd, 0);
                        WRITE_4(1); // entry_count;

                        if (tr->info.track_media_kind == e_audio || tr->info.track_media_kind == e_private)
                        {
                            // AudioSampleEntry() assume MP4E_HANDLER_TYPE_SOUN
                            if (tr->info.track_media_kind == e_audio)
                            {
                                ATOM(BOX_mp4a);
                            } else
                            {
                                ATOM(BOX_mp4s);
                            }

                            // SampleEntry
                            WRITE_4(0); WRITE_2(0); // reserved[6]
                            WRITE_2(1); // data_reference_index; - this is a tag for descriptor below

                            if (tr->info.track_media_kind == e_audio)
                            {
                                // AudioSampleEntry
                                WRITE_4(0); WRITE_4(0); // reserved[2]
                                WRITE_2(tr->info.u.a.channelcount); // channelcount
                                WRITE_2(16); // samplesize
                                WRITE_4(0);  // pre_defined+reserved
                                WRITE_4((tr->info.time_scale << 16));  // samplerate == = {timescale of media}<<16;
                            }

                                ATOM_FULL(BOX_esds, 0);
                                if (tr->vsps.bytes > 0)
                                {
                                    int dsi_bytes = tr->vsps.bytes - 2; //  - two bytes size field
                                    int dsi_size_size = od_size_of_size(dsi_bytes);
                                    int dcd_bytes = dsi_bytes + dsi_size_size + 1 + (1 + 1 + 3 + 4 + 4);
                                    int dcd_size_size = od_size_of_size(dcd_bytes);
                                    int esd_bytes = dcd_bytes + dcd_size_size + 1 + 3;

#define WRITE_OD_LEN(size) if (size > 0x7F) do { size -= 0x7F; WRITE_1(0x00ff); } while (size > 0x7F); WRITE_1(size)
                                    WRITE_1(3); // OD_ESD
                                    WRITE_OD_LEN(esd_bytes);
                                    WRITE_2(0); // ES_ID(2) // TODO - what is this?
                                    WRITE_1(0); // flags(1)

                                    WRITE_1(4); // OD_DCD
                                    WRITE_OD_LEN(dcd_bytes);
                                    if (tr->info.track_media_kind == e_audio)
                                    {
                                        WRITE_1(MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3); // OD_DCD
                                        WRITE_1(5 << 2); // stream_type == AudioStream
                                    } else
                                    {
                                        // http://xhelmboyx.tripod.com/formats/mp4-layout.txt
                                        WRITE_1(208); // 208 = private video
                                        WRITE_1(32 << 2); // stream_type == user private
                                    }
                                    WRITE_3(tr->info.u.a.channelcount * 6144/8); // bufferSizeDB in bytes, constant as in reference decoder
                                    WRITE_4(0); // maxBitrate TODO
                                    WRITE_4(0); // avg_bitrate_bps TODO

                                    WRITE_1(5); // OD_DSI
                                    WRITE_OD_LEN(dsi_bytes);
                                    for (i = 0; i < dsi_bytes; i++)
                                    {
                                        WRITE_1(tr->vsps.data[2 + i]);
                                    }
                                }
                                END_ATOM;
                            END_ATOM;
                        }

                        if (tr->info.track_media_kind == e_video && (MP4_OBJECT_TYPE_AVC == tr->info.object_type_indication || MP4_OBJECT_TYPE_HEVC == tr->info.object_type_indication))
                        {
                            int numOfSequenceParameterSets = items_count(&tr->vsps);
                            int numOfPictureParameterSets  = items_count(&tr->vpps);
                            if (MP4_OBJECT_TYPE_AVC == tr->info.object_type_indication)
                            {
                                ATOM(BOX_avc1);
                            } else
                            {
                                ATOM(BOX_hvc1);
                            }
                            // VisualSampleEntry  8.16.2
                            // extends SampleEntry
                            WRITE_2(0); // reserved
                            WRITE_2(0); // reserved
                            WRITE_2(0); // reserved
                            WRITE_2(1); // data_reference_index

                            WRITE_2(0); // pre_defined
                            WRITE_2(0); // reserved
                            WRITE_4(0); // pre_defined
                            WRITE_4(0); // pre_defined
                            WRITE_4(0); // pre_defined
                            WRITE_2(tr->info.u.v.width);
                            WRITE_2(tr->info.u.v.height);
                            WRITE_4(0x00480000); // horizresolution = 72 dpi
                            WRITE_4(0x00480000); // vertresolution  = 72 dpi
                            WRITE_4(0); // reserved
                            WRITE_2(1); // frame_count
                            for (i = 0; i < 32; i++)
                            {
                                WRITE_1(0); //  compressorname
                            }
                            WRITE_2(24); // depth
                            WRITE_2(-1); // pre_defined

                            if (MP4_OBJECT_TYPE_AVC == tr->info.object_type_indication)
                            {
                                ATOM(BOX_avcC);
                                // AVCDecoderConfigurationRecord 5.2.4.1.1
                                WRITE_1(1); // configurationVersion
                                WRITE_1(tr->vsps.data[2 + 1]);
                                WRITE_1(tr->vsps.data[2 + 2]);
                                WRITE_1(tr->vsps.data[2 + 3]);
                                WRITE_1(255); // 0xfc + NALU_len - 1
                                WRITE_1(0xe0 | numOfSequenceParameterSets);
                                for (i = 0; i < tr->vsps.bytes; i++)
                                {
                                    WRITE_1(tr->vsps.data[i]);
                                }
                                WRITE_1(numOfPictureParameterSets);
                                for (i = 0; i < tr->vpps.bytes; i++)
                                {
                                    WRITE_1(tr->vpps.data[i]);
                                }
                            } else
                            {
                                int numOfVPS  = items_count(&tr->vpps);
                                ATOM(BOX_hvcC);
                                // TODO: read actual params from stream
                                WRITE_1(1);    // configurationVersion
                                WRITE_1(1);    // Profile Space (2), Tier (1), Profile (5)
                                WRITE_4(0x60000000); // Profile Compatibility
                                WRITE_2(0);    // progressive, interlaced, non packed constraint, frame only constraint flags
                                WRITE_4(0);    // constraint indicator flags
                                WRITE_1(0);    // level_idc
                                WRITE_2(0xf000); // Min Spatial Segmentation
                                WRITE_1(0xfc); // Parallelism Type
                                WRITE_1(0xfc); // Chroma Format
                                WRITE_1(0xf8); // Luma Depth
                                WRITE_1(0xf8); // Chroma Depth
                                WRITE_2(0);    // Avg Frame Rate
                                WRITE_1(3);    // ConstantFrameRate (2), NumTemporalLayers (3), TemporalIdNested (1), LengthSizeMinusOne (2)

                                WRITE_1(3);    // Num Of Arrays
                                WRITE_1((1 << 7) | (HEVC_NAL_VPS & 0x3f)); // Array Completeness + NAL Unit Type
                                WRITE_2(numOfVPS);
                                for (i = 0; i < tr->vvps.bytes; i++)
                                {
                                    WRITE_1(tr->vvps.data[i]);
                                }
                                WRITE_1((1 << 7) | (HEVC_NAL_SPS & 0x3f));
                                WRITE_2(numOfSequenceParameterSets);
                                for (i = 0; i < tr->vsps.bytes; i++)
                                {
                                    WRITE_1(tr->vsps.data[i]);
                                }
                                WRITE_1((1 << 7) | (HEVC_NAL_PPS & 0x3f));
                                WRITE_2(numOfPictureParameterSets);
                                for (i = 0; i < tr->vpps.bytes; i++)
                                {
                                    WRITE_1(tr->vpps.data[i]);
                                }
                            }

                            END_ATOM;
                            END_ATOM;
                        }
                        END_ATOM;

                        /************************************************************************/
                        /*      indexes                                                         */
                        /************************************************************************/

                        // Time to Sample Box
                        ATOM_FULL(BOX_stts, 0);
                        {
                            unsigned char *pentry_count = p;
                            int cnt = 1, entry_count = 0;
                            WRITE_4(0);
                            for (i = 0; i < samples_count; i++, cnt++)
                            {
                                if (i == (samples_count - 1) || sample[i].duration != sample[i + 1].duration)
                                {
                                    WRITE_4(cnt);
                                    WRITE_4(sample[i].duration);
                                    cnt = 0;
                                    entry_count++;
                                }
                            }
                            WR4(pentry_count, entry_count);
                        }
                        END_ATOM;

                        // Sample To Chunk Box
                        ATOM_FULL(BOX_stsc, 0);
                        if (mux->enable_fragmentation)
                        {
                            WRITE_4(0); // entry_count
                        } else
                        {
                            WRITE_4(1); // entry_count
                            WRITE_4(1); // first_chunk;
                            WRITE_4(1); // samples_per_chunk;
                            WRITE_4(1); // sample_description_index;
                        }
                        END_ATOM;

                        // Sample Size Box
                        ATOM_FULL(BOX_stsz, 0);
                        WRITE_4(0); // sample_size  If this field is set to 0, then the samples have different sizes, and those sizes
                                    //  are stored in the sample size table.
                        WRITE_4(samples_count);  // sample_count;
                        for (i = 0; i < samples_count; i++)
                        {
                            WRITE_4(sample[i].size);
                        }
                        END_ATOM;

                        // Chunk Offset Box
                        int is_64_bit = 0;
                        if (samples_count && sample[samples_count - 1].offset > 0xffffffff)
                            is_64_bit = 1;
                        if (!is_64_bit)
                        {
                            ATOM_FULL(BOX_stco, 0);
                            WRITE_4(samples_count);
                            for (i = 0; i < samples_count; i++)
                            {
                                WRITE_4(sample[i].offset);
                            }
                        } else
                        {
                            ATOM_FULL(BOX_co64, 0);
                            WRITE_4(samples_count);
                            for (i = 0; i < samples_count; i++)
                            {
                                WRITE_4((sample[i].offset >> 32) & 0xffffffff);
                                WRITE_4(sample[i].offset & 0xffffffff);
                            }
                        }
                        END_ATOM;

                        // Sync Sample Box
                        {
                            int ra_count = 0;
                            for (i = 0; i < samples_count; i++)
                            {
                                ra_count += !!sample[i].flag_random_access;
                            }
                            if (ra_count != samples_count)
                            {
                                // If the sync sample box is not present, every sample is a random access point.
                                ATOM_FULL(BOX_stss, 0);
                                WRITE_4(ra_count);
                                for (i = 0; i < samples_count; i++)
                                {
                                    if (sample[i].flag_random_access)
                                    {
                                        WRITE_4(i + 1);
                                    }
                                }
                                END_ATOM;
                            }
                        }
                    END_ATOM;
                END_ATOM;
            END_ATOM;
        END_ATOM;
    } // tracks loop

    if (mux->text_comment)
    {
        ATOM(BOX_udta);
            ATOM_FULL(BOX_meta, 0);
                ATOM_FULL(BOX_hdlr, 0);
                    WRITE_4(0); // pre_defined
#define MP4E_HANDLER_TYPE_MDIR  0x6d646972
                    WRITE_4(MP4E_HANDLER_TYPE_MDIR); // handler_type
                    WRITE_4(0); WRITE_4(0); WRITE_4(0); // reserved[3]
                    WRITE_4(0); // name is a null-terminated string in UTF-8 characters which gives a human-readable name for the track type (for debugging and inspection purposes).
                END_ATOM;
                ATOM(BOX_ilst);
                    ATOM(BOX_ccmt);
                        ATOM(BOX_data);
                            WRITE_4(1); // type
                            WRITE_4(0); // lang
                            for (i = 0; i < (int)strlen(mux->text_comment) + 1; i++)
                            {
                                WRITE_1(mux->text_comment[i]);
                            }
                        END_ATOM;
                    END_ATOM;
                END_ATOM;
            END_ATOM;
        END_ATOM;
    }

    if (mux->enable_fragmentation)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + 0;
        uint32_t movie_duration = get_duration(tr);

        ATOM(BOX_mvex);
            ATOM_FULL(BOX_mehd, 0);
                WRITE_4(movie_duration); // duration
            END_ATOM;
        for (ntr = 0; ntr < ntracks; ntr++)
        {
            ATOM_FULL(BOX_trex, 0);
                WRITE_4(ntr + 1);        // track_ID
                WRITE_4(1);              // default_sample_description_index
                WRITE_4(0);              // default_sample_duration
                WRITE_4(0);              // default_sample_size
                WRITE_4(0);              // default_sample_flags
            END_ATOM;
        }
        END_ATOM;
    }
    END_ATOM;   // moov atom

    assert((unsigned)(p - base) <= index_bytes);

    err = mux->write_callback(mux->write_pos, base, p - base, mux->token);
    mux->write_pos += p - base;
    free(base);
    return err;
}

int MP4E_close(MP4E_mux_t *mux)
{
    int err = MP4E_STATUS_OK;
    unsigned ntr, ntracks;
    if (!mux)
        return MP4E_STATUS_BAD_ARGUMENTS;
    if (!mux->enable_fragmentation)
        err = mp4e_flush_index(mux);
    if (mux->text_comment)
        free(mux->text_comment);
    ntracks = mux->tracks.bytes / sizeof(track_t);
    for (ntr = 0; ntr < ntracks; ntr++)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + ntr;
        minimp4_vector_reset(&tr->vsps);
        minimp4_vector_reset(&tr->vpps);
        minimp4_vector_reset(&tr->smpl);
        minimp4_vector_reset(&tr->pending_sample);
    }
    minimp4_vector_reset(&mux->tracks);
    free(mux);
    return err;
}

typedef uint32_t bs_item_t;
#define BS_BITS 32

typedef struct
{
    // Look-ahead bit cache: MSB aligned, 17 bits guaranteed, zero stuffing
    unsigned int cache;

    // Bit counter = 16 - (number of bits in wCache)
    // cache refilled when cache_free_bits >= 0
    int cache_free_bits;

    // Current read position
    const uint16_t *buf;

    // original data buffer
    const uint16_t *origin;

    // original data buffer length, bytes
    unsigned origin_bytes;
} bit_reader_t;


#define LOAD_SHORT(x) ((uint16_t)(x << 8) | (x >> 8))

static unsigned int show_bits(bit_reader_t *bs, int n)
{
    unsigned int retval;
    assert(n > 0 && n <= 16);
    retval = (unsigned int)(bs->cache >> (32 - n));
    return retval;
}

static void flush_bits(bit_reader_t *bs, int n)
{
    assert(n >= 0 && n <= 16);
    bs->cache <<= n;
    bs->cache_free_bits += n;
    if (bs->cache_free_bits >= 0)
    {
        bs->cache |= ((uint32_t)LOAD_SHORT(*bs->buf)) << bs->cache_free_bits;
        bs->buf++;
        bs->cache_free_bits -= 16;
    }
}

static unsigned int get_bits(bit_reader_t *bs, int n)
{
    unsigned int retval = show_bits(bs, n);
    flush_bits(bs, n);
    return retval;
}

static void set_pos_bits(bit_reader_t *bs, unsigned pos_bits)
{
    assert((int)pos_bits >= 0);

    bs->buf = bs->origin + pos_bits/16;
    bs->cache = 0;
    bs->cache_free_bits = 16;
    flush_bits(bs, 0);
    flush_bits(bs, pos_bits & 15);
}

static unsigned get_pos_bits(const bit_reader_t *bs)
{
    // Current bitbuffer position =
    // position of next wobits in the internal buffer
    // minus bs, available in bit cache wobits
    unsigned pos_bits = (unsigned)(bs->buf - bs->origin)*16;
    pos_bits -= 16 - bs->cache_free_bits;
    assert((int)pos_bits >= 0);
    return pos_bits;
}

static int remaining_bits(const bit_reader_t *bs)
{
    return bs->origin_bytes * 8 - get_pos_bits(bs);
}

static void init_bits(bit_reader_t *bs, const void *data, unsigned data_bytes)
{
    bs->origin = (const uint16_t *)data;
    bs->origin_bytes = data_bytes;
    set_pos_bits(bs, 0);
}

#define GetBits(n) get_bits(bs, n)

/**
*   Unsigned Golomb code
*/
static int ue_bits(bit_reader_t *bs)
{
    int clz;
    int val;
    for (clz = 0; !get_bits(bs, 1); clz++) {}
    //get_bits(bs, clz + 1);
    val = (1 << clz) - 1 + (clz ? get_bits(bs, clz) : 0);
    return val;
}


#if MINIMP4_TRANSCODE_SPS_ID

/**
*   Output bitstream
*/
typedef struct
{
    int        shift;    // bit position in the cache
    uint32_t   cache;    // bit cache
    bs_item_t  *buf;     // current position
    bs_item_t  *origin;  // initial position
} bs_t;

#define SWAP32(x) (uint32_t)((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | (((x) << 8) & 0xFF0000) | ((x & 0xFF) << 24))

static void h264e_bs_put_bits(bs_t *bs, unsigned n, unsigned val)
{
    assert(!(val >> n));
    bs->shift -= n;
    assert((unsigned)n <= 32);
    if (bs->shift < 0)
    {
        assert(-bs->shift < 32);
        bs->cache |= val >> -bs->shift;
        *bs->buf++ = SWAP32(bs->cache);
        bs->shift = 32 + bs->shift;
        bs->cache = 0;
    }
    bs->cache |= val << bs->shift;
}

static void h264e_bs_flush(bs_t *bs)
{
    *bs->buf = SWAP32(bs->cache);
}

static unsigned h264e_bs_get_pos_bits(const bs_t *bs)
{
    unsigned pos_bits = (unsigned)((bs->buf - bs->origin)*BS_BITS);
    pos_bits += BS_BITS - bs->shift;
    assert((int)pos_bits >= 0);
    return pos_bits;
}

static unsigned h264e_bs_byte_align(bs_t *bs)
{
    int pos = h264e_bs_get_pos_bits(bs);
    h264e_bs_put_bits(bs, -pos & 7, 0);
    return pos + (-pos & 7);
}

/**
*   Golomb code
*   0 => 1
*   1 => 01 0
*   2 => 01 1
*   3 => 001 00
*   4 => 001 01
*
*   [0]     => 1
*   [1..2]  => 01x
*   [3..6]  => 001xx
*   [7..14] => 0001xxx
*
*/
static void h264e_bs_put_golomb(bs_t *bs, unsigned val)
{
    int size = 0;
    unsigned t = val + 1;
    do
    {
        size++;
    } while (t >>= 1);

    h264e_bs_put_bits(bs, 2*size - 1, val + 1);
}

static void h264e_bs_init_bits(bs_t *bs, void *data)
{
    bs->origin = (bs_item_t*)data;
    bs->buf = bs->origin;
    bs->shift = BS_BITS;
    bs->cache = 0;
}

static int find_mem_cache(void *cache[], int cache_bytes[], int cache_size, void *mem, int bytes)
{
    int i;
    if (!bytes)
        return -1;
    for (i = 0; i < cache_size; i++)
    {
        if (cache_bytes[i] == bytes && !memcmp(mem, cache[i], bytes))
            return i;   // found
    }
    for (i = 0; i < cache_size; i++)
    {
        if (!cache_bytes[i])
        {
            cache[i] = malloc(bytes);
            if (cache[i])
            {
                memcpy(cache[i], mem, bytes);
                cache_bytes[i] = bytes;
            }
            return i;   // put in
        }
    }
    return -1;  // no room
}

/**
*   7.4.1.1. "Encapsulation of an SODB within an RBSP"
*/
static int remove_nal_escapes(unsigned char *dst, const unsigned char *src, int h264_data_bytes)
{
    int i = 0, j = 0, zero_cnt = 0;
    for (j = 0; j < h264_data_bytes; j++)
    {
        if (zero_cnt == 2 && src[j] <= 3)
        {
            if (src[j] == 3)
            {
                if (j == h264_data_bytes - 1)
                {
                    // cabac_zero_word: no action
                } else if (src[j + 1] <= 3)
                {
                    j++;
                    zero_cnt = 0;
                } else
                {
                    // TODO: assume end-of-nal
                    //return 0;
                }
            } else
                return 0;
        }
        dst[i++] = src[j];
        if (src[j])
            zero_cnt = 0;
        else
            zero_cnt++;
    }
    //while (--j > i) src[j] = 0;
    return i;
}

/**
*   Put NAL escape codes to the output bitstream
*/
static int nal_put_esc(uint8_t *d, const uint8_t *s, int n)
{
    int i, j = 4, cntz = 0;
    d[0] = d[1] = d[2] = 0; d[3] = 1; // start code
    for (i = 0; i < n; i++)
    {
        uint8_t byte = *s++;
        if (cntz == 2 && byte <= 3)
        {
            d[j++] = 3;
            cntz = 0;
        }
        if (byte)
            cntz = 0;
        else
            cntz++;
        d[j++] = byte;
    }
    return j;
}

static void copy_bits(bit_reader_t *bs, bs_t *bd)
{
    unsigned cb, bits;
    int bit_count = remaining_bits(bs);
    while (bit_count > 7)
    {
        cb = MINIMP4_MIN(bit_count - 7, 8);
        bits = GetBits(cb);
        h264e_bs_put_bits(bd, cb, bits);
        bit_count -= cb;
    }

    // cut extra zeros after stop-bit
    bits = GetBits(bit_count);
    for (; bit_count && ~bits & 1; bit_count--)
    {
        bits >>= 1;
    }
    if (bit_count)
    {
        h264e_bs_put_bits(bd, bit_count, bits);
    }
}

static int change_sps_id(bit_reader_t *bs, bs_t *bd, int new_id, int *old_id)
{
    unsigned bits, sps_id, i, bytes;
    for (i = 0; i < 3; i++)
    {
        bits = GetBits(8);
        h264e_bs_put_bits(bd, 8, bits);
    }
    sps_id = ue_bits(bs);               // max = 31

    *old_id = sps_id;
    sps_id = new_id;
    assert(sps_id <= 31);

    h264e_bs_put_golomb(bd, sps_id);
    copy_bits(bs, bd);

    bytes = h264e_bs_byte_align(bd) / 8;
    h264e_bs_flush(bd);
    return bytes;
}

static int patch_pps(h264_sps_id_patcher_t *h, bit_reader_t *bs, bs_t *bd, int new_pps_id, int *old_id)
{
    int bytes;
    unsigned pps_id = ue_bits(bs);  // max = 255
    unsigned sps_id = ue_bits(bs);  // max = 31

    *old_id = pps_id;
    sps_id = h->map_sps[sps_id];
    pps_id = new_pps_id;

    assert(sps_id <= 31);
    assert(pps_id <= 255);

    h264e_bs_put_golomb(bd, pps_id);
    h264e_bs_put_golomb(bd, sps_id);
    copy_bits(bs, bd);

    bytes = h264e_bs_byte_align(bd) / 8;
    h264e_bs_flush(bd);
    return bytes;
}

static void patch_slice_header(h264_sps_id_patcher_t *h, bit_reader_t *bs, bs_t *bd)
{
    unsigned first_mb_in_slice = ue_bits(bs);
    unsigned slice_type = ue_bits(bs);
    unsigned pps_id = ue_bits(bs);

    pps_id = h->map_pps[pps_id];

    assert(pps_id <= 255);

    h264e_bs_put_golomb(bd, first_mb_in_slice);
    h264e_bs_put_golomb(bd, slice_type);
    h264e_bs_put_golomb(bd, pps_id);
    copy_bits(bs, bd);
}

static int transcode_nalu(h264_sps_id_patcher_t *h, const unsigned char *src, int nalu_bytes, unsigned char *dst)
{
    int old_id;

    bit_reader_t bst[1];
    bs_t bdt[1];

    bit_reader_t bs[1];
    bs_t bd[1];
    int payload_type = src[0] & 31;

    *dst = *src;
    h264e_bs_init_bits(bd, dst + 1);
    init_bits(bs, src + 1, nalu_bytes - 1);
    h264e_bs_init_bits(bdt, dst + 1);
    init_bits(bst, src + 1, nalu_bytes - 1);

    switch(payload_type)
    {
    case 7:
        {
            int cb = change_sps_id(bst, bdt, 0, &old_id);
            int id = find_mem_cache(h->sps_cache, h->sps_bytes, MINIMP4_MAX_SPS, dst + 1, cb);
            if (id == -1)
                return 0;
            h->map_sps[old_id] = id;
            change_sps_id(bs, bd, id, &old_id);
        }
        break;
    case 8:
        {
            int cb = patch_pps(h, bst, bdt, 0, &old_id);
            int id = find_mem_cache(h->pps_cache, h->pps_bytes, MINIMP4_MAX_PPS, dst + 1, cb);
            if (id == -1)
                return 0;
            h->map_pps[old_id] = id;
            patch_pps(h, bs, bd, id, &old_id);
        }
        break;
    case 1:
    case 2:
    case 5:
        patch_slice_header(h, bs, bd);
        break;
    default:
        memcpy(dst, src, nalu_bytes);
        return nalu_bytes;
    }

    nalu_bytes = 1 + h264e_bs_byte_align(bd) / 8;
    h264e_bs_flush(bd);

    return nalu_bytes;
}

#endif

/**
*   Set pointer just after start code (00 .. 00 01), or to EOF if not found:
*
*   NZ NZ ... NZ 00 00 00 00 01 xx xx ... xx (EOF)
*                               ^            ^
*   non-zero head.............. here ....... or here if no start code found
*
*/
static const uint8_t *find_start_code(const uint8_t *h264_data, int h264_data_bytes, int *zcount)
{
    const uint8_t *eof = h264_data + h264_data_bytes;
    const uint8_t *p = h264_data;
    do
    {
        int zero_cnt = 1;
        while (p < eof && *p) p++;
        while (p + zero_cnt < eof && !p[zero_cnt]) zero_cnt++;
        if (zero_cnt >= 2 && p[zero_cnt] == 1)
        {
            *zcount = zero_cnt + 1;
            return p + zero_cnt + 1;
        }
        p += zero_cnt;
    } while (p < eof);
    *zcount = 0;
    return eof;
}


/**
*   Locate NAL unit in given buffer, and calculate it's length
*/
static const uint8_t *find_nal_unit(const uint8_t *h264_data, int h264_data_bytes, int *pnal_unit_bytes)
{
    const uint8_t *eof = h264_data + h264_data_bytes;
    int zcount;
    const uint8_t *start = find_start_code(h264_data, h264_data_bytes, &zcount);
    const uint8_t *stop = start;
    if (start)
    {
        stop = find_start_code(start, (int)(eof - start), &zcount);
        while (stop > start && !stop[-1])
        {
            stop--;
        }
    }

    *pnal_unit_bytes = (int)(stop - start - zcount);
    return start;
}


int mp4_h26x_write_init(mp4_h26x_writer_t *h, MP4E_mux_t *mux, int width, int height, int is_hevc)
{
    MP4E_track_t tr;
    tr.track_media_kind = e_video;
    tr.language[0] = 'u';
    tr.language[1] = 'n';
    tr.language[2] = 'd';
    tr.language[3] = 0;
    tr.object_type_indication = is_hevc ? MP4_OBJECT_TYPE_HEVC : MP4_OBJECT_TYPE_AVC;
    tr.time_scale = 90000;
    tr.default_duration = 0;
    tr.u.v.width = width;
    tr.u.v.height = height;
    h->mux_track_id = MP4E_add_track(mux, &tr);
    h->mux = mux;

    h->is_hevc  = is_hevc;
    h->need_vps = is_hevc;
    h->need_sps = 1;
    h->need_pps = 1;
    h->need_idr = 1;
#if MINIMP4_TRANSCODE_SPS_ID
    memset(&h->sps_patcher, 0, sizeof(h264_sps_id_patcher_t));
#endif
    return MP4E_STATUS_OK;
}

void mp4_h26x_write_close(mp4_h26x_writer_t *h)
{
#if MINIMP4_TRANSCODE_SPS_ID
    h264_sps_id_patcher_t *p = &h->sps_patcher;
    int i;
    for (i = 0; i < MINIMP4_MAX_SPS; i++)
    {
        if (p->sps_cache[i])
            free(p->sps_cache[i]);
    }
    for (i = 0; i < MINIMP4_MAX_PPS; i++)
    {
        if (p->pps_cache[i])
            free(p->pps_cache[i]);
    }
#endif
    memset(h, 0, sizeof(*h));
}

static int mp4_h265_write_nal(mp4_h26x_writer_t *h, const unsigned char *nal, int sizeof_nal, unsigned timeStamp90kHz_next)
{
    int payload_type = (nal[0] >> 1) & 0x3f;
    int is_intra = payload_type >= HEVC_NAL_BLA_W_LP && payload_type <= HEVC_NAL_CRA_NUT;
    int err = MP4E_STATUS_OK;
    //printf("payload_type=%d, intra=%d\n", payload_type, is_intra);

    if (is_intra && !h->need_sps && !h->need_pps && !h->need_vps)
        h->need_idr = 0;
    switch (payload_type)
    {
    case HEVC_NAL_VPS:
        MP4E_set_vps(h->mux, h->mux_track_id, nal, sizeof_nal);
        h->need_vps = 0;
        break;
    case HEVC_NAL_SPS:
        MP4E_set_sps(h->mux, h->mux_track_id, nal, sizeof_nal);
        h->need_sps = 0;
        break;
    case HEVC_NAL_PPS:
        MP4E_set_pps(h->mux, h->mux_track_id, nal, sizeof_nal);
        h->need_pps = 0;
        break;
    default:
        if (h->need_vps || h->need_sps || h->need_pps || h->need_idr)
            return MP4E_STATUS_BAD_ARGUMENTS;
        {
            unsigned char *tmp = (unsigned char *)malloc(4 + sizeof_nal);
            if (!tmp)
                return MP4E_STATUS_NO_MEMORY;
            int sample_kind = MP4E_SAMPLE_DEFAULT;
            tmp[0] = (unsigned char)(sizeof_nal >> 24);
            tmp[1] = (unsigned char)(sizeof_nal >> 16);
            tmp[2] = (unsigned char)(sizeof_nal >>  8);
            tmp[3] = (unsigned char)(sizeof_nal);
            memcpy(tmp + 4, nal, sizeof_nal);
            if (is_intra)
                sample_kind = MP4E_SAMPLE_RANDOM_ACCESS;
            err = MP4E_put_sample(h->mux, h->mux_track_id, tmp, 4 + sizeof_nal, timeStamp90kHz_next, sample_kind);
            free(tmp);
        }
        break;
    }
    return err;
}

int mp4_h26x_write_nal(mp4_h26x_writer_t *h, const unsigned char *nal, int length, unsigned timeStamp90kHz_next)
{
    const unsigned char *eof = nal + length;
    int payload_type, sizeof_nal, err = MP4E_STATUS_OK;
    for (;;nal++)
    {
#if MINIMP4_TRANSCODE_SPS_ID
        unsigned char *nal1, *nal2;
#endif
        nal = find_nal_unit(nal, (int)(eof - nal), &sizeof_nal);
        if (!sizeof_nal)
            break;
        if (h->is_hevc)
        {
            ERR(mp4_h265_write_nal(h, nal, sizeof_nal, timeStamp90kHz_next));
            continue;
        }
        payload_type = nal[0] & 31;
#if MINIMP4_TRANSCODE_SPS_ID
        // Transcode SPS, PPS and slice headers, reassigning ID's for SPS and  PPS:
        // - assign unique ID's to different SPS and PPS
        // - assign same ID's to equal (except ID) SPS and PPS
        // - save all different SPS and PPS
        nal1 = (unsigned char *)malloc(sizeof_nal*17/16 + 32);
        if (!nal1)
            return MP4E_STATUS_NO_MEMORY;
        nal2 = (unsigned char *)malloc(sizeof_nal*17/16 + 32);
        if (!nal2)
        {
            free(nal1);
            return MP4E_STATUS_NO_MEMORY;
        }
        sizeof_nal = remove_nal_escapes(nal2, nal, sizeof_nal);
        if (!sizeof_nal)
        {
exit_with_free:
            free(nal1);
            free(nal2);
            return MP4E_STATUS_BAD_ARGUMENTS;
        }

        sizeof_nal = transcode_nalu(&h->sps_patcher, nal2, sizeof_nal, nal1);
        sizeof_nal = nal_put_esc(nal2, nal1, sizeof_nal);

        switch (payload_type) {
        case 7:
            MP4E_set_sps(h->mux, h->mux_track_id, nal2 + 4, sizeof_nal - 4);
            h->need_sps = 0;
            break;
        case 8:
            if (h->need_sps)
                goto exit_with_free;
            MP4E_set_pps(h->mux, h->mux_track_id, nal2 + 4, sizeof_nal - 4);
            h->need_pps = 0;
            break;
        case 5:
            if (h->need_sps)
                goto exit_with_free;
            h->need_idr = 0;
            // flow through
        default:
            if (h->need_sps)
                goto exit_with_free;
            if (!h->need_pps && !h->need_idr)
            {
                bit_reader_t bs[1];
                init_bits(bs, nal + 1, sizeof_nal - 4 - 1);
                unsigned first_mb_in_slice = ue_bits(bs);
                //unsigned slice_type = ue_bits(bs);
                int sample_kind = MP4E_SAMPLE_DEFAULT;
                nal2[0] = (unsigned char)((sizeof_nal - 4) >> 24);
                nal2[1] = (unsigned char)((sizeof_nal - 4) >> 16);
                nal2[2] = (unsigned char)((sizeof_nal - 4) >>  8);
                nal2[3] = (unsigned char)((sizeof_nal - 4));
                if (first_mb_in_slice)
                    sample_kind = MP4E_SAMPLE_CONTINUATION;
                else if (payload_type == 5)
                    sample_kind = MP4E_SAMPLE_RANDOM_ACCESS;
                err = MP4E_put_sample(h->mux, h->mux_track_id, nal2, sizeof_nal, timeStamp90kHz_next, sample_kind);
            }
            break;
        }
        free(nal1);
        free(nal2);
#else
        // No SPS/PPS transcoding
        // This branch assumes that encoder use correct SPS/PPS ID's
        switch (payload_type) {
            case 7:
                MP4E_set_sps(h->mux, h->mux_track_id, nal, sizeof_nal);
                h->need_sps = 0;
                break;
            case 8:
                MP4E_set_pps(h->mux, h->mux_track_id, nal, sizeof_nal);
                h->need_pps = 0;
                break;
            case 5:
                if (h->need_sps)
                    return MP4E_STATUS_BAD_ARGUMENTS;
                h->need_idr = 0;
                // flow through
            default:
                if (h->need_sps)
                    return MP4E_STATUS_BAD_ARGUMENTS;
                if (!h->need_pps && !h->need_idr)
                {
                    bit_reader_t bs[1];
                    unsigned char *tmp = (unsigned char *)malloc(4 + sizeof_nal);
                    if (!tmp)
                        return MP4E_STATUS_NO_MEMORY;
                    init_bits(bs, nal + 1, sizeof_nal - 1);
                    unsigned first_mb_in_slice = ue_bits(bs);
                    int sample_kind = MP4E_SAMPLE_DEFAULT;
                    tmp[0] = (unsigned char)(sizeof_nal >> 24);
                    tmp[1] = (unsigned char)(sizeof_nal >> 16);
                    tmp[2] = (unsigned char)(sizeof_nal >>  8);
                    tmp[3] = (unsigned char)(sizeof_nal);
                    memcpy(tmp + 4, nal, sizeof_nal);
                    if (first_mb_in_slice)
                        sample_kind = MP4E_SAMPLE_CONTINUATION;
                    else if (payload_type == 5)
                        sample_kind = MP4E_SAMPLE_RANDOM_ACCESS;
                    err = MP4E_put_sample(h->mux, h->mux_track_id, tmp, 4 + sizeof_nal, timeStamp90kHz_next, sample_kind);
//										printf("mp4_h26x_write_nal h->mux_track_id %d, duration %d \n", h->mux_track_id, timeStamp90kHz_next);
                    free(tmp);
                }
                break;
        }
#endif
    }
    return err;
}

#endif
