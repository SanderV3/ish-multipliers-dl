# Multipliers: C LUT generation

This is the original C pipeline. It composes each 8-bit approximate multiplier
from small building blocks (2-bit primitives → 4-bit blocks → 8-bit multiplier)
and writes it out as a 256×256 lookup table that AdaPT and TransAxx use in place
of an exact multiplier.

In normal use you don't run this by hand. The `0_build_multipliers` notebook
builds every LUT, cross-checks it against this C code, and the generated headers
are already committed under `docker/luts/`. This README is for building or
inspecting the headers straight from the C, outside the notebook.

All the commands below run from this `multipliers/` directory. You need a C
compiler (`gcc` or `clang`) and nothing else.

## Build every LUT

```bash
bash generate/build_all.sh
```

That compiles the generator and writes the exact baseline plus `FPGA_ISH_1..5`
for both frameworks into `luts/adapt/` and `luts/transaxx/`. The file names are
exactly what AdaPT / TransAxx expect.

## Build a single LUT

```bash
# compile the generator. generator.c links against the composition code,
# so compose.c and specs.c go on the command line too
gcc -O2 -Wall -Wextra -o generate/generate \
    generate/generator.c compose/compose.c compose/specs.c

# generate <spec> for <framework>, where <framework> is `adapt` or `transaxx`
./generate/generate FPGA_ISH_1 adapt > luts/adapt/mul8s_fpga_ish_1.h
```

The spec names are the ones defined in `compose/specs.c`: `exact` and
`FPGA_ISH_1` through `FPGA_ISH_5`.

## Layout

- `primitives/primitives.h` holds the 4×4 truth tables the multipliers are built
  from.
- `compose/` holds `compose.{c,h}` (the 4-bit and 8-bit composition, including the
  sign-magnitude signed multiply) and `specs.{c,h}` (the named blocks and the five
  8-bit specs).
- `generate/` holds `generator.c` (writes one LUT header to stdout) and
  `build_all.sh` (the batch script above).
- `tests/` holds the Criterion unit tests for the primitives and the composition.
  They need the [Criterion](https://github.com/Snaipe/Criterion) test library
  installed, then e.g.
  `gcc -Wall -Wextra -o tests/test_primitives tests/test_primitives.c $(pkg-config --cflags --libs criterion) && ./tests/test_primitives`.

`luts/` is created on demand and is gitignored. The committed copies live under
`docker/luts/`.
