#!/usr/bin/env python3
"""
Converts PNG or WebP images to C arrays for ESP32 HUB75 LED Matrix
Generates a .h file with RGB565 or RGB888 format
"""

from PIL import Image
import sys
import os

def convert_image_to_c_array(image_path, output_path=None, format='RGB888', var_name=None):
    """
    Convert image to C array for HUB75 display
    
    Args:
        image_path: Path to input image (PNG, WebP, etc.)
        output_path: Path to output .h file (optional)
        format: 'RGB888' or 'RGB565'
        var_name: Variable name for the array (optional, auto-generated from filename)
    """
    
    # Load and resize image to 64x64
    try:
        img = Image.open(image_path)
        print(f"Loaded image: {image_path}")
        print(f"Original size: {img.size}")
        
        # Resize to 64x64 if needed
        if img.size != (64, 64):
            img = img.resize((64, 64), Image.Resampling.LANCZOS)
            print(f"Resized to: 64x64")
        
        # Convert to RGB if needed
        if img.mode != 'RGB':
            img = img.convert('RGB')
            print(f"Converted to RGB mode")
            
    except Exception as e:
        print(f"Error loading image: {e}")
        return False
    
    # Generate variable name from filename if not provided
    if var_name is None:
        base_name = os.path.splitext(os.path.basename(image_path))[0]
        var_name = base_name.replace('-', '_').replace(' ', '_').replace('.', '_').lower()
    
    # Generate output filename if not provided
    if output_path is None:
        output_path = f"{var_name}.h"
    
    # Get pixel data
    pixels = img.load()
    width, height = img.size
    
    # Generate C code
    with open(output_path, 'w') as f:
        # Header
        f.write(f"// Auto-generated image data for HUB75 LED Matrix\n")
        f.write(f"// Source: {os.path.basename(image_path)}\n")
        f.write(f"// Size: {width}x{height}\n")
        f.write(f"// Format: {format}\n\n")
        
        f.write(f"#ifndef _{var_name.upper()}_H_\n")
        f.write(f"#define _{var_name.upper()}_H_\n\n")
        
        f.write(f"#include <Arduino.h>\n\n")
        
        if format == 'RGB565':
            # RGB565 format (16-bit color)
            f.write(f"// RGB565 format: 5 bits red, 6 bits green, 5 bits blue\n")
            f.write(f"const uint16_t {var_name}_width = {width};\n")
            f.write(f"const uint16_t {var_name}_height = {height};\n\n")
            f.write(f"const uint16_t {var_name}_data[{height}][{width}] PROGMEM = {{\n")
            
            for y in range(height):
                f.write("  { ")
                for x in range(width):
                    r, g, b = pixels[x, y]
                    # Convert to RGB565
                    rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                    f.write(f"0x{rgb565:04X}")
                    if x < width - 1:
                        f.write(", ")
                f.write(" }")
                if y < height - 1:
                    f.write(",")
                f.write("\n")
            
            f.write("};\n\n")
            
        else:  # RGB888
            # RGB888 format (24-bit color, stored as separate R,G,B arrays)
            f.write(f"// RGB888 format: 8 bits per channel\n")
            f.write(f"const uint16_t {var_name}_width = {width};\n")
            f.write(f"const uint16_t {var_name}_height = {height};\n\n")
            
            # Red channel
            f.write(f"const uint8_t {var_name}_red[{height}][{width}] PROGMEM = {{\n")
            for y in range(height):
                f.write("  { ")
                for x in range(width):
                    r, g, b = pixels[x, y]
                    f.write(f"{r:3d}")
                    if x < width - 1:
                        f.write(", ")
                f.write(" }")
                if y < height - 1:
                    f.write(",")
                f.write("\n")
            f.write("};\n\n")
            
            # Green channel
            f.write(f"const uint8_t {var_name}_green[{height}][{width}] PROGMEM = {{\n")
            for y in range(height):
                f.write("  { ")
                for x in range(width):
                    r, g, b = pixels[x, y]
                    f.write(f"{g:3d}")
                    if x < width - 1:
                        f.write(", ")
                f.write(" }")
                if y < height - 1:
                    f.write(",")
                f.write("\n")
            f.write("};\n\n")
            
            # Blue channel
            f.write(f"const uint8_t {var_name}_blue[{height}][{width}] PROGMEM = {{\n")
            for y in range(height):
                f.write("  { ")
                for x in range(width):
                    r, g, b = pixels[x, y]
                    f.write(f"{b:3d}")
                    if x < width - 1:
                        f.write(", ")
                f.write(" }")
                if y < height - 1:
                    f.write(",")
                f.write("\n")
            f.write("};\n\n")
        
        # Helper function
        if format == 'RGB565':
            f.write(f"// Helper function to draw the image\n")
            f.write(f"void draw_{var_name}(MatrixPanel_I2S_DMA* display, int offsetX, int offsetY) {{\n")
            f.write(f"  for (int y = 0; y < {var_name}_height; y++) {{\n")
            f.write(f"    for (int x = 0; x < {var_name}_width; x++) {{\n")
            f.write(f"      uint16_t color = pgm_read_word(&{var_name}_data[y][x]);\n")
            f.write(f"      // Convert RGB565 to RGB888\n")
            f.write(f"      uint8_t r = ((color >> 11) & 0x1F) << 3;\n")
            f.write(f"      uint8_t g = ((color >> 5) & 0x3F) << 2;\n")
            f.write(f"      uint8_t b = (color & 0x1F) << 3;\n")
            f.write(f"      display->drawPixelRGB888(x + offsetX, y + offsetY, r, g, b);\n")
            f.write(f"    }}\n")
            f.write(f"  }}\n")
            f.write(f"}}\n\n")
        else:  # RGB888
            f.write(f"// Helper function to draw the image\n")
            f.write(f"void draw_{var_name}(MatrixPanel_I2S_DMA* display, int offsetX, int offsetY) {{\n")
            f.write(f"  for (int y = 0; y < {var_name}_height; y++) {{\n")
            f.write(f"    for (int x = 0; x < {var_name}_width; x++) {{\n")
            f.write(f"      uint8_t r = pgm_read_byte(&{var_name}_red[y][x]);\n")
            f.write(f"      uint8_t g = pgm_read_byte(&{var_name}_green[y][x]);\n")
            f.write(f"      uint8_t b = pgm_read_byte(&{var_name}_blue[y][x]);\n")
            f.write(f"      display->drawPixelRGB888(x + offsetX, y + offsetY, r, g, b);\n")
            f.write(f"    }}\n")
            f.write(f"  }}\n")
            f.write(f"}}\n\n")
        
        f.write(f"#endif // _{var_name.upper()}_H_\n")
    
    print(f"\n✓ Successfully generated: {output_path}")
    print(f"  Variable name: {var_name}")
    print(f"  Format: {format}")
    print(f"  Size: {width}x{height}")
    print(f"\nUsage in your Arduino code:")
    print(f'  #include "{os.path.basename(output_path)}"')
    print(f"  draw_{var_name}(dma_display, 0, 0);")
    
    return True

def main():
    if len(sys.argv) < 2:
        print("Usage: python image_to_hub75.py <image_path> [output_path] [format] [var_name]")
        print("  format: RGB888 (default) or RGB565")
        print("\nExample:")
        print("  python image_to_hub75.py fox.png")
        print("  python image_to_hub75.py fox.png fox_image.h RGB565")
        print("  python image_to_hub75.py fox.png fox_image.h RGB888 my_fox")
        sys.exit(1)
    
    image_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else None
    format = sys.argv[3] if len(sys.argv) > 3 else 'RGB888'
    var_name = sys.argv[4] if len(sys.argv) > 4 else None
    
    if format not in ['RGB888', 'RGB565']:
        print(f"Error: Invalid format '{format}'. Use RGB888 or RGB565")
        sys.exit(1)
    
    if not os.path.exists(image_path):
        print(f"Error: Image file not found: {image_path}")
        sys.exit(1)
    
    success = convert_image_to_c_array(image_path, output_path, format, var_name)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()