/**
 * @brief  micros and structures for a simple PNG file 
 *
 * Copyright 2018-2020 Yiqing Huang
 * Updated 2024 m27ma
 *
 * This software may be freely redistributed under the terms of MIT License
 */
#pragma once

/******************************************************************************
 * INCLUDE HEADER FILES
 *****************************************************************************/
#include <stdio.h>

/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/

//note: these sizes are for accessing PNG files, not for accessing data structure types (might be padded)
#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */

/******************************************************************************
 * STRUCTURES and TYPEDEFS 
 *****************************************************************************/
typedef unsigned char U8;
typedef unsigned int  U32;

typedef struct chunk {
    U32 length;  /* length of data in the chunk, host byte order */
    U8  type[4]; /* chunk type */
    U8  *p_data; /* pointer to location where the actual data are */
    U32 crc;     /* CRC field  */
} *chunk_p;

/* note that there are 13 Bytes valid data, compiler will padd 3 bytes to make
   the structure 16 Bytes due to alignment. So do not use the size of this
   structure if you need the actual data size, use 13 Bytes (i.e DATA_IHDR_SIZE macro).
 */
typedef struct data_IHDR {// IHDR chunk data field
    U32 width;        /* width in pixels, big endian   */
    U32 height;       /* height in pixels, big endian  */
    U8  bit_depth;    /* num of bits per sample or per palette index.
                         valid values are: 1, 2, 4, 8, 16 */
    U8  color_type;   /* =0: Grayscale; =2: Truecolor; =3 Indexed-color
                         =4: Greyscale with alpha; =6: Truecolor with alpha */
    U8  compression;  /* only method 0 is defined for now */
    U8  filter;       /* only method 0 is defined for now */
    U8  interlace;    /* =0: no interlace; =1: Adam7 interlace */
} *data_IHDR_p;

/* A simple PNG file format, three chunks only*/
typedef struct simple_PNG {
    struct chunk *p_IHDR;
    struct chunk *p_IDAT;  /* only handles one IDAT chunk */  
    struct chunk *p_IEND;
} *simple_PNG_p;

/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/
/* this is one possible way to structure the PNG manipulation functions */

int is_png(U8 *buf, size_t n); //check if PNG signature is present
    //takes in pointer to least 8 bytes of binary data
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence); //extract from file the data field of the IHDR chunk, to populate a struct data_IHDR
    //takes in file pointer and how to reach data field of the IDHR chunk (see fseek parameters)
int get_png_height(struct data_IHDR *buf); //read out image height from a struct data_IHDR
int get_png_width(struct data_IHDR *buf); //read out image width from a struct data_IHDR

int get_png_chunks(simple_PNG_p out, FILE* fp, long offset, int whence); //extract from file all chunks in a png, to populate a struct simple_PNG
    //takes in file pointer and how to reach the IHDR chunk (see fseek parameters)
chunk_p get_chunk(FILE *fp); //extract from file one chunk and populate a struct chunk
    //takes in file pointer that is already at the start of the target chunk

U32 get_chunk_crc(chunk_p in); //read out expected crc from a struct chunk
U32 calculate_chunk_crc(chunk_p in); //calculate crc using chunk type and chunk data

simple_PNG_p mallocPNG(); //allocate memory for a struct simple_PNG
void free_png( simple_PNG_p in); //free the memory of a struct simple_PNG
void free_chunk(chunk_p in); //free the memory of a struct chunk and inner data buffers

int write_PNG(char* filepath, simple_PNG_p in); //write a struct simple_PNG to file
int write_chunk(FILE* fp, chunk_p in); //write a struct chunk to file
  

/* you're free to design and declare your own functions prototypes here*/
