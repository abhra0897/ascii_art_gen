/*
MIT License

Copyright (c) 2020 Avra Mitra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


// ASCII Art Generator is an application to convert an image to ASCII text.
// It supports BMP format only as input image. Thst too with the following limitation:
// Bits per pixel: 24
// Compression: 0 (none)
// To understand BMP file format: https://en.wikipedia.org/wiki/BMP_file_format
// Nearest neighour algorithm is used to scale the image. https://tech-algorithm.com/articles/nearest-neighbor-image-scaling/
// Create BMP images using the given link to test with this application: https://online-converting.com/image/convert2bmp/#
// Output ASCII art's resolution is defined in NEW_MAX_WIDTH and NEW_MAX_HEIGHT
// Output is stored in `ascii_art_out.txt` text file.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define FILE_HEADER_SIZE		14
#define BMP_SIZE_OFFSET			2
#define IMG_START_OFFSET		10
#define IMG_WIDTH_OFFSET		18
#define IMG_HEIGHT_OFFSET		22
#define BPP_OFFSET				28
#define COMPRESSION_OFFSET		30
#define IMG_DATA_SIZE_OFFSET	34

#define SUPPORTED_BPP			24
#define SUPPORTED_COMPRESSION	0
#define SUPPORTED_MAX_H_RES		2000
#define SUPPORTED_MAX_V_RES		2000

#define NEW_MAX_WIDTH			200
#define NEW_MAX_HEIGHT			200

typedef enum
{
	ERR_COMPRESSION = 1,
	ERR_BPP,
	ERR_WIDTH,
	ERR_HEIGHT
}err_t;


typedef struct 
{
	uint8_t *header_array;
	uint32_t size_bmp;
	uint32_t img_data_offset;
	int32_t img_width;
	int32_t img_height;		// if top-bottom, height positive. else height negative
	uint8_t bits_per_pixel;
	uint8_t compression_type;
	uint32_t size_img_data;
}bmp_header_t;


char get_char_for_greyscale(uint8_t p_greyscale);
bmp_header_t get_bmp_header(FILE *p_bmp);
err_t validate_header(bmp_header_t *p_header);
void image_to_ascii(FILE *p_ascii, FILE *p_bmp, bmp_header_t *p_header);


int main()
{
	// input image and output text file names
	char *str_input_file_name = "input_image.bmp";
	char *str_output_file_name = "ascii_art_out.txt";

	// Opening input and output files
	FILE *fl_bmp_img = fopen(str_input_file_name, "rb");
	FILE *fl_ascii_out = fopen(str_output_file_name, "w");
	
	// Get the header info
	bmp_header_t bmp_header = get_bmp_header(fl_bmp_img);
	
	// Display header info (not impotant)
	printf("BMP Size (in Bytes): %d\n", bmp_header.size_bmp);
	printf("Offset to start image: %d\n", bmp_header.img_data_offset);
	printf("width: %d\tHeight: %d\n", bmp_header.img_width, bmp_header.img_height);
	printf("Bits per pixel: %d\n", bmp_header.bits_per_pixel);
	printf("Compressiopn: %d\n", bmp_header.compression_type);
	printf("Image Size with padding (in Bytes): %d\n", bmp_header.size_img_data);
	
	// Check if any error in header
	err_t error_code = validate_header(&bmp_header);
	if (error_code == 0)
	{
		// if bmp is supported, start resizing and converting
		image_to_ascii(fl_ascii_out, fl_bmp_img, &bmp_header);
	}
	else
	{
		printf("Error Code: %d\n", error_code);
	}
	
	fclose(fl_bmp_img);
	fclose(fl_ascii_out);

	return 0;
}


// Check if the image parameters are supported by this application
err_t validate_header(bmp_header_t *p_header)
{
	err_t err_code = 0;
	if (p_header->bits_per_pixel != SUPPORTED_BPP)
		err_code = ERR_BPP;
	else if (p_header->compression_type != SUPPORTED_COMPRESSION)
		err_code = ERR_COMPRESSION;
	else if (p_header->img_width > SUPPORTED_MAX_H_RES)
		err_code = ERR_WIDTH;
	else if (p_header->img_height > SUPPORTED_MAX_V_RES)
		err_code = ERR_HEIGHT;

	return err_code;
}


// Read the header and return the structure containing parameters and the header array itself
bmp_header_t get_bmp_header(FILE *p_bmp)
{
	// Set file pointer to the begininig
	fseek(p_bmp, 0, SEEK_SET);

	bmp_header_t bmp_header;

	// File header is of 14 byte (must be)
	// So, read the file header first
	bmp_header.header_array = (uint8_t*)malloc(sizeof(uint8_t) * FILE_HEADER_SIZE);

	// Read only the file header into array
	fread(bmp_header.header_array, sizeof(uint8_t), FILE_HEADER_SIZE, p_bmp);
	
	// Read file header's parameters
	bmp_header.size_bmp = *(uint32_t *)&bmp_header.header_array[BMP_SIZE_OFFSET];
	bmp_header.img_data_offset = *(uint32_t *)&bmp_header.header_array[IMG_START_OFFSET];

	// total header size = image_data_offset. So, increase the array size as per the total header size
	bmp_header.header_array = (uint8_t*)realloc(bmp_header.header_array, sizeof(uint8_t) * bmp_header.img_data_offset);
	uint8_t remainig_header_size = bmp_header.img_data_offset - FILE_HEADER_SIZE;
	// array address is offset by 14 bytes so previous header is not overwritten
	// Now read remaining header into array
	fread(bmp_header.header_array + FILE_HEADER_SIZE, sizeof(uint8_t), remainig_header_size, p_bmp);

	// Read other headers' parameters
	bmp_header.img_width = *(int32_t *)&bmp_header.header_array[IMG_WIDTH_OFFSET];
	bmp_header.img_height = *(int32_t *)&bmp_header.header_array[IMG_HEIGHT_OFFSET];
	bmp_header.bits_per_pixel = bmp_header.header_array[BPP_OFFSET];
	bmp_header.compression_type = bmp_header.header_array[COMPRESSION_OFFSET];
	bmp_header.size_img_data = *(uint32_t *)&bmp_header.header_array[IMG_DATA_SIZE_OFFSET];

	return bmp_header;
}


// Resize the image, make greyscale, and write the respective ascii chras in a text file
void image_to_ascii(FILE *p_ascii, FILE *p_bmp, bmp_header_t *p_header)
{
	// To avoid floating point in quotient, multiplying the dividend with 2^16 so that quotient can minimum be 1
	float width_scaling_ratio = NEW_MAX_WIDTH / (float)p_header->img_width;
	float height_scaling_ratio = NEW_MAX_HEIGHT / (float)p_header->img_height;	
	// which ever is lower, is the best scaling ratio
	float best_scaling_ratio = (width_scaling_ratio < height_scaling_ratio) ? width_scaling_ratio : height_scaling_ratio;
	printf("\nscale w: %f   scale h: %f		best scale: %f\n", width_scaling_ratio, height_scaling_ratio, best_scaling_ratio);

	// calculate new width and height based on scaling ratio (without losing aspect ratio)
	uint16_t img_new_width = p_header->img_width * best_scaling_ratio;
	uint16_t img_new_height = abs(p_header->img_height) * best_scaling_ratio;
	printf("\nNew w: %d   New h: %d\n", img_new_width, img_new_height);

	// Row Size (in bytes)
	uint32_t img_row_bytes = p_header->img_width * (p_header->bits_per_pixel / 8);
	uint32_t img_new_row_bytes = img_new_width * (p_header->bits_per_pixel / 8);

	// Must be multiple of 4. So, calculate the next multiple of 4
	uint32_t img_row_bytes_with_padding = ((img_row_bytes + 3)/4)*4;
	
	// Entire image data size (in Bytes)
	uint32_t img_data_bytes_with_padding = img_row_bytes_with_padding * abs(p_header->img_height);
	uint32_t img_new_data_bytes = img_new_row_bytes * img_new_height;

	// Arrays to hold entire data
	uint8_t *img_data_padded = (uint8_t *)malloc(sizeof(uint8_t) * img_data_bytes_with_padding);
	uint8_t *img_new_data = (uint8_t *)malloc(sizeof(uint8_t) * img_new_data_bytes);

	// ASCII single row buffer
	uint32_t ascii_row_buff_size = (img_new_width + 2);	// width*char + 1*'\n' + 1*'\0'
	char *ascii_row_buff = (char *)malloc(sizeof(char) * ascii_row_buff_size);

	// Start reading from the proper location
	fseek(p_bmp, p_header->img_data_offset, SEEK_SET);
	fread(img_data_padded, sizeof(uint8_t), img_data_bytes_with_padding, p_bmp);

	if (img_data_padded != NULL && img_new_data != NULL)
	{
		uint16_t x_new = 0;
		uint16_t y_new = 0;
		uint32_t src_subpixel_base = 0;
		uint32_t dest_subpixel_base = 0;
		uint8_t greyscale_value = 0;

		// Using Nearest Neighour algorithm to scale the image
		// TODO: Bilinear Interpolation and Cubic Convolution
		for (uint16_t i = 0; i < img_new_height; i++)
		{
			for (uint16_t j = 0; j < img_new_width; j++)
			{
				x_new = j / best_scaling_ratio;
				// if image is stored bottom-up (normal), calculate source's y_new accordingly
				// else if image is stored top-bottom, it's simple.
				if (p_header->img_height > 0)
					y_new = (img_new_height - i - 1) / best_scaling_ratio;
				else if (p_header->img_height < 0)
					y_new = i  / best_scaling_ratio;
				else 
					break;

				// base address of source and destination subpixels (r, g, b)
				dest_subpixel_base = (i * img_new_row_bytes) + (j * 3);
				src_subpixel_base = (y_new * img_row_bytes_with_padding) + (x_new * 3);

				// printf("j: %d, x new: %d  ", j, x_new);
				// printf("i: %d, y new: %d\t Scaling: %f\n", i, y_new, best_scaling_ratio);

				img_new_data[dest_subpixel_base + 0] = img_data_padded[src_subpixel_base + 0];	// R or B
				img_new_data[dest_subpixel_base + 1] = img_data_padded[src_subpixel_base + 1];	// G
				img_new_data[dest_subpixel_base + 2] = img_data_padded[src_subpixel_base + 2];	// B or R

				// Average the RGB value to get greyscale value
				greyscale_value = (img_new_data[dest_subpixel_base + 0] + 
									img_new_data[dest_subpixel_base + 1] + 
									img_new_data[dest_subpixel_base + 2]) / 3;

				// Store the respective character in the ASCII row buffer
				ascii_row_buff[j] = get_char_for_greyscale(greyscale_value);
			}
			// Add newline and null termination characters
			ascii_row_buff[img_new_width] = '\n';
			ascii_row_buff[img_new_width + 1] = '\0';

			// display every alternate line to avoid the ascii art being vertically stretched
			if (i & 1)	//every odd line (can be even too, no problemo)
			{
				// Show output in terminal
				printf("%s", ascii_row_buff);
				// And also store in file
				fprintf(p_ascii, "%s", ascii_row_buff);
			}
				
		}
	}
	else
	{
		printf("malloc() failed!\n");
	}
}


// Returns character based on greyscale value
char get_char_for_greyscale(uint8_t p_greyscale)
{
	//const char ascii_ramp_lut[70] = "$@B\%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]\?-_+~<>i!lI;:,\"^`\'. "; 
	const char ascii_ramp_lut[10] = "@%#*+=-:. ";
	uint8_t lut_index = ((float)p_greyscale / 255.0) * 9.0;
	char out_char = ascii_ramp_lut[lut_index]; // cause we have 70 level of grey
}