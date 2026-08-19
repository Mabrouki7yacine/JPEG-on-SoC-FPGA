#!/usr/bin/env python3

import sys
from pathlib import Path
from PIL import Image


WIDTH  = 480
HEIGHT = 640


def main():

    if len(sys.argv) != 3:
        sys.exit(
            f"Usage: {sys.argv[0]} <input_image> <output.h>"
        )

    input_path  = Path(sys.argv[1])
    output_path = Path(sys.argv[2])

    # --------------------------------------------------------
    # Load image
    # --------------------------------------------------------

    img = Image.open(input_path).convert("RGB")

    print(f"Original size: {img.width}x{img.height}")

    # --------------------------------------------------------
    # Resize exactly to 640x480
    # --------------------------------------------------------

    img = img.resize(
        (WIDTH, HEIGHT),
        Image.Resampling.LANCZOS
    )

    print(f"Output size: {img.width}x{img.height}")

    # Optional preview
    preview_path = output_path.with_suffix(".png")
    img.save(preview_path)

    # --------------------------------------------------------
    # Generate C RGB array
    # --------------------------------------------------------

    with open(output_path, "w") as f:

        f.write("#pragma once\n\n")
        f.write("#include <stdint.h>\n\n")

        f.write(f"#define IMAGE_WIDTH  {WIDTH}\n")
        f.write(f"#define IMAGE_HEIGHT {HEIGHT}\n")
        f.write(f"#define IMAGE_SIZE   (IMAGE_WIDTH * IMAGE_HEIGHT)\n\n")

        f.write(
            "typedef struct {\n"
            "    uint8_t R;\n"
            "    uint8_t G;\n"
            "    uint8_t B;\n"
            "} RGB;\n\n"
        )

        f.write("const RGB ImageRGB[IMAGE_SIZE] = {\n")

        pixels = img.load()

        for y in range(HEIGHT):

            f.write(f"    /* Row {y} */\n    ")

            for x in range(WIDTH):

                r, g, b = pixels[x, y]

                f.write(
                    f"{{{r:3d}, {g:3d}, {b:3d}}}"
                )

                # No comma after final pixel
                if not (
                    x == WIDTH - 1
                    and y == HEIGHT - 1
                ):
                    f.write(", ")

                # Keep generated file somewhat readable
                if (x + 1) % 8 == 0 and x != WIDTH - 1:
                    f.write("\n    ")

            f.write("\n")

        f.write("};\n")

    print(f"C array written to : {output_path}")
    print(f"Preview written to : {preview_path}")
    print(f"Pixels             : {WIDTH * HEIGHT}")


if __name__ == "__main__":
    main()
