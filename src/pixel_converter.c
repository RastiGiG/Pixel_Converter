/**************************************************************************
 * PIXEL CONVERTER V 1.0 
 * ------------------------------------------------------------------------
 * Copyright (c) 2023-2024 RastiGiG <randomly.ventilates@simplelogin.co>
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 ***********************************************************************/

/* TODOS
 *
 * @todo: Add conversion ppm -> rgb888
 * @todo: Add conversion ppm -> rgb565
 * @todo: Add conversion ppm -> pgm
 * */

/* Libraries
 *
 * */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

/* Macros
 *
 * MAX_BUFF_SIZE                        -- Limits the amount of bytes for the input file
 * MAX_PIXEL_VALUE                      -- Max value a pixel can have (for netpbm formats)
 * BINARY_WRITE                         -- String representing binary write flag
 * BINARY_READ                          -- String representing binary read flag
 * PBM_MAGIC_NUMBER                     -- Magic number representing pbm file in netpbm header
 * PGM_MAGIC_NUMBER                     -- Magic number representing pgm file in netpbm header
 * PPM_MAGIC_NUMBER                     -- Magic number representing ppm file in netpbm header
 * */
#define MAX_BUFF_SIZE                1024 * 1024 
#define MAX_PIXEL_VALUE              255
#define BINARY_WRITE                 "wb"
#define BINARY_READ                  "rb"
#define PBM_MAGIC_NUMBER             "P4"
#define PGM_MAGIC_NUMBER             "P5"
#define PPM_MAGIC_NUMBER             "P6"

/* Type declarations
 *
 * usize            -- Rust inspired name for size_t type
 * u8               -- Rust inspired shorthand for uint8_t
 * u16              -- Rust inspired shorthand for uint16_t
 * u32              -- Rust inspired shorthand for uint16_t
 * color_channels   -- Object to separately store rgb color channels
 * */
typedef size_t usize;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct {
    u8 red;
    u8 green;
    u8 blue;
} color_channels;

/* Global constants
 *
 * rgb565masks      -- Masks needed for conversion to 16-bit rgb 
 * rgb565max        -- Max values for 16-bit color channels
 * rgb32masks       -- Masks needed for conversion to 32-bit rgb
 * */
enum rgb565masks {
    redMask         = 0xf8, // dec: 248, bin: 0b11111000
    greenMask       = 0xfc, // dec: 252, bin: 0b11111100 
    blueMask        = 0xf8  // dec: 248, bin: 0b11111000 
};

enum rgb565max {
    redMax          = 0x1f, // dec: 31, bin: 0b00011111
    greenMax        = 0x3f, // dec: 63, bin: 0b00111111
    blueMax         = 0x1f, // dec: 31, bin: 0b00011111
};

enum rgb32masks {
    redMask32       = redMask   * 2^16, // dec: 248 * 2^16, bin: 0b11111000 00000000 00000000
    greenMask32     = greenMask *  2^8, // dec: 252 * 2^8,  bin: 0b11111100 00000000 
    blueMask32      = blueMask          // dec: 248,        bin: 0b11111000 
};


/* Convert RGB888 to RGB565
 *
 * */
u16 rgb888_to_rgb565(color_channels rgb888pixel) {
    u16 RGB565 = 0;
    
    RGB565 = (
        ((rgb888pixel.red & redMask) << 8) + 
        ((rgb888pixel.green & greenMask) << 3) + 
        (rgb888pixel.blue >> 3)
    );

    return RGB565;
}

/* Convert RGB565 to RGB888
 *
 * */
color_channels rgb565_to_rgb888(u16 rgb16bit) {
    color_channels rgb888pixel = {0, 0, 0};
    u8 red = (u8) ((rgb16bit >> 11) & redMax);
    u8 green = (u8) ((rgb16bit >> 5)  & greenMax);
    u8 blue = (u8) (rgb16bit & blueMax);

    // Multiplication first to avoid rounding down to 0
    rgb888pixel.red = round(red * 0xff * 1.0 / redMax);
    rgb888pixel.green = round(green * 0xff * 1.0 / greenMax);
    rgb888pixel.blue = round(blue * 0xff * 1.0 / blueMax);

    return rgb888pixel;
}

/* Convert RGB565 to RGB888
 *
 * */
u16 rgb32_to_rgb565(u32 rgb32bit) {
    u16 RGB565 = 0;

    RGB565 = (
        ((rgb32bit & redMask32) >> 8) + 
        ((rgb32bit & greenMask32) >> 5) + 
        ((rgb32bit & blueMask32) >> 3));   

    return RGB565;
}

/* Convert RGB888 to Grayscale (Luminosity Method) 
 * see https://www.baeldung.com/cs/convert-rgb-to-grayscale
 *
 * Calculation:
 * Grayscale = 0.3 * Red + 0.59 * Green + 0.11 * Blue
 * */
u8 rgb888_to_grayscale(color_channels rgb888pixel){
    u8 grayscale_pixel = 0;
    
    grayscale_pixel = round(
        (rgb888pixel.red * 0.3) + 
        (rgb888pixel.green * 0.59) + 
        (rgb888pixel.blue * 0.11) 
    );

    return grayscale_pixel;
}

/* Create header for netpbm formats
 *
 * */
const char* concat_netpbm_header(char* netpbm_magic_number, u8 max_pixel_value_per_channel){
    /* Variables for Netpbm formats
    *
    * header_size          -- buffer for number of bytes in header
    * netpbm_header        -- buffer for netpbm header
    * width                -- stores image width (needed for netpbm formats)
    * height               -- stores image height (needed for netpbm formats)
    * max_pixel_value      -- stores max value per pixel and color channel (needed for netpbm formats)
    * */
    u32 width                   = 0;
    u32 height                  = 0;
    u8 max_pixel_value          = max_pixel_value_per_channel;
    usize header_size           = sizeof(char) * 2 // Netpbm Magic Number consists of 2 chars
        + sizeof(width)
        + sizeof(height)
        + sizeof(max_pixel_value)
        + 3;  // whitespace characters
    char* netpbm_header = calloc(header_size, sizeof(char));

    // Get dimensions from user, since they cannot be extracted from raw data
    printf("Enter width: ");
    scanf("%20u", &width);
    printf("Enter height: ");
    scanf("%20u", &height);
    // printf("Enter max value per color channel per pixel: ");
    // scanf("%20u", max_pixel_value);

    // Concat variables to header string
    snprintf(netpbm_header, 
             header_size, 
             "%s\n%u %u\n%u", 
             netpbm_magic_number, 
             width,
             height, 
             max_pixel_value);

    // Header needs to terminate with a single whitespace character (here: '\n')
    strcat(netpbm_header, "\n");

    return netpbm_header;
}

/* Return size of the raw image data
 *
 * */
usize find_end_of_header(char* input_string, u8 max_pixel_value){
    /* Variables
     *
     * search_string_length     -- buffer for length of search string 
     * search_string            -- buffer for search string
     * remaining_file_size      -- buffer for size of raw image data
     * */
    u32 search_string_length = 20;
    char search_string[search_string_length];
    usize remaining_file_size = 0;
    
    /* Define search string representing the end of the header file
     */
    snprintf(search_string, 
             search_string_length, 
             "%u", 
             max_pixel_value);
    strcat(search_string, "\n");
    search_string_length = strlen(search_string); // recalculate search string length

    // Get pointer to beginning of search string to calculate raw image data size
    char* p_end_of_header = strstr(input_string, search_string);
    remaining_file_size = strlen(&p_end_of_header[search_string_length]);
    return remaining_file_size;
}


/*
 * Function to handle file opening and writing
 * */
usize file_open_and_write (char *filepath, void * buffer, usize size){
    // Check file exists
    if (access(filepath, F_OK) == 0){
        fprintf(stderr, "[ERROR]: file '%s' already exists! Delete/move it or choose a different filename.", filepath);
        exit(1);
    }

    // Open file and check writability, 'wb' -> 'write binary'
    FILE *file = fopen(filepath, BINARY_WRITE);
    if(file == NULL){
        fprintf(stderr, "[ERROR]: File '%s' cannot be opened! Do you have write permissions?", filepath);
        exit(1);
    }

    // cast input to 8 bit integer to process data 1 byte at a time
    u8 *data = buffer;
    usize i;

    // Write file to memory
    for (i = 0; i < size; i++) {
        fwrite(&data[i], 1, 1, file);   //sizeof(u8),
    }

    // Check size of written file
    usize file_size;
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);

    fclose(file);
    return file_size;
}

/*
 * Function to handle file opening and reading
 * */
usize file_open_and_read (char *filepath, u8* buffer){
    // Check file exists
    if (access(filepath, F_OK) != 0){
        fprintf(stderr, "[ERROR]: File '%s' cannot be accessed! Does it exist?", filepath);
        exit(1);
    }

    // Open file and check readability
    FILE *file = fopen(filepath, "rb");
    if(file == NULL){
        fprintf(stderr, "[ERROR]: File '%s' cannot be opened! Do you have read permissions?", filepath);
        exit(1);
    }

    // Check file size
    usize file_size;
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    if(file_size <= 0){
        fprintf(stderr, "[ERROR]: File has '%zu' <= 0 bytes!\n", file_size);
        exit(1);
    }

    // Read file to memory
    fread(buffer, sizeof(char), file_size, file);
    fclose(file);

    return file_size;
}

/* Main Function
 *
 * */
int main(int argc, char *argv[])
{
    if (argc <= 3 || argc > 5){
        fprintf(stderr, "Usage: %s <input file> <output file> <input file type: 16bit|24bit|pbm|pgm|ppm> <output format: none|grayscale|rgb565|rgb888|pbm|pgm|ppm>\n", argv[0]);
        exit(0);
    }

    /* Input variables
     *
     * read_filepath        -- path to input file
     * write_filepath       -- path to output file
     * input_file_type      -- specify file type of input file (rgb565/16bit or rgb888/24bit)
     * netpbm_format        -- specify weither and which netpbm file format to use for the output
     * output_file_size     -- determines size of output file
     * b_is_grayscale       -- set grayscale output to true/false
     * */
    char *read_filepath     = argv[1];
    char *write_filepath    = argv[2];
    char *input_file_type   = argv[3];
    char* output_format     = argv[4];
    usize output_file_size  = 0;
    bool b_is_grayscale     = false;
    bool b_is_netpbm_format = false;
    
    void* input_buffer = calloc(MAX_BUFF_SIZE, sizeof(char));
    void* output_buffer = calloc(MAX_BUFF_SIZE, sizeof(u8));

    // Read the input file
    usize file_size = file_open_and_read(read_filepath, input_buffer);

    // Set grayscale as required
    if ((strcmp(output_format, "grayscale") == 0) ||
        (strcmp(output_format, "pgm") == 0)) {
        b_is_grayscale = true; 
    }
    
    // Set input to netpbm format as required
    if ((strcmp(input_file_type, "ppm") == 0)) {
        b_is_netpbm_format = true; 
    }

    /*
    * i, j              -- counter for bytes in input and output file
    * rgb565pixel       -- buffer for 16bit pixel; 
    * rgb888pixel       -- buffer for 24bit pixel;
    * grayscale_pixel   -- buffer for 8bit grayscale pixel;
    * */
    usize i, j                  = 0;
    u16 rgb565pixel             = 0;
    u8 grayscale_pixel          = 0;
    color_channels rgb888pixel  = {0, 0, 0};

    /************************************************************************************
     * Conversion from 24 bit rgb (RGB888)
     *
     * */
    if ((strcmp(input_file_type, "24bit") == 0) ||
    strcmp(input_file_type, "ppm") == 0) {
        /* Convert from 24bit (RGB888) to 16bit (RGB565) or 8bit Grayscale
         * 
        * */
        u8* input_data = input_buffer;
        usize byte_offset = 0; 

        // Remove header from netpbm format 
        if (b_is_netpbm_format) {
            // get size of remaining image
            printf("before: %lu\n", file_size);
            usize remaining_file_size = find_end_of_header(input_buffer, MAX_PIXEL_VALUE);
            byte_offset = file_size - remaining_file_size;
            file_size = remaining_file_size;
            j = byte_offset;
            printf("after: %lu\n", file_size);
            printf("offset: %lu\n", byte_offset);
        }
        if (b_is_grayscale) {
            /* Convert to 8bit Grayscale
             *
             * */
            u8* output_data = output_buffer; 
            output_file_size = file_size * sizeof(u8) / (sizeof(u8) * 3);
            
            /* Add pgm header as required
             *
             * */
            if (strcmp(output_format, "pgm") == 0) {
                const char * netpbm_header = concat_netpbm_header(PGM_MAGIC_NUMBER, MAX_PIXEL_VALUE);
                for (j = 0; j < strlen(netpbm_header); j++) {
                    output_data[j] = netpbm_header[j];
                    output_file_size += strlen(netpbm_header);
                } 
            }

            //convert the input buffer to binary
            for (i = 0 + byte_offset; i < output_file_size; i++) {
                rgb888pixel.red = input_data[j++];
                rgb888pixel.green = input_data[j++];
                rgb888pixel.blue = input_data[j++];

                grayscale_pixel = rgb888_to_grayscale(rgb888pixel);

                output_data[i] = grayscale_pixel;
            }        
        } else if ((strcmp(output_format, "rgb888") == 0) &&
        (strcmp(input_file_type, "24bit") == 0)) {
            printf("Output format '%s' matches specified input format '%s'. Nothing to do.", output_format, input_file_type);
            exit(0);
        } else if ((strcmp(output_format, "rgb888") == 0) &&
        (strcmp(input_file_type, "ppm") == 0)) {
            /* Remove netpbm header from image data to turn it into raw rgb888
             *
             * */
            output_file_size = file_size;
            u8* output_data = output_buffer;
            for (i = 0; i < file_size; i++){
                output_data[i] = input_data[i+13];
            }
        } else if (strcmp(output_format, "rgb565") == 0) { 
            /* Convert to 16bit rgb (RGB565)
             *
             * */
            u16* output_data = output_buffer; 
            output_file_size = file_size * sizeof(u16) / (sizeof(u8) * 3);
            //convert the input buffer to binary
            for (i = 0; i < output_file_size; i++) {
                rgb888pixel.red = input_data[j++];
                rgb888pixel.green = input_data[j++];
                rgb888pixel.blue = input_data[j++];

                rgb565pixel = rgb888_to_rgb565(rgb888pixel);

                output_data[i] = rgb565pixel;
            }        
        } else {
            printf("No valid output format was specified. Input was '%s'\n", output_format);
            exit(1);
        }
    /************************************************************************************
     * Conversion from 16 bit rgb (RGB565)
     *
     * */ 
    } else if (strcmp(input_file_type, "16bit") == 0) {
        /* Convert from 16bit (RGB565) to 24bit (RGB888) or 8bit Grayscale
         * 
        * */
        u16* input_data         = input_buffer;
        u8* output_data         = output_buffer;

        // Convert to grayscale as required
        if (b_is_grayscale) {
            /* Convert to 8bit Grayscale
             *
             * */ 
            output_file_size        = file_size * (sizeof(u8)) / sizeof(u16);
           
            /* Add pgm header as required
             *
             * */
            if (strcmp(output_format, "pgm") == 0) {
                const char * netpbm_header = concat_netpbm_header(PGM_MAGIC_NUMBER, MAX_PIXEL_VALUE);
                for (j = 0; j < strlen(netpbm_header); j++) {
                    output_data[j] = netpbm_header[j];
                    output_file_size += strlen(netpbm_header);
                } 
            }
            
            //convert the input buffer to right format and store it to output buffer
            for (i = 0; i < file_size * sizeof(u16) / sizeof(u8); i++) {
                rgb888pixel = rgb565_to_rgb888(input_data[i]);

                output_data[j++] = rgb888_to_grayscale(rgb888pixel);

                // reset output pixel
                rgb888pixel.red = 0;
                rgb888pixel.green = 0;
                rgb888pixel.blue = 0;
            }
        // If output format equals input format, do nothing
        } else if (strcmp(output_format, "rgb565") == 0) {
            printf("Output format '%s' matches specified input format '%s'. Nothing to do.", output_format, input_file_type);
            exit(0);
        } else if (strcmp(output_format, "rgb888") == 0 ||
        strcmp(output_format, "ppm") == 0) { 
            /* Convert to 24bit rgb (RGB888)
             *
             * */
            output_file_size        = file_size * (sizeof(u8) * 3) / sizeof(u16);

            /* Add ppm header as required
             *
             * */
            if (strcmp(output_format, "ppm") == 0) {
                const char * netpbm_header = concat_netpbm_header(PPM_MAGIC_NUMBER, MAX_PIXEL_VALUE);
                for (j = 0; j < strlen(netpbm_header); j++) {
                    output_data[j] = netpbm_header[j];
                    output_file_size += strlen(netpbm_header);
                } 
            }

            //convert the input buffer to binary
            for (i = 0; i < file_size * sizeof(u16) / sizeof(u8); i++) {
                rgb888pixel = rgb565_to_rgb888(input_data[i]);

                output_data[j++] = rgb888pixel.red;
                rgb888pixel.red = 0;        // Reset used colors
                output_data[j++] = rgb888pixel.green;
                rgb888pixel.green = 0;        // Reset used colors
                output_data[j++] = rgb888pixel.blue;
                rgb888pixel.blue = 0;        // Reset used colors
            }
        } else {
            printf("No valid output format was specified. Input was '%s'\n", output_format);
            exit(1);
        }
    } else {
        fprintf(stderr, "Wrong file type specified. Needs to be '16bit' or '24bit'"); 
        fprintf(stderr, "Usage: %s <input file> <output file> <input file type: 16bit|24bit|pbm|pgm|ppm> <output format: none|grayscale|rgb565|rgb888|pbm|pgm|ppm>\n", argv[0]);
        exit(1);
    }

    // Write the input file to binary, half file size because of concatenation
    file_size = file_open_and_write(write_filepath, output_buffer, output_file_size);

    printf("Sucessfully wrote file '%s' of size '%zu'\n", write_filepath, file_size);
    
    free(input_buffer);
    free(output_buffer);
    input_buffer = NULL;
    output_buffer = NULL;
    return EXIT_SUCCESS;
}
