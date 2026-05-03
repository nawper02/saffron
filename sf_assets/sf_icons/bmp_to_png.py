#!/usr/bin/env python3
"""Convert all .bmp icons to .png with transparent background.
The top-left pixel color is used as the background key color."""
import os
from PIL import Image

icon_dir = os.path.dirname(os.path.abspath(__file__))

converted = 0
for fname in sorted(os.listdir(icon_dir)):
    if not fname.lower().endswith('.bmp'):
        continue
    src = os.path.join(icon_dir, fname)
    dst = os.path.join(icon_dir, fname[:-4] + '.png')
    img = Image.open(src).convert('RGBA')
    pixels = img.load()
    w, h = img.size
    # Sample top-left pixel as background color (ignore alpha channel)
    bg = pixels[0, 0][:3]
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if (r, g, b) == bg:
                pixels[x, y] = (r, g, b, 0)
    img.save(dst, 'PNG')
    converted += 1
    print(f'  {fname} -> {fname[:-4]}.png  (bg={bg})')

print(f'\nConverted {converted} files.')
