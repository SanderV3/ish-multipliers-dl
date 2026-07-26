#ifndef COMPOSE_H
#define COMPOSE_H

#include <stdint.h>
#include "../primitives/primitives.h"

// 4x4 array with data type uint8_t, where each table is one of the primitives.
typedef const uint8_t prim_table_t[4][4];

// This is the recipie for a 4-bit multiplier, built using the primitives.
typedef struct {
  prim_table_t *HH;
  prim_table_t *HL;
  prim_table_t *LH;
  prim_table_t *LL;
} primitive_spec_t;

// Similarily this it's the recipie for an 8-bit multiplier, built using the 4-bit multipliers.
typedef struct {
  const primitive_spec_t *HH;
  const primitive_spec_t *HL;
  const primitive_spec_t *LH;
  const primitive_spec_t *LL;
} composed_spec_t;

uint8_t mul_4bit(uint8_t a, uint8_t b, primitive_spec_t spec);
int32_t mul_8bit(uint8_t a, uint8_t b, composed_spec_t spec);
int16_t mul_8bit_signed(int8_t a, int8_t b, composed_spec_t spec);

#endif
