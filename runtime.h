// runtime.h
// FireFly server runtime constants

#ifndef FIREFLY_RUNTIME_H
#define FIREFLY_RUNTIME_H

#define MAX_LOCALS 255

// Register allocation types (compiler)

#define RTYP_RESERVED                    0
#define RTYP_LOCAL                       1
#define RTYP_TEMP                        2
#define RTYP_CONST                       3
#define RTYP_MEMBER                      4

#define RTYP_MASK                        0x0F
#define RTYP_INVOKE                      0x10
#define RTYP_LVALUE                      0x20

// Reserved register indices

#define IDXL_THIS                        0
#define IDXL_ARGS                        1

#define INVALID_REGISTER				 255

/*
// Intrinsic functions

#define INTRINSIC_ZERO                   0

#define INTRINSIC_COPY                   1
#define INTRINSIC_SLEEP                  2

#define INTRINSIC_CREATE_CLASS           3
#define INTRINSIC_DELETE_CLASS           4
#define INTRINSIC_CREATE_MEMBER          5
#define INTRINSIC_DELETE_MEMBER          6
#define INTRINSIC_CREATE_STATIC          7
#define INTRINSIC_DELETE_STATIC          8
#define INTRINSIC_CREATE_GLOBAL          9
#define INTRINSIC_DELETE_GLOBAL          10

#define INTRINSIC_COMPILE                11
#define INTRINSIC_UNCOMPILE              12

#define INTRINSIC_CONNECT                13
#define INTRINSIC_LISTEN                 14
#define INTRINSIC_ACCEPT                 15
#define INTRINSIC_CLOSE                  16
#define INTRINSIC_READ                   17
#define INTRINSIC_WRITE                  18
#define INTRINSIC_READLN                 19
#define INTRINSIC_WRITELN                20
#define INTRINSIC_GETCH                  21

#define INTRINSIC_INHERIT                22
#define INTRINSIC_INHERITS               23

#define INTRINSIC_CENTER                 24
#define INTRINSIC_CAPITALISE             25
#define INTRINSIC_TOUPPER                26
#define INTRINSIC_TOLOWER                27
#define INTRINSIC_LTRIM                  28
#define INTRINSIC_RTRIM                  29
#define INTRINSIC_TRIM                   30
#define INTRINSIC_LEFT                   31
#define INTRINSIC_RIGHT                  32
#define INTRINSIC_MID                    33
#define INTRINSIC_WRAP                   34
#define INTRINSIC_CHAR                   35
#define INTRINSIC_ORD                    36
#define INTRINSIC_CRYPT                  37
#define INTRINSIC_INSTR                  38

#define NUM_INTRINSIC                    39
*/

#endif // FIREFLY_RUNTIME_H
