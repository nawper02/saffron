// png2bmp.c
#include <stdio.h>
#include <stdlib.h>

// Define the implementations before including the headers
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input.png> <output.bmp>\n", argv[0]);
        printf("Example: %s crate.png crate.bmp\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    int width, height, original_channels;
    
    // We pass '3' as the last argument to force stb_image to output 3 channels (RGB).
    // This strips out the alpha channel if it exists, leaving you with a standard 
    // 24-bit image, which is exactly what you want for a simple custom BMP loader.
    unsigned char *img_data = stbi_load(input_file, &width, &height, &original_channels, 3);

    if (img_data == NULL) {
        printf("Error: Could not load image '%s'\n", input_file);
        printf("Reason: %s\n", stbi_failure_reason());
        return 1;
    }

    printf("Loaded %s: %dx%d (Channels: %d, Forced to: 3)\n", input_file, width, height, original_channels);

    // Write the raw pixel data out as a BMP
    int success = stbi_write_bmp(output_file, width, height, 3, img_data);

    if (!success) {
        printf("Error: Failed to write BMP to '%s'\n", output_file);
        stbi_image_free(img_data);
        return 1;
    }

    printf("Success! Saved as %s\n", output_file);

    // Clean up
    stbi_image_free(img_data);
    return 0;
}
