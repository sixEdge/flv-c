#ifndef __AMF_H__
#define __AMF_H__


#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "std_log.h"
#include "util.h"


/* AMF data types */
typedef byte amf_type;
#define AMF_TYPE_NUMBER             ((byte)0x00)
#define AMF_TYPE_BOOLEAN	        ((byte)0x01)
#define AMF_TYPE_STRING	            ((byte)0x02)
#define AMF_TYPE_OBJECT	            ((byte)0x03)
#define AMF_TYPE_NULL               ((byte)0x05)
#define AMF_TYPE_UNDEFINED	        ((byte)0x06)
/* #define AMF_TYPE_REFERENCE	    ((byte)0x07) */
#define AMF_TYPE_ASSOCIATIVE_ARRAY	((byte)0x08)
#define AMF_TYPE_END                ((byte)0x09)
#define AMF_TYPE_ARRAY	            ((byte)0x0A)
#define AMF_TYPE_DATE	            ((byte)0x0B)
/* #define AMF_TYPE_SIMPLEOBJECT	((byte)0x0D) */
#define AMF_TYPE_XML	            ((byte)0x0F)
#define AMF_TYPE_CLASS	            ((byte)0x10)

/* AMF error codes */
typedef byte amf_code;
#define AMF_ERROR_OK                ((byte)0x00)
#define AMF_ERROR_EOF               ((byte)0x01)
#define AMF_ERROR_UNKNOWN_TYPE      ((byte)0x02)
#define AMF_ERROR_END_TAG           ((byte)0x03)
#define AMF_ERROR_NULL_POINTER      ((byte)0x04)
#define AMF_ERROR_MEMORY            ((byte)0x05)
#define AMF_ERROR_UNSUPPORTED_TYPE  ((byte)0x06)




typedef struct amf_node_s * p_amf_node;

/* string type */
typedef struct amf_string_s {
    u_short         size;
    byte           *mbstr;
} amf_string_t;

/* array type */
typedef struct amf_list_s {
    u_int           size;
    p_amf_node      first_element;
    p_amf_node      last_element;
} amf_list_t;

/* date type */
typedef struct amf_date_s {
    u_int64         milliseconds;
    short           timezone;
} amf_date_t;

/* XML string type */
typedef struct amf_xmlstring_s {
    u_int           size;
    char           *mbstr;
} amf_xmlstring_t;

/* class type */
typedef struct amf_class_s {
    amf_string_t    name;
    amf_list_t      elements;
} amf_class_t;

/* structure encapsulating the various AMF objects */
typedef struct amf_data_s {
    amf_type        type;
    amf_code        error_code;
    union {
        u_int64             number_data;
        u_byte              boolean_data;
        amf_string_t        string_data;
        amf_list_t          list_data;
        amf_date_t          date_data;
        amf_xmlstring_t     xmlstring_data;
        amf_class_t         class_data;
    };
} amf_data_t;

/* node used in lists, relies on amf_data */
typedef struct amf_node_s {
    amf_data_t     *data;
    p_amf_node      prev;
    p_amf_node      next;
} amf_node_t;




#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Pluggable backend support */
typedef size_t (*amf_read_proc)(void * out_buffer, size_t size, void * user_data);
typedef size_t (*amf_write_proc)(const void * in_buffer, size_t size, void * user_data);


/* read AMF data */
amf_data_t  *   amf_data_read(amf_read_proc read_proc, void * user_data);

/* write AMF data */
size_t          amf_data_write(const amf_data_t * data, amf_write_proc write_proc, void * user_data);


/* generic functions */

/* allocate an AMF data object */
amf_data_t  *   amf_data_new(byte type);
/* load AMF data from buffer */
amf_data_t  *   amf_data_buffer_read(byte * buffer, size_t maxbytes);
/* load AMF data from stream */
amf_data_t  *   amf_data_file_read(FILE * stream);
/* AMF data size */
size_t          amf_data_size(const amf_data_t * data);
/* write encoded AMF data into a buffer */
size_t          amf_data_buffer_write(amf_data_t * data, byte * buffer, size_t maxbytes);
/* write encoded AMF data into a stream */
size_t          amf_data_file_write(const amf_data_t * data, FILE * stream);
/* get the type of AMF data */
byte            amf_data_get_type(const amf_data_t * data);
/* get the error code of AMF data */
byte            amf_data_get_error_code(const amf_data_t * data);
/* return a new copy of AMF data */
amf_data_t  *   amf_data_clone(const amf_data_t * data);
/* release the memory of AMF data */
void            amf_data_free(amf_data_t * data);
/* dump AMF data into a stream as text */
void            amf_data_dump(FILE * stream, const amf_data_t * data, int indent_level);


/* return a null AMF object with the specified error code attached to it */
amf_data_t  *   amf_data_error(byte error_code);


/* number functions */
amf_data_t  *   amf_number_new(u_int64 value);
u_int64         amf_number_get_value(const amf_data_t * data);
void            amf_number_set_value(amf_data_t * data, u_int64 value);


/* boolean functions */
amf_data_t  *   amf_boolean_new(u_byte value);
u_byte          amf_boolean_get_value(const amf_data_t * data);
void            amf_boolean_set_value(amf_data_t * data, u_byte value);


/* string functions */
amf_data_t  *   amf_string_new(char * str, u_short size);
amf_data_t  *   amf_str(const char * str);
u_short         amf_string_get_size(const amf_data_t * data);
byte        *   amf_string_get_bytes(const amf_data_t * data);


/* object functions */
amf_data_t  *   amf_object_new(void);
u_int           amf_object_size(const amf_data_t * data);
amf_data_t  *   amf_object_add(amf_data_t * data, const char * name, amf_data_t * element);
amf_data_t  *   amf_object_get(const amf_data_t * data, const char * name);
amf_data_t  *   amf_object_set(amf_data_t * data, const char * name, amf_data_t * element);
amf_data_t  *   amf_object_delete(amf_data_t * data, const char * name);
amf_node_t  *   amf_object_first(const amf_data_t * data);
amf_node_t  *   amf_object_last(const amf_data_t * data);
amf_node_t  *   amf_object_next(amf_node_t * node);
amf_node_t  *   amf_object_prev(amf_node_t * node);
amf_data_t  *   amf_object_get_name(amf_node_t * node);
amf_data_t  *   amf_object_get_data(amf_node_t * node);


/* null functions */
#define amf_null_new()          amf_data_new(AMF_TYPE_NULL)

/* undefined functions */
#define amf_undefined_new()     amf_data_new(AMF_TYPE_UNDEFINED)


/* associative array functions */
amf_data_t  *   amf_associative_array_new(void);
#define amf_associative_array_size(d)       amf_object_size(d)
#define amf_associative_array_add(d, n, e)  amf_object_add(d, n, e)
#define amf_associative_array_get(d, n)     amf_object_get(d, n)
#define amf_associative_array_set(d, n, e)  amf_object_set(d, n, e)
#define amf_associative_array_delete(d, n)  amf_object_delete(d, n)
#define amf_associative_array_first(d)      amf_object_first(d)
#define amf_associative_array_last(d)       amf_object_last(d)
#define amf_associative_array_next(n)       amf_object_next(n)
#define amf_associative_array_prev(n)       amf_object_prev(n)
#define amf_associative_array_get_name(n)   amf_object_get_name(n)
#define amf_associative_array_get_data(n)   amf_object_get_data(n)


/* array functions */
amf_data_t  *   amf_array_new(void);
u_int           amf_array_size(const amf_data_t * data);
amf_data_t  *   amf_array_push(amf_data_t * data, amf_data_t * element);
amf_data_t  *   amf_array_pop(amf_data_t * data);
amf_node_t  *   amf_array_first(const amf_data_t * data);
amf_node_t  *   amf_array_last(const amf_data_t * data);
amf_node_t  *   amf_array_next(amf_node_t * node);
amf_node_t  *   amf_array_prev(amf_node_t * node);
amf_data_t  *   amf_array_get(amf_node_t * node);
amf_data_t  *   amf_array_get_at(const amf_data_t * data, u_int n);
amf_data_t  *   amf_array_delete(amf_data_t * data, amf_node_t * node);
amf_data_t  *   amf_array_insert_before(amf_data_t * data, amf_node_t * node, amf_data_t * element);
amf_data_t  *   amf_array_insert_after(amf_data_t * data, amf_node_t * node, amf_data_t * element);


/* date functions */
amf_data_t  *   amf_date_new(u_int64 milliseconds, short timezone);
u_int64         amf_date_get_milliseconds(const amf_data_t * data);
short           amf_date_get_timezone(const amf_data_t * data);
time_t          amf_date_to_time_t(const amf_data_t * data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AMF_H__ */