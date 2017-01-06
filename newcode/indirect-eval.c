#include <math.h>

// Do all math ops with a numeric register file.
// Convert external vars to numbers on load.
// Build a vector of strings to concat at the end.

// On load we will link in all native pointers and link up refs.
// We can yield on any command boundary (wait, read, write)
// How will that work? Because every activity is a thing.

// Allocate an activity card. Size for nums and refs.

typedef union op_t {
  void* op;
  double num;
} op_t;

op_t run() __attribute__ ((hot));

op_t run(unsigned char* ip, double* nums) {

  static void* labels[] = {
    &&OP_NUM,
    &&OP_END,
    &&OP_NEG,
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
    &&OP_SQRT,
    &&OP_LOG,
    &&OP_LN,
    &&OP_SIN,
    &&OP_COS,
    &&OP_TAN,
    &&OP_ABS,
    &&OP_CALL
  };

  if (!ip) { op_t res; res.op = (void*)labels; return res; }

  register double acc = 0;

  goto *labels[*ip++];

  OP_NUM: { acc = nums[*ip++]; goto *labels[*ip++]; }
  OP_LD: { acc = nums[*ip++]; goto *labels[*ip++]; }
  OP_NEG: { acc = -acc; goto *labels[*ip++]; }
  OP_ADD: { acc += nums[*ip++]; goto *labels[*ip++]; }
  OP_SUB: { acc -= nums[*ip++]; goto *labels[*ip++]; }
  OP_RSUB: { acc = nums[*ip++] - acc; goto *labels[*ip++]; }
  OP_MUL: { acc *= nums[*ip++]; goto *labels[*ip++]; }
  OP_DIV: { acc /= nums[*ip++]; goto *labels[*ip++]; }
  OP_RDIV: { acc = nums[*ip++] / acc; goto *labels[*ip++]; }
  OP_MOD: { acc = fmod(acc, nums[*ip++]); goto *labels[*ip++]; }
  OP_RMOD: { acc = fmod(nums[*ip++], acc); goto *labels[*ip++]; }
  OP_POW: { acc = pow(acc, nums[*ip++]); goto *labels[*ip++]; }
  OP_RPOW: { acc = pow(nums[*ip++], acc); goto *labels[*ip++]; }
  OP_SQRT: { acc = sqrt(acc); goto *labels[*ip++]; }
  OP_LOG: { acc = log(acc); goto *labels[*ip++]; }
  OP_LN: { acc = ln(acc); goto *labels[*ip++]; }
  OP_SIN: { acc = sin(acc); goto *labels[*ip++]; }
  OP_COS: { acc = cos(acc); goto *labels[*ip++]; }
  OP_TAN: { acc = tan(acc); goto *labels[*ip++]; }
  OP_ABS: { acc = (acc<0) ? -acc : acc; goto *labels[*ip++]; }
  OP_CALL: {
    // call out to a bound function in ip[1]
    // need to reserve reg space for it, and save our ip and acc.
  }
  OP_END: { op_t res; res.num = acc; return res; }

}
