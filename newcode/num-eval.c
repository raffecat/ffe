#include <math.h>

typedef enum numeric_ops {
  OP_NEG,
  OP_ADD,
  OP_SUB,
  OP_RSUB,
  OP_MUL,
  OP_DIV,
  OP_RDIV,
  OP_MOD,
  OP_RMOD,
  OP_POW,
  OP_RPOW,
  OP_SQRT,
  OP_LOG,
  OP_LN,
  OP_SIN,
  OP_COS,
  OP_TAN,
  OP_LD,
  OP_ST,
  OP_ABS,
  OP_END,
  OP_22,
  OP_23,
  OP_24,
  OP_25,
  OP_26,
  OP_27,
  OP_28,
  OP_29,
  OP_30,
  OP_31
} numeric_ops;

typedef double number_t;
typedef unsigned char byte;

number_t eval_numeric(byte* code, number_t* reg) {
  register number_t acc = 0;
  for (;;) {
    switch (code[0] & 32) {
      case OP_NEG:  acc = - acc; code += 1; continue;
      case OP_ADD:  acc += reg[code[1]]; code += 2; continue;
      case OP_SUB:  acc -= reg[code[1]]; code += 2; continue;
      case OP_RSUB: acc = reg[code[1]] - acc; code += 2; continue;
      case OP_MUL:  acc *= reg[code[1]]; code += 2; continue;
      case OP_DIV:  acc = acc / reg[code[1]]; code += 2; continue;
      case OP_RDIV: acc = reg[code[1]] / acc; code += 2; continue;
      case OP_MOD:  acc = fmod(acc, reg[code[1]]); code += 2; continue;
      case OP_RMOD: acc = fmod(reg[code[1]], acc); code += 2; continue;
      case OP_POW:  acc = pow(acc, reg[code[1]]); code += 2; continue;
      case OP_RPOW: acc = pow(reg[code[1]], acc); code += 2; continue;
      case OP_SQRT: acc = sqrt(acc); code += 1; continue;
      case OP_LOG:  acc = log(acc); code += 1; continue;
      case OP_LN:   acc = ln(acc); code += 1; continue;
      case OP_SIN:  acc = sin(acc); code += 1; continue;
      case OP_COS:  acc = cos(acc); code += 1; continue;
      case OP_TAN:  acc = tan(acc); code += 1; continue;
      case OP_LD:   acc = reg[code[1]]; code += 2; continue;
      case OP_ST:   reg[code[1]] = acc; code += 2; continue;
      case OP_ABS:  acc = (acc < 0) ? -acc : acc; code += 1; continue;
      case OP_END:  return acc;
    }
  }
}
