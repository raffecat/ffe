// global.h
// FireFly server global include file

#ifndef FIREFLY_GLOBAL_H
#define FIREFLY_GLOBAL_H

#define DATABASE_MAGIC				{'M','F','F','E'}
#define DATABASE_VERSION			111
#define DATABASE_VERSION_0			110
#define FIREFLY_VERSION_STRING		"FireFly Server 1.14"

// Maximum search depth for inherited attributes (limits a recursive search)
#define MAX_INHERIT_DEPTH			8

// Define this to enable static assertions
// These will always crash the server (but will prevent corruption etc)
#define STATIC_ASSERTIONS

// Define this to enable safe runtime assertions
// These throw a runtime error instead of crashing the server
// If not defined, all runtime assertions will become static assertions
// The runtime error will not contain a description of the assertion
#undef RUNTIME_ASSERTIONS

// Define this to display the log to the console
#define DISPLAY_LOG

// Define this to log all AddRef/Release calls
#undef DEBUG_REF

// Not sure what this was for
#undef LOG_ALLOC

// Globally used header files
#include "platform.h"
#include "types.h"
#include "result.h"

// Debug assertion macros
#include <assert.h>
#ifdef STATIC_ASSERTIONS
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif
#ifdef RUNTIME_ASSERTIONS
#define RT_ASSERT_RET(X) if( !(X) ) return E_ASSERT;
#else
#define RT_ASSERT_RET(X) ASSERT(X)
#endif

// Global variables
extern bool g_bRunning;
extern const char* g_szErrorMessage;

// Global class instances
class index_sentinel_t;
template<class T> class index_t;
extern class database g_oDatabase;
extern index_t<class string_t> g_oIdxString;
extern index_t<class array_t> g_oIdxArray;
extern index_t<class mapping_t> g_oIdxMapping;
extern index_t<class object_t> g_oIdxObject;
extern index_t<class function_t> g_oIdxFunction;
extern index_t<class port_t> g_oIdxPort;
extern index_t<class thread_t> g_oIdxThread;

#endif // FIREFLY_GLOBAL_H
