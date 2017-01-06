
#include <stdint.h>

// Single uint32 method.
enum op_flags {
	f_const  = 0x0100,
	f_neg    = 0x0200,
	f_mul    = 0x0400,
	f_swp    = 0x0800,
	f_last   = 0x1000,
	A = 0,
	L = 0,
	C = f_const,
	N = f_neg,
	M = f_mul,
	D = (f_mul|f_neg),
	S = f_swp,
	X = (f_swp|f_last)
};
typedef uint16_t op_t;
#define OP(f,a) ((f)|(a))
#define STLD(a,b) (((b)<<8)|(a))
#define Rf(P) (P)
#define Ra(P) ((P) & 0xFF)
#define Rb(P) (((P)>>8) & 0xFF)

// Intel version: cannot use conditional moves for doubles.
// Best built with x87+CMOV (not SSE2)
static __declspec(noinline)
void run_nums(op_t* fun, double* reg, double* consts) {
	double acc = reg[*fun++];
	for (;;) {
		op_t op = *fun++;
		// acc = (Rf(op) & f_mul) ? ( (Rf(op) & f_neg) ? (acc/a) : (acc*a) ) : ( (Rf(op) & f_neg) ? (acc-a) : (acc+a) );
		double* ptr = reg;
		if (Rf(op) & f_const) ptr = consts; // cmov
		double a = ptr[Ra(op)];
		if (!(Rf(op) & f_mul)) {
			if (!(Rf(op) & f_neg)) {
				acc += a;
				if (!(Rf(op) & f_swp)) continue; // taken
			} else {
				acc -= a;
				if (!(Rf(op) & f_swp)) continue; // taken
			}
		} else {
			if (!(Rf(op) & f_neg)) {
				acc *= a;
				if (!(Rf(op) & f_swp)) continue; // taken
			} else {
				acc /= a;
				if (!(Rf(op) & f_swp)) continue; // taken
			}
		}
		if (Rf(op) & f_last) break; // not taken
		op_t swp = *fun++;
		reg[Ra(swp)] = acc;
		acc = reg[Rb(swp)];
	}
	reg[0] = acc;
}


double test_acc() {
	static double reg[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	static double consts[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	// 11 + 3 * int + 2 * wis
	consts[0] = 11.0;
	consts[1] = 3.0;
	consts[2] = 2.0;
	static op_t fun[] = {
		OP(L,     0),        // lda r0(int)
		OP(M|C,   1),        // mul c1(3.0)
		OP(A|C|S, 0),        // add c0(11.0)
		STLD(2,   1),        // sta r2, lda r1(wis)
		OP(M|C,   2),        // mul c2(2.0)
		OP(A|X,   2),        // add r2
		/*
		OP(M|C,   2, 0, 1),  // r2 = r0(int) * c1(3.0)
		OP(M|C,   3, 1, 2),  // r3 = r1(wis) * c2(2.0)
		OP(A,     2, 2, 3),  // r2 = r2 + r3
		OP(A|C|X, 0, 2, 0),  // r0 = r2 + c0(11.0)
		*/
	};

	// 3 * (int + wis) / 2
	consts[3] = 3.0;
	consts[4] = 0.5;
	static op_t fun2[] = {
		OP(L,     0),        // lda r0(int)
		OP(A,     1),        // add r1(wis)
		OP(M|C,   3),        // mul c3(3.0)
		OP(M|C|X, 4),        // mul c4(0.5)
		/*
		OP(A,     2, 0, 1),  // r2 = r0(int) + r1(wis)
		OP(M|C,   2, 2, 3),  // r2 = r2 * c3(3.0)
		OP(M|C|X, 0, 2, 4),  // r0 = r2 * c4(0.5)
		*/
	};

	// (10 * int + wis + 15 * wis + 2 * con) / dex
	consts[5] = 10.0;
	consts[6] = 15.0;
	consts[7] = 2.0;
	static op_t fun3[] = {
		OP(L,     0),        // lda r0(int)
		OP(M|C,   5),        // mul c5(10.0)
		OP(A|S,   1),        // add r1(wis)
		STLD(4,   1),        // sta r4, lda r1(wis)
		OP(M|C,   6),        // mul c6(15.0)
		OP(A|S,   4),        // add r4
		STLD(4,   2),        // sta r4, lda r2(con)
		OP(M|C,   7),        // mul c7(2.0)
		OP(A,     4),        // add r4
		OP(D|X,   3),        // div r3(dex)
		/*
		OP(M|C,   4, 0, 5),  // r4 = r0(int) * c5(10.0)
		OP(A,     4, 4, 1),  // r4 = r4 + r1(wis)
		OP(M|C,   5, 1, 6),  // r5 = r1(wis) * c6(15.0)
		OP(A,     4, 4, 5),  // r4 = r4 + r5
		OP(M|C,   5, 2, 7),  // r5 = r2(con) * c7(2.0)
		OP(A,     4, 4, 5),  // r4 = r4 + r5
		OP(D|X,   0, 3, 4),  // r0 = r4 / r3(dex)
		*/
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
