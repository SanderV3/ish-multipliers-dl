#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <stdint.h>

// for a x b the rows represent a, and columns represent b.

// Exact 2-bit multiplier.
static const uint8_t prim_m5[4][4] = {
  {0, 0, 0, 0},
  {0, 1, 2, 3},
  {0, 2, 4, 6},
  {0, 3, 6, 9},
};

// M1 - 3x3  = 7
// Has an error rate of 1/16.
// Has an error magnitude of 2.
// Has an mae of 0.125.
static const uint8_t prim_m1[4][4] = {
  {0, 0, 0, 0},
  {0, 1, 2, 3},
  {0, 2, 4, 6},
  {0, 3, 6, 7},
};

// M2 - 1x1=0, 1x3=2, 3x1=2
// Has an error rate of 3/16.
// Has an error magnitude of 1.
static const uint8_t prim_m2[4][4] = {
  {0, 0, 0, 0},
  {0, 0, 2, 2},
  {0, 2, 4, 6},
  {0, 2, 6, 9},
};

// M3 - 3x3=11
// Has an error rate of 1/16.
// Has an error magnitude of 2.
static const uint8_t prim_m3[4][4] = {
  {0, 0, 0, 0},
  {0, 1, 2, 3},
  {0, 2, 4, 6},
  {0, 3, 6, 11},
};

// M4 - 3x3=5
// Has an error rate of 1/16.
// Has an error magnitude of 4.
static const uint8_t prim_m4[4][4] = {
  {0, 0, 0, 0},
  {0, 1, 2, 3},
  {0, 2, 4, 6},
  {0, 3, 6, 5},
};

#endif
