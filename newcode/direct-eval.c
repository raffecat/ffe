#include <math.h>

// Do all math ops with a numeric register file.
// Convert external vars to numbers on load.
// Emit (build a vector of) strings to concat at the end.
// This technique is good for flat coroutines.

// On load we will link in all native pointers and link up refs.
// We can yield on any command boundary (wait, read, write)

// NB. both exits of a branch instruction SHOULD use their own goto *ip++
// for better BTB prediction.

// Generated branch-free traces (specialised from branching code)

// #if defined(__GNUC__)

typedef struct object_t {
  int refs;
} object_t;

typedef union op_t {
  void* op;
  double num;
  int i;
  object_t* ref;
  union op_t* code;
} op_t;

#define REG(OP) ((op_t*)(reg+(OP).i))

typedef unsigned char byte;
void dealloc(object_t* ptr);

op_t run() __attribute__ ((hot));

op_t run(op_t* ip, byte* reg) {

  static void* labels[] = {
    &&OP_LDN,
    &&OP_LDN_I,
    &&OP_LDN_V,
    &&OP_STN,
    &&OP_STN_V,

    &&OP_ADD,
    &&OP_SUB,
    &&OP_RSUB,
    &&OP_MUL,
    &&OP_DIV,
    &&OP_RDIV,
    &&OP_MOD,
    &&OP_RMOD,
    &&OP_POW,
    &&OP_RPOW,

    &&OP_ADD_I,
    &&OP_SUB_I,
    &&OP_RSUB_I,
    &&OP_MUL_I,
    &&OP_DIV_I,
    &&OP_RDIV_I,
    &&OP_MOD_I,
    &&OP_RMOD_I,
    &&OP_POW_I,
    &&OP_RPOW_I,

    &&OP_NEG,
    &&OP_ABS,
    &&OP_SQRT,
    &&OP_LOG,
    &&OP_LN,
    &&OP_SIN,
    &&OP_COS,
    &&OP_TAN,
    &&OP_ASIN,
    &&OP_ACOS,
    &&OP_ATAN,
    &&OP_ATAN2,

    &&OP_LDR,
    &&OP_LDR_I,
    &&OP_LDR_V,
    &&OP_STR,
    &&OP_IS,
    &&OP_ISN,
    &&OP_IS_I,
    &&OP_ISN_I,

    &&OP_END
  };

  if (!ip) { op_t res; res.op = (void*)labels; return res; }

  register double acc = 0;
  register object_t* ref = 0;

  goto *ip++;

  // these ops can't directly ref the temporary registers
  // unless we do not allow re-entrant use of the compiled ops.

  OP_LDN: { acc = REG(*ip++)->num; goto *ip++; }
  OP_LDN_I: { acc = (*ip++).num; goto *ip++; }
  OP_LDN_V: { ip++; goto *ip++; } // TODO
  OP_STN: { REG(*ip++)->num = acc; goto *ip++; }
  OP_STN_V: { ip++; goto *ip++; } // TODO

  OP_ADD: { acc += REG(*ip++)->num; goto *ip++; }
  OP_SUB: { acc -= REG(*ip++)->num; goto *ip++; }
  OP_RSUB: { acc = REG(*ip++)->num - acc; goto *ip++; }
  OP_MUL: { acc *= REG(*ip++)->num; goto *ip++; }
  OP_DIV: { acc /= REG(*ip++)->num; goto *ip++; }
  OP_RDIV: { acc = REG(*ip++)->num / acc; goto *ip++; }
  OP_MOD: { acc = fmod(acc, REG(*ip++)->num); goto *ip++; }
  OP_RMOD: { acc = fmod(REG(*ip++)->num, acc); goto *ip++; }
  OP_POW: { acc = pow(acc, REG(*ip++)->num); goto *ip++; }
  OP_RPOW: { acc = pow(REG(*ip++)->num, acc); goto *ip++; }

  OP_ADD_I: { acc += (*ip++).num; goto *ip++; }
  OP_SUB_I: { acc -= (*ip++).num; goto *ip++; }
  OP_RSUB_I: { acc = (*ip++).num - acc; goto *ip++; }
  OP_MUL_I: { acc *= (*ip++).num; goto *ip++; }
  OP_DIV_I: { acc /= (*ip++).num; goto *ip++; }
  OP_RDIV_I: { acc = (*ip++).num / acc; goto *ip++; }
  OP_MOD_I: { acc = fmod(acc, (*ip++).num); goto *ip++; }
  OP_RMOD_I: { acc = fmod((*ip++).num, acc); goto *ip++; }
  OP_POW_I: { acc = pow(acc, (*ip++).num); goto *ip++; }
  OP_RPOW_I: { acc = pow((*ip++).num, acc); goto *ip++; }

  OP_NEG: { acc = -acc; goto *ip++; }
  OP_ABS: { acc = fabs(acc); goto *ip++; }
  OP_SQRT: { acc = sqrt(acc); goto *ip++; }
  OP_LOG: { acc = log(acc); goto *ip++; }
  OP_LN: { acc = ln(acc); goto *ip++; }
  OP_SIN: { acc = sin(acc); goto *ip++; }
  OP_COS: { acc = cos(acc); goto *ip++; }
  OP_TAN: { acc = tan(acc); goto *ip++; }
  OP_ASIN: { acc = asin(acc); goto *ip++; }
  OP_ACOS: { acc = acos(acc); goto *ip++; }
  OP_ATAN: { acc = atan(acc); goto *ip++; }
  OP_ATAN2: { acc = atan2(acc, REG(*ip++)->num); goto *ip++; }

  OP_LDR: { ref = REG(*ip++)->ref; goto *ip++; }
  OP_LDR_I: { ref = (*ip++).ref; goto *ip++; }
  OP_LDR_V: { ip++; goto *ip++; } // TODO
  OP_STR: {
    object_t** ptr = &REG(*ip++)->ref;
    ref->refs++; // add ref before store
    if (*ptr && !--(*ptr)->refs) dealloc(*ptr); // release stored ref
    *ptr = ref; // store new ref
    goto *ip++;
  }
  OP_IS: { if (ref == REG(*ip++)->ref) { ip = (*ip).code; goto *ip++; } else { ip++; goto *ip++; } }
  OP_ISN: { if (ref != REG(*ip++)->ref) { ip = (*ip).code; goto *ip++; } else { ip++; goto *ip++; } }
  OP_IS_I: { if (ref == (*ip++).ref) { ip = (*ip).code; goto *ip++; } else { ip++; goto *ip++; } }
  OP_ISN_I: { if (ref != (*ip++).ref) { ip = (*ip).code; goto *ip++; } else { ip++; goto *ip++; } }

  OP_END: { op_t res; res.num = acc; return res; }

}
