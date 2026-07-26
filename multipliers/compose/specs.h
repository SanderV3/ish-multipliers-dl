#ifndef SPECS_H
#define SPECS_H

#include "compose.h"

extern const primitive_spec_t exact_block;
extern const composed_spec_t exact_spec;

// area-optimized 4-bit recursive multipliers
extern const primitive_spec_t fpga_block_5421;
extern const primitive_spec_t fpga_block_3511;
extern const primitive_spec_t fpga_block_3311;
extern const primitive_spec_t fpga_block_5155;

// power-optimized 4-bit recursive multipliers
extern const primitive_spec_t fpga_block_1311;
extern const primitive_spec_t fpga_block_1315;
extern const primitive_spec_t fpga_block_4335;
extern const primitive_spec_t fpga_block_1555;

// power-optimized 8-bit recursive multipliers
extern const composed_spec_t fpga_ish1;
extern const composed_spec_t fpga_ish2;
extern const composed_spec_t fpga_ish3;
extern const composed_spec_t fpga_ish4;
extern const composed_spec_t fpga_ish5;

#endif
