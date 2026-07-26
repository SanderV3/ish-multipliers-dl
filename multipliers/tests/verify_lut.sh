#!/usr/bin/env bash
  # Compare a generated LUT against a reference, ignoring formatting differences.
  # Extracts all signed integers from each file and diffs the streams.
  #
  # Usage:
  #   ./verify_lut.sh                              # defaults: exact vs mul8s_acc.h
  #   ./verify_lut.sh <generated.h> <reference.h>  # explicit

  set -euo pipefail
  cd "$(dirname "$0")"

  GENERATED="${1:-../luts/mul8s_exact.h}"
  REFERENCE="${2:-../../adapt/adapt/cpu-kernels/axx_mults/mul8s_acc.h}"

  if [ ! -f "$GENERATED" ]; then echo "missing: $GENERATED"; exit 1; fi
  if [ ! -f "$REFERENCE" ]; then echo "missing: $REFERENCE"; exit 1; fi

  if diff -q \
       <(grep -oE -- '-?[0-9]+' "$GENERATED") \
       <(grep -oE -- '-?[0-9]+' "$REFERENCE") > /dev/null
  then
    echo "PASS: $GENERATED matches $REFERENCE (numerical content)"
  else
    echo "FAIL: differ. First 20 lines of diff:"
    diff <(grep -oE -- '-?[0-9]+' "$GENERATED") \
         <(grep -oE -- '-?[0-9]+' "$REFERENCE") | head -20
    exit 1
  fi
