#ifndef __MINIMP4_COMM_H_
#define __MINIMP4_COMM_H_

/*
    https://github.com/aspt/mp4
    https://github.com/lieff/minimp4
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

#define MINIMP4_MIN(x, y) ((x) < (y) ? (x) : (y))

/************************************************************************/
/*                  Build configuration                                 */
/************************************************************************/

#define FIX_BAD_ANDROID_META_BOX  1

// Max chunks nesting level
#define MAX_CHUNKS_DEPTH          64

#define MINIMP4_MAX_SPS 32
#define MINIMP4_MAX_PPS 256

#define MINIMP4_TRANSCODE_SPS_ID     0

// Support indexing of MP4 files over 4 GB.
// If disabled, files with 64-bit offset fields is still supported,
// but error signaled if such field contains too big offset
// This switch affect return type of MP4D__frame_offset() function
#define MINIMP4_ALLOW_64BIT          1


/************************************************************************/
/*          Some values of MP4(E/D)_track_t->object_type_indication     */
/************************************************************************/
// MPEG-4 AAC (all profiles)
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3                  0x40
// MPEG-2 AAC, Main profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE     0x66
// MPEG-2 AAC, LC profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE       0x67
// MPEG-2 AAC, SSR profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE      0x68
// MP3 Audio
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_11172_3									 0x6B
// H.264 (AVC) video
#define MP4_OBJECT_TYPE_AVC                                    0x21
// H.265 (HEVC) video
#define MP4_OBJECT_TYPE_HEVC                                   0x23
// http://www.mp4ra.org/object.html 0xC0-E0  && 0xE2 - 0xFE are specified as "user private"
#define MP4_OBJECT_TYPE_USER_PRIVATE                           0xC0

/************************************************************************/
/*          API error codes                                             */
/************************************************************************/
#define MP4E_STATUS_OK                       0
#define MP4E_STATUS_BAD_ARGUMENTS           -1
#define MP4E_STATUS_NO_MEMORY               -2
#define MP4E_STATUS_FILE_WRITE_ERROR        -3
#define MP4E_STATUS_ONLY_ONE_DSI_ALLOWED    -4

/************************************************************************/
/*          Sample kind for MP4E__put_sample()                          */
/************************************************************************/
#define MP4E_SAMPLE_DEFAULT             0   // (beginning of) audio or video frame
#define MP4E_SAMPLE_RANDOM_ACCESS       1   // mark sample as random access point (key frame)
#define MP4E_SAMPLE_CONTINUATION        2   // Not a sample, but continuation of previous sample (new slice)

/************************************************************************/
/*                  Portable 64-bit type definition                     */
/************************************************************************/

#if MINIMP4_ALLOW_64BIT
    typedef uint64_t boxsize_t;
#else
    typedef unsigned int boxsize_t;
#endif
typedef boxsize_t MP4D_file_offset_t;

typedef struct
{
    void *sps_cache[MINIMP4_MAX_SPS];
    void *pps_cache[MINIMP4_MAX_PPS];
    int sps_bytes[MINIMP4_MAX_SPS];
    int pps_bytes[MINIMP4_MAX_PPS];

    int map_sps[MINIMP4_MAX_SPS];
    int map_pps[MINIMP4_MAX_PPS];

} h264_sps_id_patcher_t;


#ifdef __cplusplus
}
#endif //__cplusplus

#define FOUR_CHAR_INT(a, b, c, d) (((uint32_t)(a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#if 0
enum
{
    BOX_co64    = FOUR_CHAR_INT( 'c', 'o', '6', '4' ),//ChunkLargeOffsetAtomType
    BOX_stco    = FOUR_CHAR_INT( 's', 't', 'c', 'o' ),//ChunkOffsetAtomType
    BOX_crhd    = FOUR_CHAR_INT( 'c', 'r', 'h', 'd' ),//ClockReferenceMediaHeaderAtomType
    BOX_ctts    = FOUR_CHAR_INT( 'c', 't', 't', 's' ),//CompositionOffsetAtomType
    BOX_cprt    = FOUR_CHAR_INT( 'c', 'p', 'r', 't' ),//CopyrightAtomType
    BOX_url_    = FOUR_CHAR_INT( 'u', 'r', 'l', ' ' ),//DataEntryURLAtomType
    BOX_urn_    = FOUR_CHAR_INT( 'u', 'r', 'n', ' ' ),//DataEntryURNAtomType
    BOX_dinf    = FOUR_CHAR_INT( 'd', 'i', 'n', 'f' ),//DataInformationAtomType
    BOX_dref    = FOUR_CHAR_INT( 'd', 'r', 'e', 'f' ),//DataReferenceAtomType
    BOX_stdp    = FOUR_CHAR_INT( 's', 't', 'd', 'p' ),//DegradationPriorityAtomType
    BOX_edts    = FOUR_CHAR_INT( 'e', 'd', 't', 's' ),//EditAtomType
    BOX_elst    = FOUR_CHAR_INT( 'e', 'l', 's', 't' ),//EditListAtomType
    BOX_uuid    = FOUR_CHAR_INT( 'u', 'u', 'i', 'd' ),//ExtendedAtomType
    BOX_free    = FOUR_CHAR_INT( 'f', 'r', 'e', 'e' ),//FreeSpaceAtomType
    BOX_hdlr    = FOUR_CHAR_INT( 'h', 'd', 'l', 'r' ),//HandlerAtomType
    BOX_hmhd    = FOUR_CHAR_INT( 'h', 'm', 'h', 'd' ),//HintMediaHeaderAtomType
    BOX_hint    = FOUR_CHAR_INT( 'h', 'i', 'n', 't' ),//HintTrackReferenceAtomType
    BOX_mdia    = FOUR_CHAR_INT( 'm', 'd', 'i', 'a' ),//MediaAtomType
    BOX_mdat    = FOUR_CHAR_INT( 'm', 'd', 'a', 't' ),//MediaDataAtomType
    BOX_mdhd    = FOUR_CHAR_INT( 'm', 'd', 'h', 'd' ),//MediaHeaderAtomType
    BOX_minf    = FOUR_CHAR_INT( 'm', 'i', 'n', 'f' ),//MediaInformationAtomType
    BOX_moov    = FOUR_CHAR_INT( 'm', 'o', 'o', 'v' ),//MovieAtomType
    BOX_mvhd    = FOUR_CHAR_INT( 'm', 'v', 'h', 'd' ),//MovieHeaderAtomType
    BOX_stsd    = FOUR_CHAR_INT( 's', 't', 's', 'd' ),//SampleDescriptionAtomType
    BOX_stsz    = FOUR_CHAR_INT( 's', 't', 's', 'z' ),//SampleSizeAtomType
    BOX_stz2    = FOUR_CHAR_INT( 's', 't', 'z', '2' ),//CompactSampleSizeAtomType
    BOX_stbl    = FOUR_CHAR_INT( 's', 't', 'b', 'l' ),//SampleTableAtomType
    BOX_stsc    = FOUR_CHAR_INT( 's', 't', 's', 'c' ),//SampleToChunkAtomType
    BOX_stsh    = FOUR_CHAR_INT( 's', 't', 's', 'h' ),//ShadowSyncAtomType
    BOX_skip    = FOUR_CHAR_INT( 's', 'k', 'i', 'p' ),//SkipAtomType
    BOX_smhd    = FOUR_CHAR_INT( 's', 'm', 'h', 'd' ),//SoundMediaHeaderAtomType
    BOX_stss    = FOUR_CHAR_INT( 's', 't', 's', 's' ),//SyncSampleAtomType
    BOX_stts    = FOUR_CHAR_INT( 's', 't', 't', 's' ),//TimeToSampleAtomType
    BOX_trak    = FOUR_CHAR_INT( 't', 'r', 'a', 'k' ),//TrackAtomType
    BOX_tkhd    = FOUR_CHAR_INT( 't', 'k', 'h', 'd' ),//TrackHeaderAtomType
    BOX_tref    = FOUR_CHAR_INT( 't', 'r', 'e', 'f' ),//TrackReferenceAtomType
    BOX_udta    = FOUR_CHAR_INT( 'u', 'd', 't', 'a' ),//UserDataAtomType
    BOX_vmhd    = FOUR_CHAR_INT( 'v', 'm', 'h', 'd' ),//VideoMediaHeaderAtomType
    BOX_url     = FOUR_CHAR_INT( 'u', 'r', 'l', ' ' ),
    BOX_urn     = FOUR_CHAR_INT( 'u', 'r', 'n', ' ' ),

    BOX_gnrv    = FOUR_CHAR_INT( 'g', 'n', 'r', 'v' ),//GenericVisualSampleEntryAtomType
    BOX_gnra    = FOUR_CHAR_INT( 'g', 'n', 'r', 'a' ),//GenericAudioSampleEntryAtomType

    //V2 atoms
    BOX_ftyp    = FOUR_CHAR_INT( 'f', 't', 'y', 'p' ),//FileTypeAtomType
    BOX_padb    = FOUR_CHAR_INT( 'p', 'a', 'd', 'b' ),//PaddingBitsAtomType

    //MP4 Atoms
    BOX_sdhd    = FOUR_CHAR_INT( 's', 'd', 'h', 'd' ),//SceneDescriptionMediaHeaderAtomType
    BOX_dpnd    = FOUR_CHAR_INT( 'd', 'p', 'n', 'd' ),//StreamDependenceAtomType
    BOX_iods    = FOUR_CHAR_INT( 'i', 'o', 'd', 's' ),//ObjectDescriptorAtomType
    BOX_odhd    = FOUR_CHAR_INT( 'o', 'd', 'h', 'd' ),//ObjectDescriptorMediaHeaderAtomType
    BOX_mpod    = FOUR_CHAR_INT( 'm', 'p', 'o', 'd' ),//ODTrackReferenceAtomType
    BOX_nmhd    = FOUR_CHAR_INT( 'n', 'm', 'h', 'd' ),//MPEGMediaHeaderAtomType
    BOX_esds    = FOUR_CHAR_INT( 'e', 's', 'd', 's' ),//ESDAtomType
    BOX_sync    = FOUR_CHAR_INT( 's', 'y', 'n', 'c' ),//OCRReferenceAtomType
    BOX_ipir    = FOUR_CHAR_INT( 'i', 'p', 'i', 'r' ),//IPIReferenceAtomType
    BOX_mp4s    = FOUR_CHAR_INT( 'm', 'p', '4', 's' ),//MPEGSampleEntryAtomType
    BOX_mp4a    = FOUR_CHAR_INT( 'm', 'p', '4', 'a' ),//MPEGAudioSampleEntryAtomType
    BOX_mp4v    = FOUR_CHAR_INT( 'm', 'p', '4', 'v' ),//MPEGVisualSampleEntryAtomType

    // http://www.itscj.ipsj.or.jp/sc29/open/29view/29n7644t.doc
    BOX_avc1    = FOUR_CHAR_INT( 'a', 'v', 'c', '1' ),
    BOX_avc2    = FOUR_CHAR_INT( 'a', 'v', 'c', '2' ),
    BOX_svc1    = FOUR_CHAR_INT( 's', 'v', 'c', '1' ),
    BOX_avcC    = FOUR_CHAR_INT( 'a', 'v', 'c', 'C' ),
    BOX_svcC    = FOUR_CHAR_INT( 's', 'v', 'c', 'C' ),
    BOX_btrt    = FOUR_CHAR_INT( 'b', 't', 'r', 't' ),
    BOX_m4ds    = FOUR_CHAR_INT( 'm', '4', 'd', 's' ),
    BOX_seib    = FOUR_CHAR_INT( 's', 'e', 'i', 'b' ),

    // H264/HEVC
    BOX_hev1    = FOUR_CHAR_INT( 'h', 'e', 'v', '1' ),
    BOX_hvc1    = FOUR_CHAR_INT( 'h', 'v', 'c', '1' ),
    BOX_hvcC    = FOUR_CHAR_INT( 'h', 'v', 'c', 'C' ),

    //3GPP atoms
    BOX_samr    = FOUR_CHAR_INT( 's', 'a', 'm', 'r' ),//AMRSampleEntryAtomType
    BOX_sawb    = FOUR_CHAR_INT( 's', 'a', 'w', 'b' ),//WB_AMRSampleEntryAtomType
    BOX_damr    = FOUR_CHAR_INT( 'd', 'a', 'm', 'r' ),//AMRConfigAtomType
    BOX_s263    = FOUR_CHAR_INT( 's', '2', '6', '3' ),//H263SampleEntryAtomType
    BOX_d263    = FOUR_CHAR_INT( 'd', '2', '6', '3' ),//H263ConfigAtomType

    //V2 atoms - Movie Fragments
    BOX_mvex    = FOUR_CHAR_INT( 'm', 'v', 'e', 'x' ),//MovieExtendsAtomType
    BOX_trex    = FOUR_CHAR_INT( 't', 'r', 'e', 'x' ),//TrackExtendsAtomType
    BOX_moof    = FOUR_CHAR_INT( 'm', 'o', 'o', 'f' ),//MovieFragmentAtomType
    BOX_mfhd    = FOUR_CHAR_INT( 'm', 'f', 'h', 'd' ),//MovieFragmentHeaderAtomType
    BOX_traf    = FOUR_CHAR_INT( 't', 'r', 'a', 'f' ),//TrackFragmentAtomType
    BOX_tfhd    = FOUR_CHAR_INT( 't', 'f', 'h', 'd' ),//TrackFragmentHeaderAtomType
    BOX_trun    = FOUR_CHAR_INT( 't', 'r', 'u', 'n' ),//TrackFragmentRunAtomType
    BOX_mehd    = FOUR_CHAR_INT( 'm', 'e', 'h', 'd' ),//MovieExtendsHeaderBox
		
    // Object Descriptors (OD) data coding
    // These takes only 1 byte; this implementation translate <od_tag> to
    // <od_tag> + OD_BASE to keep API uniform and safe for string functions
    OD_BASE    = FOUR_CHAR_INT( '$', '$', '$', '0' ),//
    OD_ESD     = FOUR_CHAR_INT( '$', '$', '$', '3' ),//SDescriptor_Tag
    OD_DCD     = FOUR_CHAR_INT( '$', '$', '$', '4' ),//DecoderConfigDescriptor_Tag
    OD_DSI     = FOUR_CHAR_INT( '$', '$', '$', '5' ),//DecoderSpecificInfo_Tag
    OD_SLC     = FOUR_CHAR_INT( '$', '$', '$', '6' ),//SLConfigDescriptor_Tag

    BOX_meta   = FOUR_CHAR_INT( 'm', 'e', 't', 'a' ),
    BOX_ilst   = FOUR_CHAR_INT( 'i', 'l', 's', 't' ),

    // Metagata tags, see http://atomicparsley.sourceforge.net/mpeg-4files.html
    BOX_calb    = FOUR_CHAR_INT( '\xa9', 'a', 'l', 'b'),    // album
    BOX_cart    = FOUR_CHAR_INT( '\xa9', 'a', 'r', 't'),    // artist
    BOX_aART    = FOUR_CHAR_INT( 'a', 'A', 'R', 'T' ),      // album artist
    BOX_ccmt    = FOUR_CHAR_INT( '\xa9', 'c', 'm', 't'),    // comment
    BOX_cday    = FOUR_CHAR_INT( '\xa9', 'd', 'a', 'y'),    // year (as string)
    BOX_cnam    = FOUR_CHAR_INT( '\xa9', 'n', 'a', 'm'),    // title
    BOX_cgen    = FOUR_CHAR_INT( '\xa9', 'g', 'e', 'n'),    // custom genre (as string or as byte!)
    BOX_trkn    = FOUR_CHAR_INT( 't', 'r', 'k', 'n'),       // track number (byte)
    BOX_disk    = FOUR_CHAR_INT( 'd', 'i', 's', 'k'),       // disk number (byte)
    BOX_cwrt    = FOUR_CHAR_INT( '\xa9', 'w', 'r', 't'),    // composer
    BOX_ctoo    = FOUR_CHAR_INT( '\xa9', 't', 'o', 'o'),    // encoder
    BOX_tmpo    = FOUR_CHAR_INT( 't', 'm', 'p', 'o'),       // bpm (byte)
    BOX_cpil    = FOUR_CHAR_INT( 'c', 'p', 'i', 'l'),       // compilation (byte)
    BOX_covr    = FOUR_CHAR_INT( 'c', 'o', 'v', 'r'),       // cover art (JPEG/PNG)
    BOX_rtng    = FOUR_CHAR_INT( 'r', 't', 'n', 'g'),       // rating/advisory (byte)
    BOX_cgrp    = FOUR_CHAR_INT( '\xa9', 'g', 'r', 'p'),    // grouping
    BOX_stik    = FOUR_CHAR_INT( 's', 't', 'i', 'k'),       // stik (byte)  0 = Movie   1 = Normal  2 = Audiobook  5 = Whacked Bookmark  6 = Music Video  9 = Short Film  10 = TV Show  11 = Booklet  14 = Ringtone
    BOX_pcst    = FOUR_CHAR_INT( 'p', 'c', 's', 't'),       // podcast (byte)
    BOX_catg    = FOUR_CHAR_INT( 'c', 'a', 't', 'g'),       // category
    BOX_keyw    = FOUR_CHAR_INT( 'k', 'e', 'y', 'w'),       // keyword
    BOX_purl    = FOUR_CHAR_INT( 'p', 'u', 'r', 'l'),       // podcast URL (byte)
    BOX_egid    = FOUR_CHAR_INT( 'e', 'g', 'i', 'd'),       // episode global unique ID (byte)
    BOX_desc    = FOUR_CHAR_INT( 'd', 'e', 's', 'c'),       // description
    BOX_clyr    = FOUR_CHAR_INT( '\xa9', 'l', 'y', 'r'),    // lyrics (may be > 255 bytes)
    BOX_tven    = FOUR_CHAR_INT( 't', 'v', 'e', 'n'),       // tv episode number
    BOX_tves    = FOUR_CHAR_INT( 't', 'v', 'e', 's'),       // tv episode (byte)
    BOX_tvnn    = FOUR_CHAR_INT( 't', 'v', 'n', 'n'),       // tv network name
    BOX_tvsh    = FOUR_CHAR_INT( 't', 'v', 's', 'h'),       // tv show name
    BOX_tvsn    = FOUR_CHAR_INT( 't', 'v', 's', 'n'),       // tv season (byte)
    BOX_purd    = FOUR_CHAR_INT( 'p', 'u', 'r', 'd'),       // purchase date
    BOX_pgap    = FOUR_CHAR_INT( 'p', 'g', 'a', 'p'),       // Gapless Playback (byte)

    //BOX_aart   = FOUR_CHAR_INT( 'a', 'a', 'r', 't' ),     // Album artist
    BOX_cART    = FOUR_CHAR_INT( '\xa9', 'A', 'R', 'T'),    // artist
    BOX_gnre    = FOUR_CHAR_INT( 'g', 'n', 'r', 'e'),

    // 3GPP metatags  (http://cpansearch.perl.org/src/JHAR/MP4-Info-1.12/Info.pm)
    BOX_auth    = FOUR_CHAR_INT( 'a', 'u', 't', 'h'),       // author
    BOX_titl    = FOUR_CHAR_INT( 't', 'i', 't', 'l'),       // title
    BOX_dscp    = FOUR_CHAR_INT( 'd', 's', 'c', 'p'),       // description
    BOX_perf    = FOUR_CHAR_INT( 'p', 'e', 'r', 'f'),       // performer
    BOX_mean    = FOUR_CHAR_INT( 'm', 'e', 'a', 'n'),       //
    BOX_name    = FOUR_CHAR_INT( 'n', 'a', 'm', 'e'),       //
    BOX_data    = FOUR_CHAR_INT( 'd', 'a', 't', 'a'),       //

    // these from http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2008-September/053151.html
    BOX_albm    = FOUR_CHAR_INT( 'a', 'l', 'b', 'm'),      // album
    BOX_yrrc    = FOUR_CHAR_INT( 'y', 'r', 'r', 'c')       // album
};

#else

#define	BOX_co64 FOUR_CHAR_INT( 'c', 'o', '6', '4' ) //ChunkLargeOffsetAtomType
#define BOX_stco FOUR_CHAR_INT( 's', 't', 'c', 'o' )//ChunkOffsetAtomType
#define BOX_crhd FOUR_CHAR_INT( 'c', 'r', 'h', 'd' )//ClockReferenceMediaHeaderAtomType
#define BOX_ctts FOUR_CHAR_INT( 'c', 't', 't', 's' )//CompositionOffsetAtomType
#define BOX_cprt FOUR_CHAR_INT( 'c', 'p', 'r', 't' )//CopyrightAtomType
#define BOX_url_ FOUR_CHAR_INT( 'u', 'r', 'l', ' ' )//DataEntryURLAtomType
#define    BOX_urn_    FOUR_CHAR_INT( 'u', 'r', 'n', ' ' )//DataEntryURNAtomType
#define    BOX_dinf    FOUR_CHAR_INT( 'd', 'i', 'n', 'f' )//DataInformationAtomType
#define    BOX_dref    FOUR_CHAR_INT( 'd', 'r', 'e', 'f' )//DataReferenceAtomType
#define    BOX_stdp    FOUR_CHAR_INT( 's', 't', 'd', 'p' )//DegradationPriorityAtomType
#define    BOX_edts    FOUR_CHAR_INT( 'e', 'd', 't', 's' )//EditAtomType
#define    BOX_elst    FOUR_CHAR_INT( 'e', 'l', 's', 't' )//EditListAtomType
#define    BOX_uuid    FOUR_CHAR_INT( 'u', 'u', 'i', 'd' )//ExtendedAtomType
#define    BOX_free    FOUR_CHAR_INT( 'f', 'r', 'e', 'e' )//FreeSpaceAtomType
#define    BOX_hdlr    FOUR_CHAR_INT( 'h', 'd', 'l', 'r' )//HandlerAtomType
#define    BOX_hmhd    FOUR_CHAR_INT( 'h', 'm', 'h', 'd' )//HintMediaHeaderAtomType
#define    BOX_hint    FOUR_CHAR_INT( 'h', 'i', 'n', 't' )//HintTrackReferenceAtomType
#define    BOX_mdia    FOUR_CHAR_INT( 'm', 'd', 'i', 'a' )//MediaAtomType
#define    BOX_mdat    FOUR_CHAR_INT( 'm', 'd', 'a', 't' )//MediaDataAtomType
#define    BOX_mdhd    FOUR_CHAR_INT( 'm', 'd', 'h', 'd' )//MediaHeaderAtomType
#define    BOX_minf    FOUR_CHAR_INT( 'm', 'i', 'n', 'f' )//MediaInformationAtomType
#define    BOX_moov    FOUR_CHAR_INT( 'm', 'o', 'o', 'v' )//MovieAtomType
#define    BOX_mvhd    FOUR_CHAR_INT( 'm', 'v', 'h', 'd' )//MovieHeaderAtomType
#define    BOX_stsd    FOUR_CHAR_INT( 's', 't', 's', 'd' )//SampleDescriptionAtomType
#define    BOX_stsz    FOUR_CHAR_INT( 's', 't', 's', 'z' )//SampleSizeAtomType
#define    BOX_stz2    FOUR_CHAR_INT( 's', 't', 'z', '2' )//CompactSampleSizeAtomType
#define    BOX_stbl    FOUR_CHAR_INT( 's', 't', 'b', 'l' )//SampleTableAtomType
#define    BOX_stsc    FOUR_CHAR_INT( 's', 't', 's', 'c' )//SampleToChunkAtomType
#define    BOX_stsh    FOUR_CHAR_INT( 's', 't', 's', 'h' )//ShadowSyncAtomType
#define    BOX_skip    FOUR_CHAR_INT( 's', 'k', 'i', 'p' )//SkipAtomType
#define    BOX_smhd    FOUR_CHAR_INT( 's', 'm', 'h', 'd' )//SoundMediaHeaderAtomType
#define    BOX_stss    FOUR_CHAR_INT( 's', 't', 's', 's' )//SyncSampleAtomType
#define    BOX_stts    FOUR_CHAR_INT( 's', 't', 't', 's' )//TimeToSampleAtomType
#define    BOX_trak    FOUR_CHAR_INT( 't', 'r', 'a', 'k' )//TrackAtomType
#define    BOX_tkhd    FOUR_CHAR_INT( 't', 'k', 'h', 'd' )//TrackHeaderAtomType
#define    BOX_tref    FOUR_CHAR_INT( 't', 'r', 'e', 'f' )//TrackReferenceAtomType
#define    BOX_udta    FOUR_CHAR_INT( 'u', 'd', 't', 'a' )//UserDataAtomType
#define    BOX_vmhd    FOUR_CHAR_INT( 'v', 'm', 'h', 'd' )//VideoMediaHeaderAtomType
#define    BOX_url     FOUR_CHAR_INT( 'u', 'r', 'l', ' ' )
#define    BOX_urn     FOUR_CHAR_INT( 'u', 'r', 'n', ' ' )

#define    BOX_gnrv    FOUR_CHAR_INT( 'g', 'n', 'r', 'v' )//GenericVisualSampleEntryAtomType
#define    BOX_gnra    FOUR_CHAR_INT( 'g', 'n', 'r', 'a' )//GenericAudioSampleEntryAtomType

    //V2 atoms
#define    BOX_ftyp    FOUR_CHAR_INT( 'f', 't', 'y', 'p' )//FileTypeAtomType
#define    BOX_padb    FOUR_CHAR_INT( 'p', 'a', 'd', 'b' )//PaddingBitsAtomType

    //MP4 Atoms
#define    BOX_sdhd    FOUR_CHAR_INT( 's', 'd', 'h', 'd' )//SceneDescriptionMediaHeaderAtomType
#define    BOX_dpnd    FOUR_CHAR_INT( 'd', 'p', 'n', 'd' )//StreamDependenceAtomType
#define    BOX_iods    FOUR_CHAR_INT( 'i', 'o', 'd', 's' )//ObjectDescriptorAtomType
#define    BOX_odhd    FOUR_CHAR_INT( 'o', 'd', 'h', 'd' )//ObjectDescriptorMediaHeaderAtomType
#define    BOX_mpod    FOUR_CHAR_INT( 'm', 'p', 'o', 'd' )//ODTrackReferenceAtomType
#define    BOX_nmhd    FOUR_CHAR_INT( 'n', 'm', 'h', 'd' )//MPEGMediaHeaderAtomType
#define    BOX_esds    FOUR_CHAR_INT( 'e', 's', 'd', 's' )//ESDAtomType
#define    BOX_sync    FOUR_CHAR_INT( 's', 'y', 'n', 'c' )//OCRReferenceAtomType
#define    BOX_ipir    FOUR_CHAR_INT( 'i', 'p', 'i', 'r' )//IPIReferenceAtomType
#define    BOX_mp4s    FOUR_CHAR_INT( 'm', 'p', '4', 's' )//MPEGSampleEntryAtomType
#define    BOX_mp4a    FOUR_CHAR_INT( 'm', 'p', '4', 'a' )//MPEGAudioSampleEntryAtomType
#define    BOX_mp4v    FOUR_CHAR_INT( 'm', 'p', '4', 'v' )//MPEGVisualSampleEntryAtomType

    // http://www.itscj.ipsj.or.jp/sc29/open/29view/29n7644t.doc
#define    BOX_avc1    FOUR_CHAR_INT( 'a', 'v', 'c', '1' )
#define    BOX_avc2    FOUR_CHAR_INT( 'a', 'v', 'c', '2' )
#define    BOX_svc1    FOUR_CHAR_INT( 's', 'v', 'c', '1' )
#define    BOX_avcC    FOUR_CHAR_INT( 'a', 'v', 'c', 'C' )
#define    BOX_svcC    FOUR_CHAR_INT( 's', 'v', 'c', 'C' )
#define    BOX_btrt    FOUR_CHAR_INT( 'b', 't', 'r', 't' )
#define    BOX_m4ds    FOUR_CHAR_INT( 'm', '4', 'd', 's' )
#define    BOX_seib    FOUR_CHAR_INT( 's', 'e', 'i', 'b' )

    // H264/HEVC
#define    BOX_hev1    FOUR_CHAR_INT( 'h', 'e', 'v', '1' )
#define    BOX_hvc1    FOUR_CHAR_INT( 'h', 'v', 'c', '1' )
#define    BOX_hvcC    FOUR_CHAR_INT( 'h', 'v', 'c', 'C' )

    //3GPP atoms
#define    BOX_samr    FOUR_CHAR_INT( 's', 'a', 'm', 'r' )//AMRSampleEntryAtomType
#define    BOX_sawb    FOUR_CHAR_INT( 's', 'a', 'w', 'b' )//WB_AMRSampleEntryAtomType
#define    BOX_damr    FOUR_CHAR_INT( 'd', 'a', 'm', 'r' )//AMRConfigAtomType
#define    BOX_s263    FOUR_CHAR_INT( 's', '2', '6', '3' )//H263SampleEntryAtomType
#define    BOX_d263    FOUR_CHAR_INT( 'd', '2', '6', '3' )//H263ConfigAtomType

    //V2 atoms - Movie Fragments
#define    BOX_mvex    FOUR_CHAR_INT( 'm', 'v', 'e', 'x' )//MovieExtendsAtomType
#define    BOX_trex    FOUR_CHAR_INT( 't', 'r', 'e', 'x' )//TrackExtendsAtomType
#define    BOX_moof    FOUR_CHAR_INT( 'm', 'o', 'o', 'f' )//MovieFragmentAtomType
#define    BOX_mfhd    FOUR_CHAR_INT( 'm', 'f', 'h', 'd' )//MovieFragmentHeaderAtomType
#define    BOX_traf    FOUR_CHAR_INT( 't', 'r', 'a', 'f' )//TrackFragmentAtomType
#define    BOX_tfhd    FOUR_CHAR_INT( 't', 'f', 'h', 'd' )//TrackFragmentHeaderAtomType
#define    BOX_trun    FOUR_CHAR_INT( 't', 'r', 'u', 'n' )//TrackFragmentRunAtomType
#define    BOX_mehd   FOUR_CHAR_INT( 'm', 'e', 'h', 'd' )//MovieExtendsHeaderBox

    // Object Descriptors (OD) data coding
    // These takes only 1 byte; this implementation translate <od_tag> to
    // <od_tag> + OD_BASE to keep API uniform and safe for string functions
#define    OD_BASE    FOUR_CHAR_INT( '$', '$', '$', '0' )//
#define    OD_ESD     FOUR_CHAR_INT( '$', '$', '$', '3' )//SDescriptor_Tag
#define    OD_DCD     FOUR_CHAR_INT( '$', '$', '$', '4' )//DecoderConfigDescriptor_Tag
#define    OD_DSI     FOUR_CHAR_INT( '$', '$', '$', '5' )//DecoderSpecificInfo_Tag
#define    OD_SLC     FOUR_CHAR_INT( '$', '$', '$', '6' )//SLConfigDescriptor_Tag

#define    BOX_meta   FOUR_CHAR_INT( 'm', 'e', 't', 'a' )
#define    BOX_ilst   FOUR_CHAR_INT( 'i', 'l', 's', 't' )

    // Metagata tags, see http://atomicparsley.sourceforge.net/mpeg-4files.html
#define    BOX_calb   FOUR_CHAR_INT( '\xa9', 'a', 'l', 'b')    // album
#define    BOX_cart   FOUR_CHAR_INT( '\xa9', 'a', 'r', 't')    // artist
#define    BOX_aART   FOUR_CHAR_INT( 'a', 'A', 'R', 'T' )      // album artist
#define    BOX_ccmt   FOUR_CHAR_INT( '\xa9', 'c', 'm', 't')    // comment
#define    BOX_cday   FOUR_CHAR_INT( '\xa9', 'd', 'a', 'y')    // year (as string)
#define    BOX_cnam   FOUR_CHAR_INT( '\xa9', 'n', 'a', 'm')    // title
#define    BOX_cgen   FOUR_CHAR_INT( '\xa9', 'g', 'e', 'n')    // custom genre (as string or as byte!)
#define    BOX_trkn   FOUR_CHAR_INT( 't', 'r', 'k', 'n')       // track number (byte)
#define    BOX_disk   FOUR_CHAR_INT( 'd', 'i', 's', 'k')       // disk number (byte)
#define    BOX_cwrt   FOUR_CHAR_INT( '\xa9', 'w', 'r', 't')    // composer
#define    BOX_ctoo   FOUR_CHAR_INT( '\xa9', 't', 'o', 'o')    // encoder
#define    BOX_tmpo   FOUR_CHAR_INT( 't', 'm', 'p', 'o')       // bpm (byte)
#define    BOX_cpil   FOUR_CHAR_INT( 'c', 'p', 'i', 'l')       // compilation (byte)
#define    BOX_covr   FOUR_CHAR_INT( 'c', 'o', 'v', 'r')       // cover art (JPEG/PNG)
#define    BOX_rtng   FOUR_CHAR_INT( 'r', 't', 'n', 'g')       // rating/advisory (byte)
#define    BOX_cgrp   FOUR_CHAR_INT( '\xa9', 'g', 'r', 'p')    // grouping
#define    BOX_stik   FOUR_CHAR_INT( 's', 't', 'i', 'k')       // stik (byte)  0 = Movie   1 = Normal  2 = Audiobook  5 = Whacked Bookmark  6 = Music Video  9 = Short Film  10 = TV Show  11 = Booklet  14 = Ringtone
#define    BOX_pcst   FOUR_CHAR_INT( 'p', 'c', 's', 't')       // podcast (byte)
#define    BOX_catg   FOUR_CHAR_INT( 'c', 'a', 't', 'g')       // category
#define    BOX_keyw   FOUR_CHAR_INT( 'k', 'e', 'y', 'w')       // keyword
#define    BOX_purl   FOUR_CHAR_INT( 'p', 'u', 'r', 'l')       // podcast URL (byte)
#define    BOX_egid   FOUR_CHAR_INT( 'e', 'g', 'i', 'd')       // episode global unique ID (byte)
#define    BOX_desc   FOUR_CHAR_INT( 'd', 'e', 's', 'c')       // description
#define    BOX_clyr   FOUR_CHAR_INT( '\xa9', 'l', 'y', 'r')    // lyrics (may be > 255 bytes)
#define    BOX_tven   FOUR_CHAR_INT( 't', 'v', 'e', 'n')       // tv episode number
#define    BOX_tves   FOUR_CHAR_INT( 't', 'v', 'e', 's')       // tv episode (byte)
#define    BOX_tvnn   FOUR_CHAR_INT( 't', 'v', 'n', 'n')       // tv network name
#define    BOX_tvsh   FOUR_CHAR_INT( 't', 'v', 's', 'h')       // tv show name
#define    BOX_tvsn   FOUR_CHAR_INT( 't', 'v', 's', 'n')       // tv season (byte)
#define    BOX_purd   FOUR_CHAR_INT( 'p', 'u', 'r', 'd')       // purchase date
#define    BOX_pgap   FOUR_CHAR_INT( 'p', 'g', 'a', 'p')       // Gapless Playback (byte)

    //BOX_aart   = FOUR_CHAR_INT( 'a', 'a', 'r', 't' ),     // Album artist
#define    BOX_cART   FOUR_CHAR_INT( '\xa9', 'A', 'R', 'T')    // artist
#define    BOX_gnre   FOUR_CHAR_INT( 'g', 'n', 'r', 'e')

    // 3GPP metatags  (http://cpansearch.perl.org/src/JHAR/MP4-Info-1.12/Info.pm)
#define    BOX_auth   FOUR_CHAR_INT( 'a', 'u', 't', 'h')      // author
#define    BOX_titl   FOUR_CHAR_INT( 't', 'i', 't', 'l')       // title
#define    BOX_dscp   FOUR_CHAR_INT( 'd', 's', 'c', 'p')       // description
#define    BOX_perf   FOUR_CHAR_INT( 'p', 'e', 'r', 'f')       // performer
#define    BOX_mean   FOUR_CHAR_INT( 'm', 'e', 'a', 'n')       //
#define    BOX_name   FOUR_CHAR_INT( 'n', 'a', 'm', 'e')      //
#define    BOX_data   FOUR_CHAR_INT( 'd', 'a', 't', 'a')       //

    // these from http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2008-September/053151.html
#define    BOX_albm   FOUR_CHAR_INT( 'a', 'l', 'b', 'm')      // album
#define    BOX_yrrc   FOUR_CHAR_INT( 'y', 'r', 'r', 'c')       // album


#endif

#define HEVC_NAL_VPS 32
#define HEVC_NAL_SPS 33
#define HEVC_NAL_PPS 34
#define HEVC_NAL_BLA_W_LP 16
#define HEVC_NAL_CRA_NUT  21

#endif
