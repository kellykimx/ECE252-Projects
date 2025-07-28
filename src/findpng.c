#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "lab_png.h"
#include "zutil.h"

//use is_png() function that is in pnginfo.
//using codes from ls_fname.c, cmd_arg.c
//write a recursive function to check the directory

int search_directory(const char *path, int *found) { //path: current directory, found: seeing if any png files were found
    DIR *p_dir; //pointer to a directory structure
    struct dirent *p_dirent; //pointer to each file/entry in the directory

    if ((p_dir = opendir(path)) == NULL) { //if they try to open the directory and its empty, exits
        perror(path); //printing an error
        return 1;
    }

    //keep looping through each entry in the directory until there is none
    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *currentfile_name = p_dirent->d_name;

        if (strcmp(currentfile_name, ".") == 0 || strcmp(currentfile_name, "..") == 0) {
            continue;
        }


        char png_path[512]; //this is where the path to png file will be stored, can store upto 512 characters
        snprintf(png_path, sizeof(png_path), "%s/%s", path, currentfile_name); //writes them into string and storing them into png_path

        struct stat fileinfo;
        if (stat(png_path, &fileinfo) != 0) { //using the is_png() function to check if the file is png
            perror(png_path);
            continue;
        }

        if(S_ISREG(fileinfo.st_mode)) {
            FILE *fp = fopen(png_path, "rb");
            if(fp) {
                U8 buf[8];
                fread(buf, 1, 8, fp);
                fclose(fp);

                /* printf("File: %s\nBytes: ", png_path);
                for (int i = 0; i < 8; i++) {
                    printf("%02x ", buf[i]);
                }
                printf("\n"); */


                if (is_png(buf, 8)) {
                    printf("%s\n", png_path);
                    *found = 1;
                }
            }
        } else if (S_ISDIR(fileinfo.st_mode)) {
            search_directory(png_path, found);
        }

    } 

    if(closedir(p_dir) != 0) { //closing the directory
        perror("closedir");
        return 1;
    }
    return 0;
}

int main (int argc, char *argv[]) {
    //argc: argument count
    //argc: argument vector, array of strings, argv[1] is directory name

    if (argc == 1) { // if the user does not provide a directory name, it shows an error and exits
        return 1;
    }

    int found = 0; //this is to check if there is any valid png files in the directory
    search_directory(argv[1], &found); //calling search directory to go into argv[1] name directory, passing the pointer of found

    if (found==0) {
        printf("findpng: No PNG file found");
    }
    return 0;
}
