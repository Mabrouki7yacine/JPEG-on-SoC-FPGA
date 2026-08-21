#!/usr/bin/env python3

import sys
import struct
from pathlib import Path
from PIL import Image

WIDTH  = 480
HEIGHT = 640


def write_topdown_bmp(img: Image.Image, output_path: Path):
    """
    Write a 24-bit uncompressed BMP with:
    - BGR pixel order
    - top-down storage (negative height)
    - row padding to 4-byte boundary
    """

    img = img.convert("RGB")
    width, height = img.size

    row_raw_size = width * 3
    row_padded_size = (row_raw_size + 3) & ~3
    padding_size = row_padded_size - row_raw_size
    image_size = row_padded_size * height
    file_size = 14 + 40 + image_size
    pixel_offset = 14 + 40

    with open(output_path, "wb") as f:
        # --------------------------------------------------
        # BMP FILE HEADER (14 bytes)
        # --------------------------------------------------
        f.write(b"BM")                               # Signature
        f.write(struct.pack("<I", file_size))        # File size
        f.write(struct.pack("<H", 0))                # Reserved1
        f.write(struct.pack("<H", 0))                # Reserved2
        f.write(struct.pack("<I", pixel_offset))     # Pixel data offset

        # --------------------------------------------------
        # DIB HEADER = BITMAPINFOHEADER (40 bytes)
        # --------------------------------------------------
        f.write(struct.pack("<I", 40))               # Header size
        f.write(struct.pack("<i", width))            # Width
        f.write(struct.pack("<i", -height))          # NEGATIVE => top-down BMP
        f.write(struct.pack("<H", 1))                # Planes
        f.write(struct.pack("<H", 24))               # Bits per pixel
        f.write(struct.pack("<I", 0))                # Compression = BI_RGB
        f.write(struct.pack("<I", image_size))       # Image size
        f.write(struct.pack("<i", 0))                # X pixels per meter
        f.write(struct.pack("<i", 0))                # Y pixels per meter
        f.write(struct.pack("<I", 0))                # Colors used
        f.write(struct.pack("<I", 0))                # Important colors

        # --------------------------------------------------
        # PIXEL DATA
        # BMP 24-bit expects B, G, R
        # top-down => write rows from top to bottom
        # --------------------------------------------------
        pixels = img.load()
        padding = b"\x00" * padding_size

        for y in range(height):
            row = bytearray()
            for x in range(width):
                r, g, b = pixels[x, y]
                row.extend([b, g, r])  # BGR
            row.extend(padding)
            f.write(row)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_image> <output.bmp>")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])

    img = Image.open(input_path).convert("RGB")
    print(f"Original size: {img.width}x{img.height}")

    # exact resize to 640x480
    img = img.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)

    print(f"Output size  : {img.width}x{img.height}")

    write_topdown_bmp(img, output_path)

    print(f"BMP written  : {output_path}")
    print("Format       : 24-bit BMP, uncompressed, top-down")


if __name__ == "__main__":
    main()
