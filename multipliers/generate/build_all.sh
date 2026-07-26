#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

# Ensure the binary is built.
gcc -O2 -Wall -Wextra -o generate generator.c ../compose/compose.c ../compose/specs.c

# List of variants to generate.
variants=(
    exact
    FPGA_ISH_1
    FPGA_ISH_2
    FPGA_ISH_3
    FPGA_ISH_4
    FPGA_ISH_5
)

# Generate each variant for each target framework.
# Outputs land in ../luts/<target>/mul8s_<name_lower>.h so the file
# names match what AdaPT / TransAxx expect directly — deploy with a
# straight `cp ../luts/adapt/*.h <framework>/.../axx_mults/`.
targets=(adapt transaxx)

for target in "${targets[@]}"; do
    mkdir -p "../luts/${target}"
    for v in "${variants[@]}"; do
        name_lower="$(echo "$v" | tr '[:upper:]' '[:lower:]')"
        output="../luts/${target}/mul8s_${name_lower}.h"
        echo "generating ${output}..."
        ./generate "$v" "$target" > "$output"
    done
done

echo "done. ${#variants[@]} LUTs per target in ../luts/{adapt,transaxx}/"
