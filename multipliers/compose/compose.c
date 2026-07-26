#include "compose.h"
#include <stdint.h>

// splits each 4-bit operand into 2-bit halves
// indexes the appropriate primitive table to look up the partial product.
// sums the four partial products with weights.
uint8_t mul_4bit(uint8_t a, uint8_t b, primitive_spec_t spec){
  uint8_t a_h = (a >> 2) & 0x3;
  uint8_t a_l = a & 0x3;
  uint8_t b_h = (b >> 2) & 0x3;
  uint8_t b_l = b & 0x3;

  return 16 * (*spec.HH)[a_h][b_h]
        + 4 * (*spec.HL)[a_h][b_l]
        + 4 * (*spec.LH)[a_l][b_h]
        + 1 * (*spec.LL)[a_l][b_l];
}

// identical, but one level up.
int32_t mul_8bit(uint8_t a, uint8_t b, composed_spec_t spec){
  uint8_t a_h = (a >> 4) & 0xF;
  uint8_t a_l = a & 0xF;
  uint8_t b_h = (b >> 4) & 0xF;
  uint8_t b_l = b & 0xF;

  return 256 * mul_4bit(a_h, b_h, *(spec.HH))
        + 16 * mul_4bit(a_h, b_l, *(spec.HL))
        + 16 * mul_4bit(a_l, b_h, *(spec.LH))
        + 1 * mul_4bit(a_l, b_l, *(spec.LL));
}

// using signed magnitude, it takes the magnitude of the operands, calls
// the unsigned mul_8bit on those. result is negative iff exactly one of
// a or b is negative.
int16_t mul_8bit_signed(int8_t a, int8_t b, composed_spec_t spec){
  uint8_t mag_a = (a < 0) ? (uint8_t)(-(int)a) : (uint8_t)a;
  uint8_t mag_b = (b < 0) ? (uint8_t)(-(int)b) : (uint8_t)b;
  int32_t mag_prod = mul_8bit(mag_a, mag_b, spec);
  int negative = (a < 0) != (b < 0);
  return (int16_t)(negative ? -mag_prod : mag_prod);
}
