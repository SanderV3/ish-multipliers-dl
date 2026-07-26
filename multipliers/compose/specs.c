#include "specs.h"
#include "../primitives/primitives.h"
#include "compose.h"

// 4-bit blocks composed from 2 bit primitives

// exact specs
const primitive_spec_t exact_block = {
  .HH = &prim_m5, .HL = &prim_m5, .LH = &prim_m5, .LL = &prim_m5,
};
const composed_spec_t exact_spec = {
  .HH = &exact_block, .HL = &exact_block, .LH = &exact_block, .LL = &exact_block,
};

// area-optimized
const primitive_spec_t fpga_block_5421 = {
  .HH = &prim_m5, .HL = &prim_m4, .LH = &prim_m2, .LL = &prim_m1,
};

const primitive_spec_t fpga_block_3511 = {
  .HH = &prim_m3, .HL = &prim_m5, .LH = &prim_m1, .LL = &prim_m1,
};

const primitive_spec_t fpga_block_3311 = {
  .HH = &prim_m3, .HL = &prim_m3, .LH = &prim_m1, .LL = &prim_m1,
};

const primitive_spec_t fpga_block_5155 = {
  .HH = &prim_m5, .HL = &prim_m1, .LH = &prim_m5, .LL = &prim_m5,
};

// power-optimized
const primitive_spec_t fpga_block_1311 = {
  .HH = &prim_m1, .HL = &prim_m3, .LH = &prim_m1, .LL = &prim_m1,
};

const primitive_spec_t fpga_block_1315 = {
  .HH = &prim_m1, .HL = &prim_m3, .LH = &prim_m1, .LL = &prim_m5,
};

const primitive_spec_t fpga_block_4335 = {
  .HH = &prim_m4, .HL = &prim_m3, .LH = &prim_m3, .LL = &prim_m5,
};

const primitive_spec_t fpga_block_1555 = {
  .HH = &prim_m1, .HL = &prim_m5, .LH = &prim_m5, .LL = &prim_m5,
};

// 8-bit blocks composed from 4-bit blocks
const composed_spec_t fpga_ish1 = {
  .HH = &fpga_block_1311,
  .HL = &fpga_block_1311,
  .LH = &fpga_block_1311,
  .LL = &fpga_block_1311,
};

const composed_spec_t fpga_ish2 = {
  .HH = &fpga_block_1315,
  .HL = &fpga_block_1311,
  .LH = &fpga_block_1311,
  .LL = &fpga_block_1311,
};

const composed_spec_t fpga_ish3 = {
  .HH = &fpga_block_4335,
  .HL = &fpga_block_1315,
  .LH = &fpga_block_1311,
  .LL = &fpga_block_1311,
};

const composed_spec_t fpga_ish4 = {
  .HH = &fpga_block_4335,
  .HL = &fpga_block_1315,
  .LH = &fpga_block_1315,
  .LL = &fpga_block_1311,
};

const composed_spec_t fpga_ish5 = {
  .HH = &fpga_block_4335,
  .HL = &fpga_block_1315,
  .LH = &fpga_block_1315,
  .LL = &fpga_block_1315,
};

