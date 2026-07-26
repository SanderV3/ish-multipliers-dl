#include <criterion/criterion.h>
#include <stdint.h>

#include "../primitives/primitives.h"

static double mae(const uint8_t prim[4][4]) {
  int total = 0;
  for (int a = 0; a < 4; a++) {
    for (int b = 0; b < 4; b++) {
      int error = (int)prim[a][b] - (a * b);
      total += (error < 0) ? -error : error;
    }
  }
  return total / 16.0;
}

static int error_count(const uint8_t prim[4][4]) {
  int count = 0;
  for (int a = 0; a < 4; a++) {
    for (int b = 0; b < 4; b++) {
      if ((int)prim[a][b] != a * b) count++;      
    }
  }
  return count;
}

static int error_magnitude(const uint8_t prim[4][4]) {
  int magnitude = 0;
  for (int a = 0; a < 4; a++) {
    for (int b = 0; b < 4; b++) {
      int diff = (int)prim[a][b] - (a * b);
      int abs_diff = (diff < 0) ? -diff : diff;
      if (abs_diff > magnitude) magnitude = abs_diff; 
    }
  }
  return magnitude;
}

// The m5 exact primitive must ofcourse be equal to a*b for all entries, and have a zero MAE and zero errors. 
Test(primitives, m5_is_correct) {
  for (int a = 0; a < 4; a++) {
    for (int b = 0; b < 4; b++) {
      cr_assert_eq(prim_m5[a][b], a * b,
        "prim_m5[%d][%d] = %u, expected %d",
        a, b, prim_m5[a][b], a * b);
    }
  }
  cr_assert_float_eq(mae(prim_m5), 0.0, 1e-9,
    "m5 MAE must be 0, got %f", mae(prim_m5));
  cr_assert_eq(error_count(prim_m5), 0,
    "m5 must have 0 errors, got %d", error_count(prim_m5));
}

Test(primitives, prim_m1_metrics) {
  const double EXPECTED_MAE = 0.125;
  const int EXPECTED_ERR_COUNT = 1;
  const int EXPECTED_ERR_MAGNITUDE = 2;

  cr_assert_float_eq(mae(prim_m1), EXPECTED_MAE, 0.001,
    "prim_m1 MAE = %f, expected %f", mae(prim_m1), EXPECTED_MAE);

  cr_assert_eq(error_count(prim_m1), EXPECTED_ERR_COUNT,
    "prim_m1 has %d errors, expected %d",
    error_count(prim_m1), EXPECTED_ERR_COUNT);

  cr_assert_eq(error_magnitude(prim_m1), EXPECTED_ERR_MAGNITUDE,
    "prim_m1 has %d errors, expected %d",
    error_magnitude(prim_m1), EXPECTED_ERR_MAGNITUDE);

  for (int a = 0; a < 4; a++) {
        for (int b = 0; b < 4; b++) {
            cr_assert_leq(prim_m1[a][b], 15,
                "prim_1[%d][%d] = %u exceeds 4-bit range",
                a, b, prim_m1[a][b]);
        }
    }
}

Test(primitives, prim_m2_metrics) {
  const double EXPECTED_MAE = 0.1875;
  const int EXPECTED_ERR_COUNT = 3;
  const int EXPECTED_ERR_MAGNITUDE = 1;

  cr_assert_float_eq(mae(prim_m2), EXPECTED_MAE, 0.001,
    "prim_m2 MAE = %f, expected %f", mae(prim_m2), EXPECTED_MAE);

  cr_assert_eq(error_count(prim_m2), EXPECTED_ERR_COUNT,
    "prim_m2 has %d errors, expected %d",
    error_count(prim_m2), EXPECTED_ERR_COUNT);

  cr_assert_eq(error_magnitude(prim_m2), EXPECTED_ERR_MAGNITUDE,
    "prim_m2 has %d errors, expected %d",
    error_magnitude(prim_m2), EXPECTED_ERR_MAGNITUDE);

  for (int a = 0; a < 4; a++) {
        for (int b = 0; b < 4; b++) {
            cr_assert_leq(prim_m2[a][b], 15,
                "prim_1[%d][%d] = %u exceeds 4-bit range",
                a, b, prim_m2[a][b]);
        }
    }
}

Test(primitives, prim_m3_metrics) {
  const double EXPECTED_MAE = 0.125;
  const int EXPECTED_ERR_COUNT = 1;
  const int EXPECTED_ERR_MAGNITUDE = 2;

  cr_assert_float_eq(mae(prim_m3), EXPECTED_MAE, 0.001,
    "prim_m3 MAE = %f, expected %f", mae(prim_m3), EXPECTED_MAE);

  cr_assert_eq(error_count(prim_m3), EXPECTED_ERR_COUNT,
    "prim_m3 has %d errors, expected %d",
    error_count(prim_m3), EXPECTED_ERR_COUNT);

  cr_assert_eq(error_magnitude(prim_m3), EXPECTED_ERR_MAGNITUDE,
    "prim_m3 has %d errors, expected %d",
    error_magnitude(prim_m3), EXPECTED_ERR_MAGNITUDE);

  for (int a = 0; a < 4; a++) {
        for (int b = 0; b < 4; b++) {
            cr_assert_leq(prim_m3[a][b], 15,
                "prim_1[%d][%d] = %u exceeds 4-bit range",
                a, b, prim_m3[a][b]);
        }
    }
}

Test(primitives, prim_m4_metrics) {
  const double EXPECTED_MAE = 0.25;
  const int EXPECTED_ERR_COUNT = 1;
  const int EXPECTED_ERR_MAGNITUDE = 4;

  cr_assert_float_eq(mae(prim_m4), EXPECTED_MAE, 0.001,
    "prim_m4 MAE = %f, expected %f", mae(prim_m4), EXPECTED_MAE);

  cr_assert_eq(error_count(prim_m4), EXPECTED_ERR_COUNT,
    "prim_m4 has %d errors, expected %d",
    error_count(prim_m4), EXPECTED_ERR_COUNT);

  cr_assert_eq(error_magnitude(prim_m4), EXPECTED_ERR_MAGNITUDE,
    "prim_m4 has %d errors, expected %d",
    error_magnitude(prim_m4), EXPECTED_ERR_MAGNITUDE);

  for (int a = 0; a < 4; a++) {
        for (int b = 0; b < 4; b++) {
            cr_assert_leq(prim_m4[a][b], 15,
                "prim_1[%d][%d] = %u exceeds 4-bit range",
                a, b, prim_m4[a][b]);
        }
    }
}
