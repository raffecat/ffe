// types.h
// FireFly server intrinsic data types

#ifndef FIREFLY_TYPES_H
#define FIREFLY_TYPES_H

// basic driver data type aliases
#include <stdint.h>
typedef unsigned char byte;
typedef int32_t int32;
typedef uint32_t uint32;
typedef double float64;

// intrinsic data types
typedef class string_t* stringp;
typedef class array_t* arrayp;
typedef class mapping_t* mappingp;
typedef class object_t* objectp;
typedef class function_t* functionp;
typedef class port_t* portp;
typedef class thread_t* threadp;

typedef struct range_s {
    int32 nLower;
    int32 nUpper;
} range_t;

// Reference to function with context
typedef struct func_ref_s {
    functionp fnValue;
    objectp oContext;
} func_ref_t;

typedef struct att_name_s {
	objectp oValue;
	stringp sName;
} att_name_t;

typedef struct arr_index_s {
	arrayp arValue;
	int32 nIndex;
} arr_index_t;

typedef struct arr_slice_s {
	arrayp arValue;
	int32 nReg; // Register containing range_t
} arr_slice_t;

typedef struct map_key_s {
	mappingp mpValue;
	int32 nReg; // Register containing key (any_t)
} map_key_t;

typedef struct for_each_s {
	arrayp arIterate;
	int32 nIndex;
} for_each_t;


// Intrinsic type constants
#define TYPE_UNDEFINED		0
#define TYPE_NULL			1
#define TYPE_INTEGER		2
#define TYPE_FLOAT			3
#define TYPE_RANGE			4
#define TYPE_STRING         5
#define TYPE_ARRAY          6
#define TYPE_MAPPING        7
#define TYPE_OBJECT         8
#define TYPE_PORT           9
#define TYPE_THREAD         10
#define TYPE_FUNCTION       11
#define TYPE_FUNC_REF		12
#define TYPE_ATT_NAME		13
#define TYPE_ARR_INDEX		14
#define TYPE_ARR_SLICE		15
#define TYPE_MAP_KEY		16
#define TYPE_FOR_EACH		17
#define TYPE_OBJECT_REF		18
#define TYPE_DELETED		19

#endif // FIREFLY_TYPES_H
