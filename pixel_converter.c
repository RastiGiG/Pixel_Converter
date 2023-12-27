/**************************************************************************
 * COLOR CONVERTER V 1.0 
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

/* Libraries
 *
 * */
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Macros
 *
 * */
#define MAX_BUFF_SIZE                1024 * 1024 
#define NUMBER_OF_CHARS_PER_DIGITS   4 
#define BINARY_WRITE                 "wb"


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


/* Function converting hexadecimal chars into integers
 *
 * */
u8 hex_to_int(char hex_digit) {
    u8 decimal;
    if (hex_digit >= '0' && hex_digit <= '9') {
        decimal = hex_digit - '0';
    } else if (hex_digit >= 'a' && hex_digit <= 'f') {
        decimal = hex_digit - 'a' + 10;
    } else if (hex_digit >= 'A' && hex_digit <= 'F') {
        decimal = hex_digit - 'A' + 10;
    } else {
        fprintf(stderr, "[ERROR]: Invalid hexadecimal digit: %c\n", hex_digit);
        //exit(1);
        return 0;
    }

    return decimal;
}

/* 
 *
 *
*/
u16 concat_digits(char *hex_digits) {
    u16 output_number = 0;
    for (int i = 0; i < NUMBER_OF_CHARS_PER_DIGITS; i++) {
        if ((hex_digits[i] >= 'A' && hex_digits[i] <= 'F') 
            || (hex_digits[i] >= 'a' && hex_digits[i] <= 'f') 
            || (hex_digits[i] > '0' && hex_digits[i] <= '9')) {
            output_number |= hex_to_int(hex_digits[i]) << (12 - i*4);
        } 
    }
    return output_number;
}

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
    rgb888pixel.red = round(red * 0xff / redMax);
    rgb888pixel.green = round(green * 0xff / greenMax);
    rgb888pixel.blue = round(blue * 0xff / blueMax);

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
    if (argc != 4){
        fprintf(stderr, "Usage: %s <input file> <output file> <input file type: 16bit|24bit>\n", argv[0]);
        exit(0);
    }

    char *read_filepath = argv[1];
    char *write_filepath = argv[2];
    char *input_file_type = argv[3];
    void* input_buffer = calloc(MAX_BUFF_SIZE, sizeof(char));
    void* output_buffer = calloc(MAX_BUFF_SIZE, sizeof(u8));

    // Read the input file
    usize file_size = file_open_and_read(read_filepath, input_buffer);

    /*
    * i, j              -- counter for bytes in input and output file
    * rgb565pixel       -- buffer for 16bit pixel; 
    * rgb888pixel       -- buffer for 24bit pixel;
    * output_file_size  -- determines size of output file
    * */
    usize i, j                  = 0;
    u16 rgb565pixel             = 0;
    color_channels rgb888pixel  = {0, 0, 0};
    usize output_file_size      = 0;

    if (strcmp(input_file_type, "24bit") == 0) {
        /* Convert from 24bit (RGB888) to 16bit (RGB565)
         * 
        * */
        u8* input_data = input_buffer;
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
    
    } else if (strcmp(input_file_type, "16bit") == 0) {
        /* Convert from 16bit (RGB565) to 24bit (RGB888)
         * 
        * */
        u16* input_data = input_buffer;
        u8* output_data = output_buffer; 
        output_file_size = file_size * (sizeof(u8) * 3) / sizeof(u16);
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
        fprintf(stderr, "Wrong file type specified. Needs to be '16bit' or '24bit'"); 
        fprintf(stderr, "Usage: %s <input file> <output file> <input file type: 16bit|24bit>\n", argv[0]);
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
