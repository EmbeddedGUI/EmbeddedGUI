#!/usr/bin/env python3
"""
Shadow Alpha LUT Generator for EmbeddedGUI

Generates a Gaussian falloff lookup table for shadow rendering.
The LUT maps normalized distance (0 = inner edge, N-1 = outer edge)
to alpha values (255 = fully opaque, 0 = fully transparent).

Usage:
    python shadow_lut_gen.py [-s SIZE]

Output:
    Prints C array definition to stdout.
"""

import math
import argparse


def generate_gaussian_lut(size):
    """Generate Gaussian falloff LUT values.

    Uses sigma = size/3 so that at 3*sigma the alpha approaches 0.
    """
    values = []
    for i in range(size):
        # Normalize to 0..3 (3-sigma range)
        x = i * 3.0 / (size - 1)
        alpha = int(round(255 * math.exp(-0.5 * x * x)))
        alpha = max(0, min(255, alpha))
        values.append(alpha)
    return values


def print_c_array(values, name="egui_shadow_alpha_lut"):
    """Print values as a C const array."""
    size = len(values)
    print(f"// Gaussian falloff LUT ({size} bytes)")
    print(f"// Formula: lut[i] = 255 * exp(-0.5 * (i * 3.0 / {size - 1})^2)")
    print(f"static const uint8_t {name}[{size}] =")
    print("{")
    for i in range(0, size, 8):
        row = values[i:i + 8]
        line = ", ".join("0x%02x" % v for v in row)
        if i + 8 < size:
            print(f"    {line},")
        else:
            print(f"    {line},")
    print("};")


def main():
    parser = argparse.ArgumentParser(description="Shadow Alpha LUT Generator")
    parser.add_argument("-s", "--size", type=int, default=64,
                        help="LUT size in bytes (default: 64)")
    args = parser.parse_args()

    values = generate_gaussian_lut(args.size)
    print_c_array(values)

    # Print statistics
    print(f"\n// Statistics: min={min(values)}, max={max(values)}, "
          f"size={len(values)} bytes")


if __name__ == "__main__":
    main()
