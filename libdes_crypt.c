/* This file exists to give the constants in libdes-l/spr.h
 * somewhere to live. Do not include this file in the build
 * unless you are also building the following files from
 * libdes to implement crypt() on the win32 platform:
 *   - fcrypt.c
 *   - fcrypt_b.c
 *   - set_key.c
 */

#include <stdio.h>

#define DES_FCRYPT
#include "libdes-l/des_locl.h"
#undef DES_FCRYPT

#include "libdes-l/spr.h"
