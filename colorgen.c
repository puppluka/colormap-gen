#include <stdio.h>
#include <stdlib.h> // For abs() and exit()

/*
 * ============================================================================
 * Function: convert_24_to_8
 *
 * Description:
 * Samples a 24-bit RGB value to the closest color on the provided
 * 8-bit palette. It calculates the squared Euclidean distance in RGB space
 * to find the best match.
 *
 * Parameters:
 * palette (const unsigned char[768]): The input 256-color palette.
 * rgb (const int[3]): The 24-bit RGB color to convert.
 *
 * Returns:
 * The 8-bit index of the best-matching color in the palette.
 * ============================================================================
 */
unsigned char convert_24_to_8(const unsigned char palette[768], const int rgb[3]) {
    int i, j;
    int best_index = -1;
    int best_dist = 0;

    for (i = 0; i < 256; i++) {
        int dist = 0;
        for (j = 0; j < 3; j++) {
            /* Note: We could use RGB luminosity bias for greater accuracy, but
               Quake's colormap apparently didn't do this. */
            int d = abs(rgb[j] - palette[i * 3 + j]);
            dist += d * d; // Squared distance
        }

        if (best_index == -1 || dist < best_dist) {
            best_index = i;
            best_dist = dist;
        }
    }

    return (unsigned char)best_index;
}

/*
 * ============================================================================
 * Function: generate_colormap
 *
 * Description:
 * Generates Quake's 64 levels of lighting for a given 256-color palette.
 * The final 32 colors are treated as "fullbright" and are not affected
 * by lighting.
 *
 * Parameters:
 * palette (const unsigned char[768]): The input 256-color palette.
 * out_colormap (unsigned char[16384]): The output buffer for the colormap.
 * ============================================================================
 */
void generate_colormap(const unsigned char palette[768], unsigned char out_colormap[16384]) {
    int num_fullbrights = 32; /* The last 32 colors will be full bright */
    int x, y, i;

    // A 256x64 grid: 256 palette entries, 64 light levels
    for (y = 0; y < 64; y++) {       // Light level
        for (x = 0; x < 256; x++) {  // Palette index
            // Fullbright colors are not dimmed
            if (x >= 256 - num_fullbrights) {
                out_colormap[y * 256 + x] = x;
                continue;
            }

            int rgb[3];
            for (i = 0; i < 3; i++) {
                // Dim the original palette color based on the light level 'y'
                // The formula (val * (63 - y) + 16) >> 5 is equivalent to
                // round(val * (63 - y) / 32.0), but faster.
                // It maps the full light (y=0) to a divisor of 63/32 and
                // near-darkness (y=63) to a divisor of 0/32.
                rgb[i] = (palette[x * 3 + i] * (63 - y) + 16) >> 5;

                // Clamp to a valid 8-bit range
                if (rgb[i] > 255) {
                    rgb[i] = 255;
                }
            }

            // Find the closest color in the original palette for the new dimmed color
            out_colormap[y * 256 + x] = convert_24_to_8(palette, rgb);
        }
    }
}

/*
 * ============================================================================
 * MAIN
 * ============================================================================
 */
int main(int argc, char *argv[]) {
    // --- Argument check ---
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_palette.lmp>\n", argv[0]);
        fprintf(stderr, "       Generates 'colormap.lmp' in the current directory.\n");
        return 1;
    }

    const char *palette_filename = argv[1];
    const char *colormap_filename = "colormap.lmp";

    // --- Data buffers ---
    unsigned char palette[768];
    unsigned char colormap[16384]; // 64 light levels * 256 colors

    // --- Read input palette file ---
    FILE *f_in = fopen(palette_filename, "rb");
    if (!f_in) {
        perror("Error opening input palette file");
        return 1;
    }

    size_t bytes_read = fread(palette, 1, sizeof(palette), f_in);
    fclose(f_in);

    if (bytes_read != sizeof(palette)) {
        fprintf(stderr, "Error: Input palette file is not 768 bytes long. Read %zu bytes.\n", bytes_read);
        return 1;
    }

    printf("✅ Successfully read %s (%zu bytes).\n", palette_filename, bytes_read);

    // --- Generate the colormap ---
    printf("🎨 Generating colormap...\n");
    generate_colormap(palette, colormap);

    // --- Write output colormap file ---
    FILE *f_out = fopen(colormap_filename, "wb");
    if (!f_out) {
        perror("Error opening output colormap file");
        return 1;
    }

    size_t bytes_written = fwrite(colormap, 1, sizeof(colormap), f_out);
    fclose(f_out);

    if (bytes_written != sizeof(colormap)) {
        fprintf(stderr, "Error: Failed to write all 16384 bytes to %s. Wrote %zu bytes.\n", colormap_filename, bytes_written);
        return 1;
    }

    printf("✅ Successfully wrote %s (%zu bytes).\n", colormap_filename, bytes_written);
    printf("✨ Done!\n");

    return 0;
}
