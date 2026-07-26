#include <criterion/criterion.h>
#include <stdint.h>

#include "../compose/compose.h"

// Exact multipliers for testing
const primitive_spec_t fpga_block_exact = {
  .HH = &prim_m5, .HL = &prim_m5, .LH = &prim_m5, .LL = &prim_m5,
};

const composed_spec_t fpga_ish_exact = {
  .HH = &fpga_block_exact,
  .HL = &fpga_block_exact,
  .LH = &fpga_block_exact,
  .LL = &fpga_block_exact,
};

Test(compose, mul_8bit_exact_matches_host_unsigned){
  for (int a = 0; a < 256; a++) {
    for (int b = 0; b < 256; b++){
      cr_assert_eq(
        mul_8bit(a, b, fpga_ish_exact), (uint16_t)a * (uint16_t)b,
        "mul_8bit(%d, %d) = %d, expected %d",
        a, b, mul_8bit(a, b, fpga_ish_exact), (uint16_t)a * (uint16_t)b);
    }
  }
};


Test(compose, mul_8bit_exact_matches_host_signed){
  for (int a = -128; a < 128; a++) {
    for (int b = -128; b < 128; b++){
      int16_t got = mul_8bit_signed((int8_t)a, (int8_t)b, fpga_ish_exact);
      int16_t want = (int16_t)a * (int16_t)b;
      cr_assert_eq(
        got, want,
        "mul_8bit_signed(%d, %d) = %d, expected %d",
        a, b, got, want);
    }
  }
};
