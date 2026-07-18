#!/usr/bin/env python3
import OpenEXR
import Imath
import numpy as np
from PIL import Image
import sys
import os

def exr_to_png(exr_path, png_path):
    """Convert EXR file to PNG with tone mapping."""
    # Open EXR file
    exr_file = OpenEXR.InputFile(exr_path)
    header = exr_file.header()
    
    # Get image dimensions
    dw = header['dataWindow']
    width = dw.max.x - dw.min.x + 1
    height = dw.max.y - dw.min.y + 1
    
    # Read RGB channels
    pt = Imath.PixelType(Imath.PixelType.FLOAT)
    r_data = np.frombuffer(exr_file.channel('R', pt), dtype=np.float32)
    g_data = np.frombuffer(exr_file.channel('G', pt), dtype=np.float32)
    b_data = np.frombuffer(exr_file.channel('B', pt), dtype=np.float32)
    
    # Reshape to 2D arrays
    r = r_data.reshape((height, width))
    g = g_data.reshape((height, width))
    b = b_data.reshape((height, width))
    
    # Stack into RGB image
    rgb = np.stack([r, g, b], axis=-1)
    
    # Simple tone mapping (Reinhard)
    rgb = rgb / (1.0 + rgb)
    
    # Gamma correction
    rgb = np.power(rgb, 1.0 / 2.2)
    
    # Convert to 8-bit
    rgb = np.clip(rgb * 255.0, 0, 255).astype(np.uint8)
    
    # Create PIL Image and save
    img = Image.fromarray(rgb, 'RGB')
    img.save(png_path)
    
    print(f"Converted {exr_path} -> {png_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: exr2png.py <input.exr> <output.png>")
        sys.exit(1)
    
    exr_to_png(sys.argv[1], sys.argv[2])
