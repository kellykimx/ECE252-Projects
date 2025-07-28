#include <stdio.h>
#include <stdlib.h>
#include "lab_png.h"
#include "crc.h"
#include "zutil.h"

//this function takes 8 bytes and check whether they match the PNG image file signature
    //U8 *buf is a pointer to a buffer (array) of bytes, U8: unsigned 8-bit integer
    //size_t n represents the number of butes available in buf
int is_png(U8 *buf, size_t n) {

    //this is the png signature that we need to check if it matches
    const U8 png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

    if (buf == NULL || n < 8) { //first need to check if the size is less than 8 because then its an error
       return -1; //it is an error, impossible
    } else {
        for (int i=0; i<n; i++) { //comparing if they match the PNG image file signature
            if(buf[i]!=png_signature[i]) {
                return 0;
                break;
            }
            //if it matches, then continue throughout all 8 bytes
        }
        return 1; //if everything matches return true
    }
}    

//this function extracts the image meta information including height and width from a PNG file IHDR chunk
    //struct data_IHDR *out: pointer to a struct to store extracted metadata, writing the width and the height into this struct
    //FILE *fp: file pointer to the png file
    //long offset: how many bytes to move in file before reading
    //int whence: passed to fseek(fp, offset, whence) to control how the offset is interpreted
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence) {
    //PNG signature --> chunk length --> IHDR chunk name --> width --> height --> others

    //moving the file pointer to a specific location in the file
    if (fseek(fp, offset, whence) != 0) {
        return 1;
    }

    //SEEK_CUR: moving the file pointer relative to its current position
    //offsetting the file by 8 bytes (first 8 bytes are the PNG signature)
    if (fseek(fp, 8, SEEK_CUR) != 0) {
        return 1;
    }

    unsigned char width_bytes[4], height_bytes[4]; //

    //fread automatically reads the next four bytes
    if (fread(width_bytes, 1, 4, fp) != 4) { //read the 4 bytes that represent width, if not exactly for 4 bytes, return an error
        return -1;
    }
    if (fread(height_bytes, 1, 4, fp) != 4) { //read the 4 bytes that represent height, if not exactly for 4 bytes, return an error
        return -1;
    }

    //PNG file uses big endian byte order
    //Multi-byte variable (ex. length, width, height) should be converted to host order (little endian) to do arithmetic calculations
    //width and height in struct are U32
    out->width= (width_bytes[0]<<24)|(width_bytes[1]<<16)|(width_bytes[2]<<8)|(width_bytes[3]);
    out->height= (height_bytes[0]<<24)|(height_bytes[1]<<16)|(height_bytes[2]<<8)|(height_bytes[3]);

    if (fread(&out->bit_depth, 1, 1, fp) != 1) return -1;
    if (fread(&out->color_type, 1, 1, fp) != 1) return -1;
    if (fread(&out->compression, 1, 1, fp) != 1) return -1;
    if (fread(&out->filter, 1, 1, fp) != 1) return -1;
    if (fread(&out->interlace, 1, 1, fp) != 1) return -1;

    return 0;
}

int get_png_width(struct data_IHDR *buf) {
    /*return ((buf->width & 0xFF000000) >> 24) |
           ((buf->width & 0x00FF0000) >> 8)  |
           ((buf->width & 0x0000FF00) << 8)  |
           ((buf->width & 0x000000FF) << 24); */
           return buf->width;
}


int get_png_height(struct data_IHDR *buf) {
    /*return ((buf->height & 0xFF000000) >> 24) |
           ((buf->height & 0x00FF0000) >> 8)  |
           ((buf->height & 0x0000FF00) << 8)  |
           ((buf->height & 0x000000FF) << 24); */
           return buf->height;
}


//chunk is a fundamental building block of a PNG file, PNG images are made up of sequence of chunks
//[4 bytes length] [4 bytes type] [N bytes data] [4 Bytes CRC]
//extract from file one chunk and populate a struct chunk
//takes in file pointer that is already at the start of the target chunk
chunk_p get_chunk(FILE *fp) {
    U8 buf[4];
    chunk_p chunk1 = malloc(sizeof(struct chunk)); //allocates memory for one PNG chunk

    if (!chunk1) { //if malloc failed, return null so it exits
        return NULL;
    }

    fread(buf, 1, 4, fp); //read 4 bytes from the file and put it into buf, this is the length field
    //length: length of data section
    chunk1->length = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; //convert from big-endian to host-byte

    //type: 4 byte chunk type (e.g. IHDR, IDAT)
    fread(chunk1->type, 1, 4, fp); //then read the next 4 bytes, which is the chunk type

    if (chunk1->length > 0) {
        //p_data is the pointer to the actua chunk data
        chunk1->p_data = malloc(chunk1->length); //allocate memory for the data field
        fread(chunk1->p_data, 1, chunk1->length, fp); //then read the next length bytes into p_data
    } else {
        chunk1->p_data = NULL;
    }

    fread(buf, 1, 4, fp); //read the 4 final bytes from crc
    //crc: 4 bytes CRC for type + daya
    chunk1->crc = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; //converting big endian to host byte

    return chunk1;
}

//calculate crc using chunk type and chunk data
U32 calculate_chunk_crc(chunk_p in) { //takes a pointer to a PNG chunk

    if (!in) { //make sure it isnt null
        return 0;
    }

    // CRC is calculated on type + data (not length)
    int numlength = in->length; //storing the number of data bytes
    int total = CHUNK_TYPE_SIZE + numlength; // 4 for type + numlength, total number of bytes needed for CRC input

    U8 *buf = malloc(total); //dynamically allocate buffer to hold the concatenated chunk type + data
    memcpy(buf, in->type, CHUNK_TYPE_SIZE); //copy the chunk type into bug

    if (numlength > 0 && in->p_data) { //if there is data
        memcpy(buf + CHUNK_TYPE_SIZE, in->p_data, numlength); //copy it into buf
    }

    //calculating the crc
    //if p_data is not NULL (there is data) then use chunk_type_size + length, or else use chunk_type_size
    U32 result = crc(buf, in->p_data ? CHUNK_TYPE_SIZE + numlength : CHUNK_TYPE_SIZE);
    free(buf); //free the dynamically allocated memory
    return result;
}

void free_chunk(chunk_p in) {
    if (in) { //if the pointer is not NULL
        if (in->p_data) { //if chunk actually has data
            free(in->p_data); //free memory
        } 
        free(in); //then free memory for struct chunk
    }
}

//write a struct simple_PNG to file
int write_chunk(FILE *fp, chunk_p in) { //in is a pointer to the struct chunk
    U8 b[4];

    //write length
    //converting (big-endian)
    b[0] = (in->length >> 24) & 0xFF;
    b[1] = (in->length >> 16) & 0xFF;
    b[2] = (in->length >> 8) & 0xFF;
    b[3] = in->length & 0xFF;
    fwrite(b, 1, 4, fp); //writing the result into file

    //write type
    fwrite(in->type, 1, 4, fp);

    //write data
    if (in->length > 0 && in->p_data) { //if chunk has data, write that to the file, length checking if it has data, also checking pointer to that data is not null
        fwrite(in->p_data, 1, in->length, fp);
    }

    //write CRC
    //convering (big-endian)
    b[0] = (in->crc >> 24) & 0xFF;
    b[1] = (in->crc >> 16) & 0xFF;
    b[2] = (in->crc >> 8) & 0xFF;
    b[3] = in->crc & 0xFF;
    fwrite(b, 1, 4, fp);

    return 0;
}


/*
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.png>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    U8 signature[8];
    if (fread(signature, 1, 8, fp) != 8) {
        fprintf(stderr, "%s: Not a PNG file\n", filename);
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (is_png(signature, 8) != 0) {
        printf("%s: Not a PNG file\n", filename);
        fclose(fp);
        return EXIT_FAILURE;
    }

    struct data_IHDR img_data;
    if (get_png_data_IHDR(&img_data, fp, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Error reading PNG IHDR data\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    printf("%s: %u x %u\n", filename, img_data.width, img_data.height);

    // Optional CRC check
    // You can add code here to read the CRC and compare with computed CRC

    fclose(fp);
    return EXIT_SUCCESS;
}
*/
