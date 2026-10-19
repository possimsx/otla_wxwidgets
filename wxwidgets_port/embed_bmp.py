#!/usr/bin/env python3
"""
Script to embed BMP files as binary data in C++ source files.
This generates embedded_bmp_resources.h and embedded_bmp_resources.cpp
"""

import os
import sys

def to_cpp_variable_name(name):
    """Convert a name to a valid C++ variable identifier (lowercase)."""
    # If name starts with a digit, prefix it with "bmp"
    if name and name[0].isdigit():
        name = "bmp" + name
    return name

def to_cpp_function_name(name):
    """Convert a name to a valid C++ function identifier (capitalized)."""
    # If name starts with a digit, prefix it with "bmp"
    if name and name[0].isdigit():
        name = "bmp" + name
    # Capitalize first letter for function names
    if name:
        return name[0].upper() + name[1:] if len(name) > 1 else name.upper()
    return name

def generate_embedded_resources(bmp_dir, output_dir):
    """Generate C++ header and source files with embedded BMP data."""
    
    # BMP files to embed (in order: ZX Spectrum, CPC, MSX, ZX81, OTLA logo)
    bmp_files = [
        ("zxspectrum.bmp", "zxspectrum"),
        ("amstred.bmp", "amstred"),
        ("msx.bmp", "msx"),
        ("zx81.bmp", "zx81"),
        ("81.bmp", "81"),  # Alternative name for ZX81
        ("otla_logo.bmp", "otla_logo"),
    ]
    
    header_path = os.path.join(output_dir, "embedded_bmp_resources.h")
    source_path = os.path.join(output_dir, "embedded_bmp_resources.cpp")
    
    # Generate header file
    with open(header_path, 'w') as f:
        f.write("// Auto-generated file - DO NOT EDIT MANUALLY\n")
        f.write("// This file contains embedded BMP resources\n")
        f.write("#ifndef EMBEDDED_BMP_RESOURCES_H\n")
        f.write("#define EMBEDDED_BMP_RESOURCES_H\n\n")
        f.write("#include <wx/wx.h>\n")
        f.write("#include <wx/mstream.h>\n\n")
        
        for bmp_file, bmp_name in bmp_files:
            bmp_path = os.path.join(bmp_dir, bmp_file)
            if os.path.exists(bmp_path):
                var_name = to_cpp_variable_name(bmp_name)
                func_name = to_cpp_function_name(bmp_name)
                f.write(f"extern const unsigned char {var_name}_bmp_data[];\n")
                f.write(f"extern const size_t {var_name}_bmp_size;\n")
                f.write(f"wxImage Load{func_name}Bmp();\n\n")
        
        f.write("#endif // EMBEDDED_BMP_RESOURCES_H\n")
    
    # Generate source file
    with open(source_path, 'w') as f:
        f.write("// Auto-generated file - DO NOT EDIT MANUALLY\n")
        f.write("// This file contains embedded BMP resources\n\n")
        f.write('#include "embedded_bmp_resources.h"\n\n')
        
        for bmp_file, bmp_name in bmp_files:
            bmp_path = os.path.join(bmp_dir, bmp_file)
            if os.path.exists(bmp_path):
                # Read binary data
                with open(bmp_path, 'rb') as bmp:
                    data = bmp.read()
                
                var_name = to_cpp_variable_name(bmp_name)
                func_name = to_cpp_function_name(bmp_name)
                
                # Write array definition
                f.write(f"const unsigned char {var_name}_bmp_data[] = {{\n")
                
                # Write data in hex format (16 bytes per line)
                for i in range(0, len(data), 16):
                    chunk = data[i:i+16]
                    hex_values = ', '.join(f'0x{b:02x}' for b in chunk)
                    f.write(f"    {hex_values}")
                    if i + 16 < len(data):
                        f.write(",")
                    f.write("\n")
                
                f.write("};\n\n")
                f.write(f"const size_t {var_name}_bmp_size = {len(data)};\n\n")
                
                # Write loader function
                f.write(f"wxImage Load{func_name}Bmp()\n")
                f.write("{\n")
                f.write(f"    wxMemoryInputStream memStream({var_name}_bmp_data, {var_name}_bmp_size);\n")
                f.write("    wxImage img;\n")
                f.write("    if (img.LoadFile(memStream, wxBITMAP_TYPE_BMP))\n")
                f.write("        return img;\n")
                f.write("    return wxImage();\n")
                f.write("}\n\n")
    
    print(f"Generated {header_path}")
    print(f"Generated {source_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: embed_bmp.py <bmp_directory> <output_directory>")
        sys.exit(1)
    
    bmp_dir = sys.argv[1]
    output_dir = sys.argv[2]
    
    if not os.path.isdir(bmp_dir):
        print(f"Error: BMP directory not found: {bmp_dir}")
        sys.exit(1)
    
    os.makedirs(output_dir, exist_ok=True)
    generate_embedded_resources(bmp_dir, output_dir)

