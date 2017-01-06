/*
All branch targets should be 16-byte aligned [Intel]
Use multi-byte NOPs to align code.
One branch per cache line; BTBs are often keyed on the cache block address.
Initially predicts forward conditional branches to be not taken (exits)
Initially predicts backward conditional branches to be taken (loops)
Pentium Pro and above: test rax, rax ; cmovz rbx, rcx
PM/Core2 only predicts near jumps; far jumps, calls, returns always stall.
More than 64 iterations will suffer a misprediction next time the loop runs.
Pentium M and above do predict indirect branches to multiple targets.
On ARM, condition codes can be used with VFP (but not NEON)
Sandy/Ivy: ensure no more than 3 conditional branches per 16 bytes.
*/

// Profiling Timer.
#ifdef WIN32
#include <windows.h>
double get_time() {
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart/(double)f.QuadPart;
}
#else
#include <sys/time.h>
#include <sys/resource.h>
double get_time() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t); // CLOCK_PROCESS_CPUTIME_ID
    return t.tv_sec + t.tv_nsec*1e-9;
}
#endif

#include <stdio.h>

double test_reg();
double test_acc();

volatile double regRes = 0;
volatile double accRes = 0;

int __cdecl main(int argc, char* argv[]) {

	// warm caches.
	for (long i=0; i<10000000; i++) {
		accRes = test_acc();
		regRes = test_reg();
	}

	// registers.
	double regStart = get_time();
	for (long i=0; i<10000000; i++) {
		regRes = test_reg();
	}
	double regEnd = get_time();


	// warm caches.
	for (long i=0; i<10000000; i++) {
		regRes = test_reg();
		accRes = test_acc();
	}

	// accumulator.
	double accStart = get_time();
	for (long i=0; i<10000000; i++) {
		accRes = test_acc();
	}
	double accEnd = get_time();


	printf("Reg Result: %f\n", regRes);
	printf("Acc Result: %f\n", accRes);
	printf("Reg time: %f\n", (regEnd - regStart) * 1000.0);
	printf("Acc time: %f\n", (accEnd - accStart) * 1000.0);

	//getchar();

	return 0;
}

/*
Foreach expr we need
 get its last value, store in reg
 if cached valid, continue (predict taken)
   recurse to update the cached value

How will expressions actually be used?

When adding equip/effect,
 for each stat bonus on it,
   add the bonus to our bonus vector
 inc vector version
 if version is too high,
   reset version to 1 and all derived versions to 0

When removing equip/effect,
 for each stat bonus on it,
   subtract the bonus from our bonus vector
 inc vector version
 if version is too high,
   reset version to 1 and all derived versions to 0

To calculate hit damage,
 for each derived stat we need,
   if the derived version is not the vector version,
   update the derived stat by running its formula
   copy the derived to our registers
 for each bonus we need,
   copy the bonus to our registers
 run the formula

We need a way to incorporate stats from race and class,
and cache them in this object. Can we just include them in
the bonuses as if they were equipment?

Bonuses = Equipment + Effects + Race & Class

So attributes can be flagged as bonus-summed?
Or the field 'bonuses' is a summed set with a list of sources.
 So when a flagged field changes [and obj has bonuses], mark bonuses map for rebuilding?
 Array of refs; Ref field.
 The set of bonuses is an index (sums of numbers, key on field name)  { name : total }
 Likewise nouns are indexed (lists of refs, key on field value)       { noun : [ refs ] }

MIN(x,y) = x + ( ((y-x)>>(WORDBITS-1)) & (y-x) )

*/

#ifdef __GNUC__
// use -fprofile-arcs instead.
#define likely(expr)    __builtin_expect((expr), !0)
#define unlikely(expr)  __builtin_expect((expr), 0)
#else
#define likely(expr)
#define unlikely(expr)
#endif
