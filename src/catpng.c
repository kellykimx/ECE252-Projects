#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lab_png.h"
#include "zutil.h"
#include "crc.h"

int concatenate_png(int argc, char *argv[]) {
    struct data_IHDR header_info[argc]; //is holding IHDR (header) information from one png file
    U8 *data[argc]; //this will point to uncompressed pixel data from one png file
    int width_check = 0; //this is to check that width for images are the same
    int total_height = 0; //sum of all the png heights
    int png_count = 0;

    for (int i = 0; i < argc; i++) {

        FILE *fp = fopen(argv[i], "rb"); //rb is for reading

        if (!fp) { 
            perror("fopen"); 
            return 1; 
        }

        //checking if the file is actually a valid PNG
        U8 signature[8];

        if (fread(signature, 1, 8, fp) != 8) {
            fclose(fp);
            continue;
        }

        if (!is_png(signature, 8)) {
            //fprintf(stderr, "[SKIP] Not a PNG: %s\n", argv[i]);
            fclose(fp);
            continue;
        }

        //this function extracts the image meta information including height and width from a PNG file IHDR chunk
        get_png_data_IHDR(&header_info[png_count], fp, 8, SEEK_SET); //storing it into header_info
        int current_width = get_png_width(&header_info[png_count]);
        int current_height = get_png_height(&header_info[png_count]);

        if (width_check == 0) { //if it is the first time (initialized it to 0)
            width_check = current_width; //set the actually width for checking
        } else if (width_check != current_width) { //if it is not the first time and width does not match with rest of the png files
            fclose(fp);
            continue;
        }

        //33 comes from png signature (8 bytes) + IHDR chunk (25 bytes)
        //8+4+4+13+4=33
        fseek(fp, 33, SEEK_SET);
        chunk_p IDAT = get_chunk(fp); //reading the next chunk from file

        if (!IDAT) {
            fclose(fp);
            return 1; // Could not find a valid IDAT chunk
        }

        while (memcmp(IDAT->type, "IDAT", 4) != 0) { //comparing type to IDAT
            free_chunk(IDAT); // if not IDAT chunk, free it
            IDAT = get_chunk(fp);
            //keeping looping until IDAT chunk found
            if (!IDAT) {
                fclose(fp);
                return 1; // Could not find a valid IDAT chunk
            }
        }

        //raw (uncompressed) data size is height*(width*4+1) bytes
        int raw_data_size = get_png_height(&header_info[png_count])*(width_check*4+1);
        U8 *raw = malloc(raw_data_size); //allocating memory to hold uncompressed image data 

        if (!raw) {
            free_chunk(IDAT);
            fclose(fp);
            continue;
        }

        U64 raw_len_u64 = raw_data_size; //mem_inf() wants a pointer to U64
        mem_inf(raw, &raw_len_u64, IDAT->p_data, IDAT->length); //decompress the data so we can use it

        if (raw_len_u64 == 0) {
            free(raw);
            free_chunk(IDAT);
            fclose(fp);
            continue;
        }

        data[png_count] = raw; //this will point to the full uncompressed pixel data, for later
        total_height += get_png_height(&header_info[png_count]); //adding the height to total height
        png_count++;

        free_chunk(IDAT);
        fclose(fp);
    }

    if (png_count == 0) {
        return 0;  // DO NOT create all.png at all
    }

    int final_data_size = total_height*(width_check*4+1);
    U8 *merge = malloc(final_data_size); //allocating entire merged image's uncompressed data

    if (!merge) {
        return 1;
    }

    int offset = 0;

    for (int i = 0; i < png_count; i++) {
        int numbytes = get_png_height(&header_info[i])*(width_check*4+1);

        if (!data[i]) {
            free(merge);
            return 1;
        }

        memcpy(merge + offset, data[i], numbytes); //copying bytes from data and putting into correct position in merge
        offset += numbytes; //updating offset so next image goes right after the last one
        free(data[i]);
    }

    U8 *compressed = malloc(final_data_size + 100); //allocates memory to hold the compresswed version of image data

    if (!compressed) {
        return 1;
    }

    U64 compressed_len_u64 = final_data_size + 100;
    mem_def(compressed, &compressed_len_u64, merge, final_data_size, -1);
    int compressed_size = (int)compressed_len_u64; //changing to int
    free(merge);

    //wb - write only, 'b' indicates it is a binary file
    FILE *out = fopen("all.png", "wb"); //opening a new file called all.png
    U8 png_sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(png_sig, 1, 8, out);

    //IHDR is the first require chunk in every PNG file
    chunk_p IHDR = malloc(sizeof(struct chunk)); //allocates memory for new png chunk
    IHDR->length = 13;
    memcpy(IHDR->type, "IHDR", 4);
    IHDR->p_data = malloc(13); //allocate space for the 13 bytes of IHDR data

    if (!IHDR->p_data) {
        free(IHDR);
        fclose(out);
        free(compressed);
        return 1;
    }

    //changing width value into big-endian 
    IHDR->p_data[0] = (width_check >> 24) & 0xFF;
    IHDR->p_data[1] = (width_check >> 16) & 0xFF;
    IHDR->p_data[2] = (width_check >> 8) & 0xFF;
    IHDR->p_data[3] = width_check & 0xFF;

    //changing height value into big-endian
    IHDR->p_data[4] = (total_height >> 24) & 0xFF;
    IHDR->p_data[5] = (total_height >> 16) & 0xFF;
    IHDR->p_data[6] = (total_height >> 8) & 0xFF;
    IHDR->p_data[7] = total_height & 0xFF;

    IHDR->p_data[8]  = header_info[0].bit_depth;
    IHDR->p_data[9]  = header_info[0].color_type;
    IHDR->p_data[10] = header_info[0].compression;
    IHDR->p_data[11] = header_info[0].filter;
    IHDR->p_data[12] = header_info[0].interlace;

    IHDR->crc = calculate_chunk_crc(IHDR); //calculating the crc
    write_chunk(out, IHDR); //writing the chunk
    free_chunk(IHDR);

    chunk_p IDAT = malloc(sizeof(struct chunk));
    IDAT->length = compressed_size;
    memcpy(IDAT->type, "IDAT", 4);
    IDAT->p_data = compressed;
    IDAT->crc = calculate_chunk_crc(IDAT);
    write_chunk(out, IDAT);
    free_chunk(IDAT);;

    chunk_p IEND = malloc(sizeof(struct chunk));
    IEND->length = 0;
    memcpy(IEND->type, "IEND", 4);
    IEND->p_data = NULL;
    IEND->crc = calculate_chunk_crc(IEND);
    write_chunk(out, IEND);
    free(IEND);

    fclose(out);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }
    return concatenate_png(argc-1, argv+1);
}
