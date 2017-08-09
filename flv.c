#include "flv.h"


/* FLV stream functions */
void 
flv_init_stream(flv_stream_t ** stream)
{
    *stream = (flv_stream_t*) std_calloc(sizeof(flv_stream_t));
    if (*stream == NULL) {
        std_log_error("alloc memory failed");
    }
}


flv_code
flv_open(const char * file_path, flv_stream_t * stream) /* stream must be calloc or memset with 0 */
{
    if (stream == NULL) {
        std_log_error("NULL-pointer stream, have you callocated?");
        return FLV_ERROR_MEMORY;
    }

    stream->flvin = fopen(file_path, "rb");
    if (stream->flvin == NULL) {
        std_log_error("file open failed: %s", file_path);
        free(stream);
        return FLV_ERROR_OPEN;
    }

    u_char str_flv[3];
    if (fread((u_char*) str_flv, sizeof(str_flv), 1, stream->flvin) == 0) {
        std_log_error("file read failed: %s", file_path);
        fclose(stream->flvin);
        free(stream);
        return FLV_ERROR_OPEN_READ;
    }

    if (str_flv[0] != 'F'
    ||  str_flv[1] != 'L'
    ||  str_flv[2] != 'V') 
    {
        std_log_error("Illegal flv file: %s", file_path);
        fclose(stream->flvin);
        free(stream);
        return FLV_ERROR_NO_FLV;
    }

    stream->state = FLV_STREAM_STATE_START;
    return FLV_OK;
}


flv_code 
flv_read_header(flv_stream_t * stream, flv_header_t * header) 
{
    if (stream == NULL
    ||  stream->flvin == NULL
    ||  feof(stream->flvin)
    ||  stream->state != FLV_STREAM_STATE_START)
    {
        std_log_error("some error occur");
        return FLV_ERROR_EOF;
    }

    if (fread(&header->version,     sizeof(header->version),    1, stream->flvin) == 0
    ||  fread(&header->flags,       sizeof(header->flags),      1, stream->flvin) == 0
    ||  fread(&header->offset,      sizeof(header->offset),     1, stream->flvin) == 0)
    {
        std_log_error("read stream header failed");
        return FLV_ERROR_EOF;
    }

    stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    return FLV_OK;
}


flv_code 
flv_read_prev_tag_size(flv_stream_t * stream, u_int * prev_tag_size) 
{
    if (stream == NULL || stream->flvin == NULL || feof(stream->flvin)) {
        std_log_error("some error occur");
        return FLV_ERROR_EOF;
    }

    /* skip remaining tag body bytes */
    if (stream->state == FLV_STREAM_STATE_TAG_BODY) {
        std_log_debug_pos("skip remaining tag body bytes");
        fseek(stream->flvin, stream->current_tag_offset + FLV_TAG_SIZE + stream->current_tag.body_length, SEEK_SET);
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    if (stream->state != FLV_STREAM_STATE_PREV_TAG_SIZE) {
        std_log_error("mismatched stream state, expected: %d, but got: %d", FLV_STREAM_STATE_PREV_TAG_SIZE, stream->state);
        return FLV_ERROR_EOF;
    }

    u_int val;
    if (fread(&val, sizeof(u_int), 1, stream->flvin) != 0) {
        stream->state = FLV_STREAM_STATE_TAG;
        *prev_tag_size = val;
        return FLV_OK;
    }

    return FLV_ERROR_EOF;
}


flv_code 
flv_read_tag(flv_stream_t * stream, flv_tag_header_t * tag) 
{
    if (stream == NULL || stream->flvin == NULL || feof(stream->flvin)) {
        std_log_error("some error occur");
        return FLV_ERROR_EOF;
    }

    /* skip header */
    if (stream->state == FLV_STREAM_STATE_START) {
        std_log_debug_pos("skip header");
        fseek(stream->flvin, FLV_HEADER_SIZE, SEEK_CUR);
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    /* skip current tag body */
    if (stream->state == FLV_STREAM_STATE_TAG_BODY) {
        std_log_debug_pos("skip current tag body");
        fseek(stream->flvin, stream->current_tag_offset + FLV_TAG_SIZE + stream->current_tag.body_length, SEEK_SET);
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    /* skip previous tag size */
    if (stream->state == FLV_STREAM_STATE_PREV_TAG_SIZE) {
        std_log_debug_pos("skip previous tag size");
        fseek(stream->flvin, sizeof(u_int), SEEK_CUR);
        stream->state = FLV_STREAM_STATE_TAG;
    }

    if (stream->state == FLV_STREAM_STATE_TAG) {
        u_int blen;
        stream->current_tag_offset = ftell(stream->flvin);

        if (fread(&tag->tag_type,       sizeof(tag->tag_type),      1, stream->flvin) == 0
        ||  fread(&blen,                FLV_TAG_SIZE_BODY_LENGTH,   1, stream->flvin) == 0
        ||  fread(&tag->timestamp,      FLV_TAG_SIZE_TIMESTAMP,     1, stream->flvin) == 0
        ||  fread(&tag->timestamp_ex,   sizeof(tag->timestamp_ex),  1, stream->flvin) == 0
        ||  fread(&tag->stream_ID,      FLV_TAG_SIZE_STREAM_ID,     1, stream->flvin) == 0)
        {
            std_log_error("read tag header failed");
            return FLV_ERROR_EOF;
        }
        blen = swap32_be(blen);
        tag->body_length = blen;

        memcpy(&stream->current_tag, tag, sizeof(flv_tag_header_t));
        stream->current_tag_body_length = blen;
        stream->current_tag_body_overflow = 0;
        stream->state = FLV_STREAM_STATE_TAG_BODY;
        return FLV_OK;
    }

    return FLV_ERROR_EOF;
}


flv_code
flv_read_audio_tag(flv_stream_t * stream, flv_audio_tag * tag)
{
    if (stream == NULL
    ||  stream->flvin == NULL
    ||  feof(stream->flvin)
    ||  stream->state != FLV_STREAM_STATE_TAG_BODY) 
    {
        std_log_error("some error occur");
        return FLV_ERROR_EOF;
    }

    if (stream->current_tag_body_length == 0) {
        std_log_error("empty audio tag");
        return FLV_ERROR_EMPTY_TAG;
    }

    if (fread(tag, sizeof(flv_audio_tag), 1, stream->flvin) == 0) {
        std_log_error("read audio tag failed");
        return FLV_ERROR_EOF;
    }


    if (stream->current_tag_body_length >= sizeof(flv_audio_tag)) {
        stream->current_tag_body_length -= sizeof(flv_audio_tag);
    } else {
        stream->current_tag_body_overflow = sizeof(flv_audio_tag) - stream->current_tag_body_length;
        stream->current_tag_body_length = 0;
    }


    if (stream->current_tag_body_length == 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
        if (stream->current_tag_body_overflow > 0) {
            fseek(stream->flvin, -(off_t) stream->current_tag_body_overflow, SEEK_CUR);
        }
    }

    return FLV_OK;
}


flv_code
flv_read_video_tag(flv_stream_t * stream, flv_video_tag * tag)
{
    if (stream == NULL
    ||  stream->flvin == NULL
    ||  feof(stream->flvin)
    ||  stream->state != FLV_STREAM_STATE_TAG_BODY)
    {
        std_log_error("some error occur");
        return FLV_ERROR_EOF;
    }

    if (stream->current_tag_body_length == 0) {
        std_log_error("empty video tag");
        return FLV_ERROR_EMPTY_TAG;
    }

    if (fread(tag, sizeof(flv_video_tag), 1, stream->flvin) == 0) {
        std_log_error("read video tag failed");
        return FLV_ERROR_EOF;
    }

    if (stream->current_tag_body_length >= sizeof(flv_video_tag)) {
        stream->current_tag_body_length -= sizeof(flv_video_tag);
    } else {
        stream->current_tag_body_overflow = sizeof(flv_video_tag) - stream->current_tag_body_length;
        stream->current_tag_body_length = 0;
    }

    if (stream->current_tag_body_length == 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
        if (stream->current_tag_body_overflow > 0) {
            fseek(stream->flvin, -(off_t) stream->current_tag_body_overflow, SEEK_CUR);
        }
    }

    return FLV_OK;
}


flv_code 
flv_read_metadata(flv_stream_t * stream, amf_data_t ** name, amf_data_t ** data)
{
    amf_data_t * d;
    amf_code e;
    size_t data_size;

    if (stream == NULL
    ||  stream->flvin == NULL
    ||  feof(stream->flvin)
    ||  stream->state != FLV_STREAM_STATE_TAG_BODY)
    {
        std_log_error("some error occur");
        return FLV_ERROR_EOF;
    }

    if (stream->current_tag_body_length == 0) {
        std_log_error("empty metadata tag");
        return FLV_ERROR_EMPTY_TAG;
    }

    /* read metadata tag name */
    d = amf_data_file_read(stream->flvin);
    *name = d;
    e = amf_data_get_error_code(d);
    if (e == AMF_ERROR_EOF) {
        return FLV_ERROR_EOF;
    } else if (e != AMF_ERROR_OK) {
        return FLV_ERROR_INVALID_METADATA_NAME;
    }

    /* if only name can be read, metadata are invalid */
    data_size = amf_data_size(d);
    if (stream->current_tag_body_length > data_size) {
        stream->current_tag_body_length -= (u_int) data_size;
    } else {
        stream->current_tag_body_length = 0;
        stream->current_tag_body_overflow = (u_int) data_size - stream->current_tag_body_length;

        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
        if (stream->current_tag_body_overflow > 0) {
            fseek(stream->flvin, -(off_t) stream->current_tag_body_overflow, SEEK_CUR);
        }

        return FLV_ERROR_INVALID_METADATA;
    }

    /* read metadata contents */
    d = amf_data_file_read(stream->flvin);
    *data = d;
    e = amf_data_get_error_code(d);
    if (e == AMF_ERROR_EOF) {
        std_log_error("invalid amf eof");
        return FLV_ERROR_EOF;
    }
    if (e != AMF_ERROR_OK) {
        std_log_error("read metadata tag failed: %d", e);
        return FLV_ERROR_INVALID_METADATA;
    }

    data_size = amf_data_size(d);
    if (stream->current_tag_body_length >= data_size) {
        stream->current_tag_body_length -= (u_int) data_size;
    } else {
        stream->current_tag_body_overflow = (u_int) data_size - stream->current_tag_body_length;
        stream->current_tag_body_length = 0;
    }

    if (stream->current_tag_body_length == 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
        if (stream->current_tag_body_overflow > 0) {
            fseek(stream->flvin, -(off_t) stream->current_tag_body_overflow, SEEK_CUR);
        }
    }

    return FLV_OK;
}


size_t
flv_read_tag_body(flv_stream_t * stream, void * buffer, size_t buffer_size)
{
    size_t bytes_number;

    if (stream == NULL
    ||  stream->flvin == NULL
    ||  feof(stream->flvin)
    ||  stream->state != FLV_STREAM_STATE_TAG_BODY)
    {
        std_log_error("som error occur");
        return 0;
    }

    bytes_number = (buffer_size > stream->current_tag_body_length) ? stream->current_tag_body_length : buffer_size;
    bytes_number = fread(buffer, sizeof(byte), bytes_number, stream->flvin);

    stream->current_tag_body_length -= (u_int) bytes_number;

    if (stream->current_tag_body_length == 0) {
        stream->state = FLV_STREAM_STATE_PREV_TAG_SIZE;
    }

    return bytes_number;
}


off_t
flv_get_current_tag_offset(flv_stream_t * stream) {
     return (stream != NULL) ? stream->current_tag_offset : 0;
}


off_t
flv_get_offset(flv_stream_t * stream) {
    return (stream != NULL) ? ftell(stream->flvin) : 0;
}


void
flv_reset(flv_stream_t * stream) 
{
    /* go back to beginning of file */
    if (stream != NULL && stream->flvin != NULL) {
        stream->current_tag_body_length = 0;
        stream->current_tag_offset = 0;
        stream->state = FLV_STREAM_STATE_START;

        fseek(stream->flvin, 0, SEEK_SET);
    }
}


void
flv_close(flv_stream_t * stream)
{
    if (stream != NULL) {
        if (stream->flvin != NULL) {
            fclose(stream->flvin);
        }
        free(stream);
    }
}


/* FLV buffer copy helper functions */
size_t 
flv_copy_header(void * to, const flv_header_t * header, size_t buffer_size)
{
    char * out = to;
    if (buffer_size < FLV_HEADER_SIZE) {
        return 0;
    }

    // memcpy(out, &header->signature, sizeof(header->signature));
    // out += sizeof(header->signature);
    out += 3;

    memcpy(out, &header->version, sizeof(header->version));
    out += sizeof(header->version);

    memcpy(out, &header->flags, sizeof(header->flags));
    out += sizeof(header->flags);

    memcpy(out, &header->offset, sizeof(header->offset));

    return FLV_HEADER_SIZE;
}


size_t 
flv_copy_tag(void * to, const flv_tag_header_t * tag, size_t buffer_size)
{
    char * out = to;
    if (buffer_size < FLV_TAG_SIZE) {
        return 0;
    }

    memcpy(out, &tag->tag_type, sizeof(tag->tag_type));
    out += sizeof(tag->tag_type);

    memcpy(out, &tag->body_length, sizeof(tag->body_length));
    out += sizeof(tag->body_length);

    memcpy(out, &tag->timestamp, sizeof(tag->timestamp));
    out += sizeof(tag->timestamp);

    memcpy(out, &tag->timestamp_ex, sizeof(tag->timestamp_ex));
    out += sizeof(tag->timestamp_ex);

    memcpy(out, &tag->stream_ID, sizeof(tag->stream_ID));

    return FLV_TAG_SIZE;
}


size_t 
flv_copy_prev_tag_size(void * to, u_int prev_tag_size, size_t buffer_size)
{
    u_int pts = prev_tag_size;

    if (buffer_size < sizeof(u_int)) {
        return 0;
    }

    memcpy(to, &pts, sizeof(u_int));

    return sizeof(u_int);
}


/* FLV stdio writing helper functions */
size_t 
flv_write_header(FILE * out, const flv_header_t * header)
{
    if (fwrite(FLV_SIGNATURE, 3, 1, out) == 0)
        return 0;
    if (fwrite(&header->version, sizeof(header->version), 1, out) == 0)
        return 0;
    if (fwrite(&header->flags, sizeof(header->flags), 1, out) == 0)
        return 0;
    if (fwrite(&header->offset, sizeof(header->offset), 1, out) == 0)
        return 0;
    return 1;
}


size_t 
flv_write_tag(FILE * out, const flv_tag_header_t * tag)
{
    if (fwrite(&tag->tag_type, sizeof(tag->tag_type), 1, out) == 0)
        return 0;
    if (fwrite(&tag->body_length, sizeof(tag->body_length), 1, out) == 0)
        return 0;
    if (fwrite(&tag->timestamp, sizeof(tag->timestamp), 1, out) == 0)
        return 0;
    if (fwrite(&tag->timestamp_ex, sizeof(tag->timestamp_ex), 1, out) == 0)
        return 0;
    if (fwrite(&tag->stream_ID, sizeof(tag->stream_ID), 1, out) == 0)
        return 0;
    return 1;
}


/* FLV event based parser */
flv_code
flv_parse(const char * file, flv_parser_t * parser)
{
    flv_code e;

    flv_header_t header;
    flv_tag_header_t tag;
    flv_audio_tag at;
    flv_video_tag vt;
    amf_data_t *name, *data;
    u_int prev_tag_size;

    if (parser == NULL) {
        return FLV_ERROR_EOF;
    }

    flv_init_stream(&parser->stream);
    if (parser->stream == NULL) {
        std_log_error("alloc memory failed");
        return FLV_ERROR_MEMORY;
    }
    if ((e = flv_open(file, parser->stream)) != FLV_OK) {
        free(parser->stream);
        return e;
    }

    e = flv_read_header(parser->stream, &header);
    if (e != FLV_OK) {
        std_log_error("read stream header failed");
        flv_close(parser->stream);
        return e;
    }

    if (parser->on_header != NULL) {
        e = parser->on_header(&header, parser);
        if (e != FLV_OK) {
            flv_close(parser->stream);
            return e;
        }
    }

    while (flv_read_tag(parser->stream, &tag) == FLV_OK) {
        if (parser->on_tag != NULL) {
            e = parser->on_tag(&tag, parser);
            if (e != FLV_OK) {
                flv_close(parser->stream);
                return e;
            }
        }

        if (tag.tag_type == FLV_TAG_HEADER_TYPE_AUDIO) {
            e = flv_read_audio_tag(parser->stream, &at);
            if (e == FLV_ERROR_EOF) {
                flv_close(parser->stream);
                return e;
            }
            if (e != FLV_ERROR_EMPTY_TAG && parser->on_audio_tag != NULL) {
                e = parser->on_audio_tag(&tag, at, parser);
                if (e != FLV_OK) {
                    flv_close(parser->stream);
                    return e;
                }
            }
        } else if (tag.tag_type == FLV_TAG_HEADER_TYPE_VIDEO) {
            e = flv_read_video_tag(parser->stream, &vt);
            if (e == FLV_ERROR_EOF) {
                flv_close(parser->stream);
                return e;
            }
            if (e != FLV_ERROR_EMPTY_TAG && parser->on_video_tag != NULL) {
                e = parser->on_video_tag(&tag, vt, parser);
                if (e != FLV_OK) {
                    flv_close(parser->stream);
                    return e;
                }
            }
        } else if (tag.tag_type == FLV_TAG_HEADER_TYPE_META) {
            name = data = NULL;
            e = flv_read_metadata(parser->stream, &name, &data);
            if (e == FLV_ERROR_EOF) {
                amf_data_free(name);
                amf_data_free(data);
                flv_close(parser->stream);
                return e;
            } else if (e == FLV_OK && parser->on_metadata_tag != NULL) {
                e = parser->on_metadata_tag(&tag, name, data, parser);
                if (e != FLV_OK) {
                    amf_data_free(name);
                    amf_data_free(data);
                    flv_close(parser->stream);
                    return e;
                }
            }
            amf_data_free(name);
            amf_data_free(data);
        } else {
            if (parser->on_unknown_tag != NULL) {
                e = parser->on_unknown_tag(&tag, parser);
                if (e != FLV_OK) {
                    flv_close(parser->stream);
                    return e;
                }
            }
        }
        e = flv_read_prev_tag_size(parser->stream, &prev_tag_size);
        if (e != FLV_OK) {
            flv_close(parser->stream);
            return e;
        }
        if (parser->on_prev_tag_size != NULL) {
            e = parser->on_prev_tag_size(prev_tag_size, parser);
            if (e != FLV_OK) {
                flv_close(parser->stream);
                return e;
            }
        }
    }

    if (parser->on_stream_end != NULL) {
        e = parser->on_stream_end(parser);
        if (e != FLV_OK) {
            flv_close(parser->stream);
            return e;
        }
    }

    flv_close(parser->stream);
    return FLV_OK;
}

















