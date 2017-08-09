#include <stdlib.h>
#include <stdio.h>

#include "flv.h"


static char *
parse_args_file(int argc, char ** argv)
{
    if (argc <= 1) {
        std_log_error("empty file name");
        return NULL;
    }

    return argv[1];
}


flv_code
main(int argc, char ** argv)
{
    flv_code err_code;


    /* Create and open flv stream */
    flv_stream_t * stream;
    flv_init_stream(&stream);
    if (stream == NULL) {
        return FLV_ERROR_MEMORY;
    }
    flv_open( parse_args_file(argc, argv) , stream);


    /* Read flv header */
    flv_header_t *header = (flv_header_t*) malloc(sizeof(flv_header_t));
    err_code = flv_read_header(stream, header);
    if (err_code != FLV_OK) {
        return err_code;
    }


    /* Read tag prev size */
    int prev_tag_size;
    if ((err_code = flv_read_prev_tag_size(stream, &prev_tag_size)) != FLV_OK) {
        std_log_error("read tag size failed: %d", err_code);
        return err_code;
    }


    /* Read tag header */
    flv_tag_header_t *tag_header = (flv_tag_header_t*) std_calloc(sizeof(flv_tag_header_t));
    if ((err_code = flv_read_tag(stream, tag_header)) != FLV_OK) {
        std_log_error("read tag header failed: %d", err_code);
        return err_code;
    }
    std_log_info("tag_header = { tag_type:0x%x, body_length:%d, timestamp:0x%x, timestamp_ex:0x%x, stream_ID:0x%x }", 
                                tag_header->tag_type, tag_header->body_length, tag_header->timestamp, tag_header->timestamp_ex, tag_header->stream_ID);


    /* Read multi-tag */
    switch(tag_header->tag_type) {
    case 18U:   /* Read Metadata tag */ 
        {
            amf_data_t *name, *data;
            err_code = flv_read_metadata(stream, &name, &data);
            amf_data_dump(stdout, name, 0);
            amf_data_dump(stdout, data, 0);
        }
        break;
    case 9U:    /* Read Video tag */
        {
            flv_video_tag video_tag;
            err_code = flv_read_video_tag(stream, &video_tag);
            std_log_debug("video tag:  %d", video_tag);
        }
        break;
    case 8U:    /* Read Audio tag */
        {
            flv_audio_tag audio_tag;
            err_code = flv_read_audio_tag(stream, &audio_tag);
            std_log_debug("audio tag:  %d", audio_tag);
        }
        break;
    default:
        std_log_error("no match tag");
    }

    return err_code;
}