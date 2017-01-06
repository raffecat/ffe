// result.h
// FireFly server result codes

#ifndef FIREFLY_RESULT_H
#define FIREFLY_RESULT_H

// result data type
typedef int RESULT;

// result testing macros
#define ISERROR(X) ((X) < 0)
#define NOTERROR(X) ((X) >= 0)

// specific success result codes
#define E_SUCCESS			0
#define E_FALSE             1
#define E_STOPPED           2		// thread has stopped

// specific failure result codes
#define E_INTERNAL			-1		// internal error
#define E_MEMORY            -2		// out of memory
#define E_NOT_IMPLEMENTED   -3		// not implemented
#define E_DATABASE          -4		// failed to open database

#define E_DELETED           -5		// object has been deleted
#define E_TYPE              -6		// type mismatch
#define E_RANGE             -7      // out of range
#define E_DENIED            -8      // access denied
#define E_ATTRIBUTE         -9      // undefined attribute
#define E_ARGS              -10     // wrong number of arguments to function
#define E_CLOSED            -11     // port has been closed
#define E_SOCKET            -12     // socket error
#define E_CONNECT           -13     // connect failed
#define E_ACCEPT            -14     // accept failed
#define E_CALL_NULL			-15     // cannot invoke null
#define E_CONTEXT			-16		// cannot invoke unbound function
#define E_LOCAL				-17		// undefined local variable
#define E_KEY               -18     // key not in mapping
#define E_NOT_DEFINED       -19     // system function not defined
#define E_INVOKE			-20		// cannot invoke non-function value
#define E_OBJECT			-21		// object does not exist
#define E_CHILDREN			-22		// object contains children
#define E_ATTRIBUTE_EXISTS	-23		// attribute already exists
#define E_OBJECT_EXISTS		-24		// object already exists
#define E_ASSERT            -25     // debug assertion failed
#define E_INHERIT           -26     // too deep inheritance
#define E_CLONE             -27     // cannot create an object within a clone
#define E_REPLACE			-28		// cannot replace an object
#define E_REMOVE			-29		// cannot remove an object
#define E_NO_YIELD			-30		// too long without yield

#endif // FIREFLY_RESULT_H
