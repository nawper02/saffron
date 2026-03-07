// png2bmp.c
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char **argv) {
    if (argc < 3) return 1;

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int width, height, original_channels;
    
    // Force 4 channels so we can read the PNG's Alpha data
    unsigned char *img_data = stbi_load(input_file, &width, &height, &original_channels, 4);
    if (img_data == NULL) return 1;

    // Create a buffer for standard 3-channel RGB
    unsigned char *rgb_data = (unsigned char *)malloc(width * height * 3);

    for (int i = 0; i < width * height; i++) {
        unsigned char r = img_data[i * 4 + 0];
        unsigned char g = img_data[i * 4 + 1];
        unsigned char b = img_data[i * 4 + 2];
        unsigned char a = img_data[i * 4 + 3];

        if (a < 128) {
            // If transparent, paint it Magic Pink (Magenta)
            rgb_data[i * 3 + 0] = 255;
            rgb_data[i * 3 + 1] = 0;
            rgb_data[i * 3 + 2] = 255;
        } else {
            // Keep original color
            rgb_data[i * 3 + 0] = r;
            rgb_data[i * 3 + 1] = g;
            rgb_data[i * 3 + 2] = b;
        }
    }

    stbi_write_bmp(output_file, width, height, 3, rgb_data);

    free(rgb_data);
    stbi_image_free(img_data);
    return 0;
}
