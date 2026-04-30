#!/usr/bin/env python3
"""
bin_to_header.py — Convert a binary file into a C header containing the
data as a const uint8_t array.

Usage:
    python3 bin_to_header.py --input zephyr.bin \
                             --output flpr_firmware.h \
                             --array-name flpr_firmware

The generated header exposes:
    static const uint8_t <array_name>[];
    #define <ARRAY_NAME>_SIZE  <byte count>U
"""

import argparse
import os
import sys
from datetime import datetime, timezone

def bin_to_header(
    input_path: str,
    output_path: str,
    array_name: str,
    bytes_per_line: int = 12,
) -> None:
    """Read *input_path* and write a C header to *output_path*."""

    # ---- Read input --------------------------------------------------------
    try:
        with open(input_path, "rb") as fh:
            data = fh.read()
    except FileNotFoundError:
        print(f"error: input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)
    except OSError as exc:
        print(f"error: cannot read '{input_path}': {exc}", file=sys.stderr)
        sys.exit(1)

    size = len(data)

    # ---- Derive header-guard symbol ----------------------------------------
    basename = os.path.basename(output_path)
    guard = basename.upper().replace(".", "_").replace("-", "_")

    # ---- Create parent directories if needed --------------------------------
    parent = os.path.dirname(output_path)
    if parent:
        os.makedirs(parent, exist_ok=True)

    # ---- Write header -------------------------------------------------------
    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")

    try:
        with open(output_path, "w") as fh:
            fh.write(
                f"/* Auto-generated — do not edit.\n"
                f" * Source : {os.path.basename(input_path)}\n"
                f" * Size   : {size} bytes\n"
                f" * Date   : {timestamp}\n"
                f" */\n\n"
                f"#ifndef {guard}\n"
                f"#define {guard}\n\n"
                f"#include <stdint.h>\n\n"
                f"/** Number of bytes in :c:data:`{array_name}`. */\n"
                f"#define {array_name.upper()}_SIZE {size}U\n\n"
                f"/** FLPR firmware image, ready to be copied to FLPR SRAM. */\n"
                f"static const uint8_t {array_name}[] = {{\n"
            )

            for i, byte in enumerate(data):
                # Indent at the start of each row.
                if i % bytes_per_line == 0:
                    fh.write("    ")

                fh.write(f"0x{byte:02x}")

                if i < size - 1:
                    fh.write(", ")

                # Newline after the last byte on a row (but not after the
                # very last byte — that gets a newline below).
                if i < size - 1 and (i + 1) % bytes_per_line == 0:
                    fh.write("\n")

            # Close the array initialiser.
            fh.write(
                f"\n}};\n\n"
                f"#endif /* {guard} */\n"
            )

    except OSError as exc:
        print(f"error: cannot write '{output_path}': {exc}", file=sys.stderr)
        sys.exit(1)

    print(
        f"bin_to_header: '{input_path}' ({size} B)"
        f"  →  '{output_path}'  (array: {array_name}[{size}])"
    )

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--input", "-i",
        required=True,
        metavar="FILE",
        help="Input binary file (e.g. zephyr.bin).",
    )
    p.add_argument(
        "--output", "-o",
        required=True,
        metavar="FILE",
        help="Output C header file path.",
    )
    p.add_argument(
        "--array-name", "-n",
        default="firmware",
        metavar="NAME",
        help="C identifier used for the array (default: firmware).",
    )
    p.add_argument(
        "--bytes-per-line",
        type=int,
        default=12,
        metavar="N",
        help="Hex bytes per source line (default: 12).",
    )
    return p


def main() -> None:
    args = _build_parser().parse_args()

    if args.bytes_per_line < 1:
        print("error: --bytes-per-line must be >= 1", file=sys.stderr)
        sys.exit(1)

    bin_to_header(
        input_path=args.input,
        output_path=args.output,
        array_name=args.array_name,
        bytes_per_line=args.bytes_per_line,
    )


if __name__ == "__main__":
    main()