#ifndef __FLV_H__
#define __FLV_H__


#include <stdlib.h>
#include <stdio.h>

#include "std_log.h"
#include "util.h"
#include "amf.h"




/* FLV file header */
#define FLV_SIGNATURE       "FLV"
#define FLV_VERSION         ((u_byte)0x01)

#define FLV_FLAG_VIDEO      ((u_byte)0x01)
#define FLV_FLAG_AUDIO      ((u_byte)0x04)

typedef struct flv_header_s {
    // u_char      signature       // must be "FLV"
    u_byte      version;        // should be 1
    u_byte      flags;          // 倒数第一位是1表示有视频，倒数第三位是1表示有音频，倒数第二、四位必须为0
    u_int       offset;         // whole header length，always 9
} flv_header_t;

#define FLV_HEADER_SIZE 9u

#define flv_header_has_video(header)    ((header)->flags & FLV_FLAG_VIDEO)
#define flv_header_has_audio(header)    ((header)->flags & FLV_FLAG_AUDIO)
#define flv_header_get_offset(header)    ((u_int) (header)->offset)




/* FLV tag */
#define FLV_TAG_HEADER_TYPE_AUDIO  ((u_int)0x08)
#define FLV_TAG_HEADER_TYPE_VIDEO  ((u_int)0x09)
#define FLV_TAG_HEADER_TYPE_META   ((u_int)0x12)

#define FLV_TAG_SIZE_BODY_LENGTH    3
#define FLV_TAG_SIZE_STREAM_ID      3 
#define FLV_TAG_SIZE_TIMESTAMP      3

typedef struct flv_tag_header_s {
    u_byte      tag_type        ;   // one of: 8：audio，9：video，18：metadata
    u_int       body_length     ;   // body length, in bytes, total tag size minus 11
    u_int       timestamp       ;   // 整数，单位是毫秒。对于 mate 型的 tag 总是0
    u_byte      timestamp_ex    ;   // 将时间戳扩展为4bytes，代表高8位。279分钟以上用
    u_int       stream_ID       ;   // must be "\0\0\0"
    // 紧接着是数据实体（flv_tag_data_t）
} flv_tag_header_t;

#define FLV_TAG_SIZE 11u

#define flv_tag_get_body_length(tag)    ((u_int) (tag)->body_length)
#define flv_tag_get_stream_ID(tag)      ((u_int) (tag)->stream_ID)
#define flv_tag_get_timestamp(tag) \
    ((u_int) (tag)->timestamp) + ((tag)->timestamp_ex << 24))

#define format_tag_header(tag)    \
    printf("{ tag_type:%d, body_length:%d, stream_ID:%d, timestamp:%d, timestamp_ex:%d", tag->tag_type, tag->body_length, tag->stream_ID, tag->timestamp, tag->timestamp_ex)




/* audio tag */
#define FLV_AUDIO_TAG_SOUND_TYPE_MONO    0
#define FLV_AUDIO_TAG_SOUND_TYPE_STEREO  1

#define FLV_AUDIO_TAG_SOUND_SIZE_8       0
#define FLV_AUDIO_TAG_SOUND_SIZE_16      1

#define FLV_AUDIO_TAG_SOUND_RATE_5_5     0
#define FLV_AUDIO_TAG_SOUND_RATE_11      1
#define FLV_AUDIO_TAG_SOUND_RATE_22      2
#define FLV_AUDIO_TAG_SOUND_RATE_44      3

#define FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM          0
#define FLV_AUDIO_TAG_SOUND_FORMAT_ADPCM               1
#define FLV_AUDIO_TAG_SOUND_FORMAT_MP3                 2
#define FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM_LE       3
#define FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO  4
#define FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO   5
#define FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER          6
#define FLV_AUDIO_TAG_SOUND_FORMAT_G711_A              7
#define FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU             8
#define FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED            9
#define FLV_AUDIO_TAG_SOUND_FORMAT_AAC                 10
#define FLV_AUDIO_TAG_SOUND_FORMAT_SPEEX               11
#define FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8               14
#define FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC     15

/* Audio Tag */
typedef u_byte flv_audio_tag;

#define flv_audio_tag_sound_type(tag)   (((tag) & 0x01) >> 0)
#define flv_audio_tag_sound_size(tag)   (((tag) & 0x02) >> 1)
#define flv_audio_tag_sound_rate(tag)   (((tag) & 0x0C) >> 2)
#define flv_audio_tag_sound_format(tag) (((tag) & 0xF0) >> 4)


/* video tag */
#define FLV_VIDEO_TAG_CODEC_JPEG            1
#define FLV_VIDEO_TAG_CODEC_SORENSEN_H263   2
#define FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO    3
#define FLV_VIDEO_TAG_CODEC_ON2_VP6         4
#define FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA   5
#define FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2 6
#define FLV_VIDEO_TAG_CODEC_AVC             7

#define FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME               1
#define FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME             2
#define FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME  3
#define FLV_VIDEO_TAG_FRAME_TYPE_GENERATED_KEYFRAME     4
#define FLV_VIDEO_TAG_FRAME_TYPE_COMMAND_FRAME          5

/* Video Tag */
typedef u_byte flv_video_tag;

#define flv_video_tag_codec_id(tag)     (((tag) & 0x0F) >> 0)
#define flv_video_tag_frame_type(tag)   (((tag) & 0xF0) >> 4)

/* AVC packet types */
typedef u_byte flv_avc_packet_type;

#define FLV_AVC_PACKET_TYPE_SEQUENCE_HEADER     0
#define FLV_AVC_PACKET_TYPE_NALU                1
#define FLV_AVC_PACKET_TYPE_SEQUENCE_END        2

/* AAC packet types */
typedef u_byte flv_aac_packet_type;

#define FLV_AAC_PACKET_TYPE_SEQUENCE_HEADER     0
#define FLV_AAC_PACKET_TYPE_RAW                 1




#define FLV_STREAM_STATE_START          0
#define FLV_STREAM_STATE_TAG            1
#define FLV_STREAM_STATE_TAG_BODY       2
#define FLV_STREAM_STATE_PREV_TAG_SIZE  3

typedef struct flv_stream_s {
    FILE                   *flvin;
    u_byte                  state;
    flv_tag_header_t        current_tag;
    u_int                   current_tag_offset;
    u_int                   current_tag_body_length;
    u_int                   current_tag_body_overflow;
} flv_stream_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FLV stream functions */
void        flv_init_stream(flv_stream_t ** stream);
flv_code    flv_open(const char * file_path, flv_stream_t * stream);
flv_code    flv_read_header(flv_stream_t * stream, flv_header_t * header);
flv_code    flv_read_prev_tag_size(flv_stream_t * stream, u_int * prev_tag_size);
flv_code    flv_read_tag(flv_stream_t * stream, flv_tag_header_t * tag);
flv_code    flv_read_audio_tag(flv_stream_t * stream, flv_audio_tag * tag);
flv_code    flv_read_video_tag(flv_stream_t * stream, flv_video_tag * tag);
flv_code    flv_read_metadata(flv_stream_t * stream, amf_data_t ** name, amf_data_t ** data);
size_t      flv_read_tag_body(flv_stream_t * stream, void * buffer, size_t buffer_size);
off_t       flv_get_current_tag_offset(flv_stream_t * stream);
off_t       flv_get_offset(flv_stream_t * stream);
void        flv_reset(flv_stream_t * stream);
void        flv_close(flv_stream_t * stream);


/* FLV buffer copy helper functions */
size_t      flv_copy_header(void * to, const flv_header_t * header, size_t buffer_size);
size_t      flv_copy_tag(void * to, const flv_tag_header_t * tag, size_t buffer_size);
size_t      flv_copy_prev_tag_size(void * to, u_int prev_tag_size, size_t buffer_size);


/* FLV stdio writing helper functions */
size_t      flv_write_header(FILE * out, const flv_header_t * header);
size_t      flv_write_tag(FILE * out, const flv_tag_header_t * tag);


/* FLV event based parser */
typedef struct flv_parser_s {
    flv_stream_t   *stream;
    void           *user_data;

    int (* on_header        )(flv_header_t * header, struct flv_parser_s * parser);
    int (* on_tag           )(flv_tag_header_t * tag, struct flv_parser_s * parser);
    int (* on_metadata_tag  )(flv_tag_header_t * tag, amf_data_t * name, amf_data_t * data, struct flv_parser_s * parser);
    int (* on_audio_tag     )(flv_tag_header_t * tag, flv_audio_tag audio_tag, struct flv_parser_s * parser);
    int (* on_video_tag     )(flv_tag_header_t * tag, flv_video_tag audio_tag, struct flv_parser_s * parser);
    int (* on_unknown_tag   )(flv_tag_header_t * tag, struct flv_parser_s * parser);
    int (* on_prev_tag_size )(u_int size, struct flv_parser_s * parser);
    int (* on_stream_end    )(struct flv_parser_s * parser);
} flv_parser_t;


flv_code flv_parse(const char * file, flv_parser_t * parser);


#ifdef __cplusplus
}
#endif /* __cplusplus */









typedef struct flv_tag_audio_s {
    /*  单位（bit）
    0 = Linear PCM, platform endian
    1 = ADPCM
    2 = MP3
    3 = Linear PCM, little endian
    4 = Nellymoser 16-kHz mono
    5 = Nellymoser 8-kHz mono
    6 = Nellymoser
    7 = G.711 A-law logarithmic PCM
    8 = G.711 mu-law logarithmic PCM
    9 = reserved
    10 = AAC
    11 = Speex
    14 = MP3 8-Khz
    15 = Device-specific sound 
        audioFormat:        4;  // 音频格式
        samplingRate:       2;  // 采样率，0 = 5.5-kHz，1 = 11-kHz，2 = 22-kHz，3 = 44-kHz，对于AAC总是3
        samplinglength:     1;  // 采样长度，0 = snd8Bit，1 = snd16Bit，压缩过的音频都是16bit
        audioType:          1;  // 音频类型，0 = sndMono，1 = sndStereo，对于AAC总是1
    */
    u_int64     audio_flags     ;  // 音频信息
    void       *audio_data      ;  // 音频数据
} flv_tag_audio_t;




typedef struct flv_tag_video_s {
    /*  单位（bit）
    1: keyframe (for AVC, a seekable frame)
    2: inter frame (for AVC, a non-seekable frame)
    3: disposable inter frame (H.263 only)
    4: generated keyframe (reserved for server use only)
    5: video info/command frame
        frameType:          4;  // 帧类型

    1: JPEG (currently unused)
    2: Sorenson H.263
    3: Screen video
    4: On2 VP6
    5: On2 VP6 with alpha channel
    6: Screen video version 2
    7: AVC
        encodeID:           4;  // 编码 ID
    */
    u_int64     video_flags     ;  // 视频信息
    void       *video_data      ;  // 视频数据
} flv_tag_video_t;



#endif /* __FLV_H__ */