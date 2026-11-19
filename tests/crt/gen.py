#!/usr/bin/env python3
"""Write the C test for the compiler's helper routines to standard output.

The cases and the answers are worked out here, on the host; the test
program does the same arithmetic on the 286 and compares.  It prints
nothing in decimal, because that would need the routines under test.
"""
import random

random.seed(286)


def s32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


def trunc_div(a, b):
    q = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        q = -q
    return q, a - q * b


edge = [0, 1, -1, 2, -2, 7, -7, 255, 256, 65535, 65536, 65537, -65536,
        0x7FFF, 0x8000, 0x12345678, -0x12345678, 0x7FFFFFFF, -0x7FFFFFFF,
        100000, -100000, 3, 10, 1000]
pairs = [(a, b) for a in edge for b in edge]
for _ in range(300):
    bits_a, bits_b = random.choice([8, 15, 16, 17, 24, 31]), random.choice([4, 8, 15, 16, 17, 24, 31])
    a = random.getrandbits(bits_a) * random.choice([1, -1])
    b = random.getrandbits(bits_b) * random.choice([1, -1])
    pairs.append((a, b))
# A divisor over 16 bits is divided by after shifting, and the quotient
# that gives is checked by multiplying back (i4d.s).  It is one too many
# for dividends just short of a multiple of the divisor: the first three
# pairs each for one of the ways that is found out, the product's low
# word too big, its high word, and a product beyond 32 bits; then more of
# the kind, and dividends at the top of the range.
pairs += [(s32(a), s32(b)) for a, b in [
    (0x005afeb7, 0x000f2a74), (0x070cffaf, 0x00c88e3b), (0xffffff45, 0x00086734),
    (0x4a9fa414, 0x4a9fa478), (0x3a4ebf86, 0x0003288f), (0x6c1bffa7, 0x000f25f9),
    (0xffffff62, 0x0009d1de), (0xffffffff, 0x00010000), (0xffffffff, 0x00010001),
    (0xffffffff, 0xffffffff), (0xfffffffe, 0xffffffff), (0x80000000, 0x7fffffff),
    (0xffff0000, 0x0000ffff), (0xffffffff, 0x8000ffff), (0x00010000, 0x00010000)]]
for _ in range(200):
    b = random.getrandbits(random.choice([17, 18, 20, 24, 28, 31, 32])) | 0x10000
    a = b * random.getrandbits(random.choice([1, 4, 8, 15, 16])) - random.choice([0, 1, 2, 3, 100])
    if not 0 <= a <= 0xFFFFFFFF:
        a = 0xFFFFFFFF - random.getrandbits(8)
    pairs.append((s32(a), s32(b)))
pairs = [(a, b) for a, b in pairs if b != 0]


def clong(x):
    return "0x%08xL" % (x & 0xFFFFFFFF)


doubles = [0.0, 0.5, -0.5, 0.99, -0.99, 1.0, -1.0, 1.5, -1.5, 2.5, -2.5, 2.7,
           -2.7, 3.999, -3.999, 32767.9, -32768.0, 65536.5, -65536.5,
           123456.789, -123456.789, 2147483000.75, -2147483000.75]

print("/* Made by tests/crt/gen.py; do not edit. */")
print("extern void putc();")
print("#define N %d" % len(pairs))
print("static long A[N] = {%s};" % ", ".join(clong(a) for a, b in pairs))
print("static long B[N] = {%s};" % ", ".join(clong(b) for a, b in pairs))
print("static long MUL[N] = {%s};" % ", ".join(clong(a * b) for a, b in pairs))
print("static long SDIV[N] = {%s};" % ", ".join(clong(trunc_div(a, b)[0]) for a, b in pairs))
print("static long SREM[N] = {%s};" % ", ".join(clong(trunc_div(a, b)[1]) for a, b in pairs))
u = [(a & 0xFFFFFFFF, b & 0xFFFFFFFF) for a, b in pairs]
print("static unsigned long UDIV[N] = {%s};" % ", ".join(clong(a // b) for a, b in u))
print("static unsigned long UREM[N] = {%s};" % ", ".join(clong(a % b) for a, b in u))
print("#define ND %d" % len(doubles))
print("static double D[ND] = {%s};" % ", ".join(repr(d) for d in doubles))
print("static long DL[ND] = {%s};" % ", ".join(clong(int(d)) for d in doubles))
print("static double D2[ND] = {%s};" % ", ".join(repr(d * 2.5 + 1.0) for d in doubles))
# The 287 works to 64 bits of precision and the host to 53.  Compare a
# computed double only where the arithmetic is exact in both.
print("static char EXACT[ND] = {%s};" % ", ".join(str(int(d * 8 == int(d * 8))) for d in doubles))
print(r'''
static int pass, fail;

static void puts_(s) char *s; { while (*s) putc(*s++); }

static void hex(x) unsigned x;
{
	int i;
	for (i = 12; i >= 0; i -= 4)
		putc("0123456789abcdef"[(x >> i) & 15]);
}

static void check(what, i, got, want) char *what; int i; long got, want;
{
	if (got == want) { pass++; return; }
	fail++;
	if (fail > 12) return;
	puts_("FAIL "); puts_(what); puts_(" case "); hex(i);
	puts_(" got "); hex((unsigned)(got >> 16)); hex((unsigned)got);
	puts_(" want "); hex((unsigned)(want >> 16)); hex((unsigned)want);
	puts_("\r\n");
}

double scale(x) double x; { return x * 2.5 + 1.0; }

int main()
{
	int i;
	unsigned long ua, ub;

	for (i = 0; i < N; i++) {
		check("mul", i, A[i] * B[i], MUL[i]);
		check("div", i, A[i] / B[i], SDIV[i]);
		check("rem", i, A[i] % B[i], SREM[i]);
		ua = A[i]; ub = B[i];
		check("umul", i, (long)(ua * ub), MUL[i]);
		check("udiv", i, (long)(ua / ub), (long)UDIV[i]);
		check("urem", i, (long)(ua % ub), (long)UREM[i]);
	}
	for (i = 0; i < ND; i++) {
		check("double to long", i, (long)D[i], DL[i]);
		if (DL[i] >= -32768L && DL[i] <= 32767L)
			check("double to int", i, (long)(int)D[i], DL[i]);
		if (EXACT[i])
			check("double result", i, (long)(scale(D[i]) == D2[i]), 1L);
	}
	check("rounding left alone", 0, (long)(int)(D[11] + 0.0), 2L);
	puts_("crt: pass="); hex(pass); puts_(" fail="); hex(fail); puts_("\r\n");
	return fail != 0;
}
''')
