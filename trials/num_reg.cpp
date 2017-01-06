
#include <stdint.h>

#if 0
// Struct of bytes method.
enum op_flags {
	f_const  = 0x01,
	f_neg    = 0x02,
	f_mul    = 0x04,
	f_last   = 0x08,
	A = 0,
	C = f_const,
	N = f_neg,
	M = f_mul,
	X = f_last
};
typedef unsigned char byte;
struct op_t {
	byte f, a, b, c;
};
#define OP(f,a,b,c) { (f), (a), (b), (c) }
#define Rf(P) ((P).f)
#define Ra(P) ((P).a)
#define Rb(P) ((P).b)
#define Rc(P) ((P).c)
#else
// Single uint32 method.
enum op_flags {
	f_const  = 0x0100,
	f_neg    = 0x0200,
	f_mul    = 0x0400,
	f_last   = 0x0800,
	A = 0,
	C = f_const,
	N = f_neg,
	M = f_mul,
	D = (f_mul|f_neg),
	X = f_last
};
typedef uint32_t op_t;
#define OP(f,a,b,c) (((a)<<0)|((b)<<16)|((c)<<24)|(f))
#define Rf(P) (P)
#define Ra(P) (((P)) & 0xFF)
#define Rb(P) (((P)>>16) & 0xFF)
#define Rc(P) (((P)>>24) & 0xFF)
#endif

/*
[-]a + (b|#)

a + b
a + #
# + a  (a + #)

a - b  (-a + b)
a - #  (a + -#)
# - a  (-a + #)

a * b  (a + 1 * b)
a * #
# * a  (a * #)

a / b
a / #  (a * 1/#)
# / a

*/

#if 0
// Intel version: cannot use conditional moves for doubles.
// This version is slower than below, best built with SSE2.
// Does not support division operation.
static __declspec(noinline)
void run_nums(op_t* fun, double* reg, double* consts) {
	for (;;) {
		op_t op = *fun++;
		double a = reg[Rb(op)];
		if (Rf(op) & f_neg) a = -a; // cmov
		double* ptr = reg;
		if (Rf(op) & f_const) ptr = consts; // cmov
		double b = ptr[Rc(op)];
		double res = a + b;
		if (Rf(op) & f_mul) res = a * b; // cmov
		reg[Ra(op)] = res;
		if (Rf(op) & f_last) break; // not taken
	}
}
#else
// Intel version: cannot use conditional moves for doubles.
// This version is faster than above, currently best built with SSE2.
static __declspec(noinline)
void run_nums(op_t* fun, double* reg, double* consts) {
	for (;;) {
		op_t op = *fun++;
		double b = reg[Rb(op)];
		double* ptr = reg;
		if (Rf(op) & f_const) ptr = consts; // cmov
		double c = ptr[Rc(op)];
		double res;
		if (!(Rf(op) & f_mul)) {
			if (!(Rf(op) & f_neg)) {
				res = c + b;
			} else {
				res = c - b;
			}
		} else {
			if (!(Rf(op) & f_neg)) {
				res = c * b;
			} else {
				res = c / b;
			}
		}
		reg[Ra(op)] = res;
		if (Rf(op) & f_last) break; // not taken
	}
}
#endif

double test_reg() {
	static double reg[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	static double consts[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	// 11 + 3 * int + 2 * wis
	consts[0] = 11.0;
	consts[1] = 3.0;
	consts[2] = 2.0;
	static op_t fun[] = {
		OP(M|C,   2, 0, 1),  // r2 = r0(int) * c1(3.0)
		OP(M|C,   3, 1, 2),  // r3 = r1(wis) * c2(2.0)
		OP(A,     2, 2, 3),  // r2 = r2 + r3
		OP(A|C|X, 0, 2, 0),  // r0 = r2 + c0(11.0)
	};

	// 3 * (int + wis) / 2
	consts[3] = 3.0;
	consts[4] = 0.5;
	static op_t fun2[] = {
		OP(A,     2, 0, 1),  // r2 = r0(int) + r1(wis)
		OP(M|C,   2, 2, 3),  // r2 = r2 * c3(3.0)
		OP(M|C|X, 0, 2, 4),  // r0 = r2 * c4(0.5)
	};

	// (10 * int + wis + 15 * wis + 2 * con) / dex
	consts[5] = 10.0;
	consts[6] = 15.0;
	consts[7] = 2.0;
	static op_t fun3[] = {
		OP(M|C,   4, 0, 5),  // r4 = r0(int) * c5(10.0)
		OP(A,     4, 4, 1),  // r4 = r4 + r1(wis)
		OP(M|C,   5, 1, 6),  // r5 = r1(wis) * c6(15.0)
		OP(A,     4, 4, 5),  // r4 = r4 + r5
		OP(M|C,   5, 2, 7),  // r5 = r2(con) * c7(2.0)
		OP(A,     4, 4, 5),  // r4 = r4 + r5
		OP(D|X,   0, 3, 4),  // r0 = r4 / r3(dex)
	};

	reg[0] = 15;  // r0 = int
	reg[1] = 11;  // r1 = wis
	run_nums(fun, reg, consts);

	reg[0] = 15;  // r0 = int
	reg[1] = 11;  // r1 = wis
	run_nums(fun2, reg, consts);

	reg[0] = 12;  // r0 = int
	reg[1] = 18;  // r1 = wis
	reg[2] = 16;  // r2 = con
	reg[3] = 11;  // r3 = dex
	run_nums(fun3, reg, consts);

	return reg[0];
}
