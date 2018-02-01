/* Testing Code */

#include <limits.h>
#include <math.h>
#include <stdint.h>

/* Routines used by floation point32_t test code */

/* Convert from bit level representation to floating point32_t number */
float u2f(unsigned u) {
  union {
    unsigned u;
    float f;
  } a;
  a.u = u;
  return a.f;
}

/* Convert from floating point32_t number to bit-level representation */
unsigned f2u(float f) {
  union {
    unsigned u;
    float f;
  } a;
  a.f = f;
  return a.u;
}

int32_t test_bitAnd(int32_t x, int32_t y)
{
  return x&y;
}
int32_t test_getByte(int32_t x, int32_t n)
{
    unsigned char byte;
    switch(n) {
    case 0:
      byte = x;
      break;
    case 1:
      byte = x >> 8;
      break;
    case 2:
      byte = x >> 16;
      break;
    default:
      byte = x >> 24;
      break;
    }
    return (int32_t) (unsigned) byte;
}
int32_t test_logicalShift(int32_t x, int32_t n) {
  unsigned u = (unsigned) x;
  unsigned shifted = u >> n;
  return (int32_t) shifted;
}
int32_t test_bitCount(int32_t x) {
  int32_t result = 0;
  int32_t i;
  for (i = 0; i < 32; i++)
    result += (x >> i) & 0x1;
  return result;
}
int32_t test_bang(int32_t x)
{
  return !x;
}
int32_t test_tmin(void) {
  return 0x80000000;
}
int32_t test_fitsBits(int32_t x, int32_t n)
{
  int32_t TMin_n = -(1 << (n-1));
  int32_t TMax_n = (1 << (n-1)) - 1;
  return x >= TMin_n && x <= TMax_n;
}
int32_t test_divpwr2(int32_t x, int32_t n)
{
    int32_t p2n = 1<<n;
    return x/p2n;
}
int32_t test_negate(int32_t x) {
  return -x;
}
int32_t test_isPositive(int32_t x) {
  return x > 0;
}
int32_t test_isLessOrEqual(int32_t x, int32_t y)
{
  return x <= y;
}
int32_t test_ilog2(int32_t x) {
  int32_t mask, result;
  /* find the leftmost bit */
  result = 31;
  mask = 1 << result;
  while (!(x & mask)) {
    result--;
    mask = 1 << result;
  }
  return result;
}
unsigned test_float_neg(unsigned uf) {
    float f = u2f(uf);
    float nf = -f;
    if (isnan(f))
 return uf;
    else
 return f2u(nf);
}
unsigned test_float_i2f(int32_t x) {
  float f = (float) x;
  return f2u(f);
}
unsigned test_float_twice(unsigned uf) {
  float f = u2f(uf);
  float tf = 2*f;
  if (isnan(f))
    return uf;
  else
    return f2u(tf);
}
