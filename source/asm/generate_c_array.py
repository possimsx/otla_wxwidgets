#!/usr/bin/env python3
import sys
import os

# Try to import clipboard support
try:
    import pyperclip
    CLIPBOARD_AVAILABLE = True
except ImportError:
    CLIPBOARD_AVAILABLE = False


def bin_to_c_array(filename, array_name):
    # Read file
    with open(filename, "rb") as f:
        data = f.read()

    size = len(data)

    # Convert bytes to "0xXX" format
    hex_bytes = [f"0x{b:02X}" for b in data]

    # Wrap every 10 bytes per line
    lines = []
    for i in range(0, len(hex_bytes), 10):
        lines.append("    " + " , ".join(hex_bytes[i:i+10]) + " ,")

    # Remove trailing comma in the last line
    if lines:
        lines[-1] = lines[-1].rstrip(" ,")

    # Build final C array
    output = []
    output.append(f"unsigned char {array_name} [{size}] = {{")
    output.append("\n".join(lines))
    output.append("};\n")

    return "\n".join(output)


def main():
    if len(sys.argv) < 2:
        print("Usage: generate_c_array.py <binfile> [array_name] [output.txt]")
        sys.exit(1)

    binfile = sys.argv[1]

    # If no array_name is given → use filename without extension
    if len(sys.argv) >= 3:
        array_name = sys.argv[2]
    else:
        array_name = os.path.splitext(os.path.basename(binfile))[0]

    outfile = sys.argv[3] if len(sys.argv) >= 4 else None

    result = bin_to_c_array(binfile, array_name)

    # Always print to stdout
    print(result)

    # Write to file if provided
    if outfile:
        with open(outfile, "w") as f:
            f.write(result)
        print(f"[OK] Written to {outfile}")
    else:
        # No output file → copy to clipboard
        if CLIPBOARD_AVAILABLE:
            pyperclip.copy(result)
            print("[OK] Copied to clipboard")
        else:
            print("[WARNING] pyperclip is not installed → could not copy to clipboard")
            print("Install with: pip install pyperclip")


if __name__ == "__main__":
    main()
