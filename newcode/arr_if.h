// Array Interface

#ifndef ARR_PREFIX
#error Must define ARR_PREFIX before including "arr_if.h"
#endif

#ifndef ARRAY_FUNCS
#define ARRAY_FUNCS

// Length of array.
#define LEN(P) ((P)->len)

// Capacity of array.
#define CAP(P) ((P)->cap)

// Element of array.
#define ELEM(P,I) ((P)->elems[I])

// Push element to array.
#define PUSH_VAL(T,P,V) do{ if ((P)->len < (P)->cap) { (P)->elems[(P)->len++] = (V); } else { T##_push( (P), (V) ); }
#define PUSH_REF(T,P,V) do{ if ((P)->len < (P)->cap) { (P)->elems[(P)->len++] = (V); } else { T##_push( (P), &(V) ); }

// There isn't really enough here to require templating.

#endif
