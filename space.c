////////////////////////////////////////////////////////////////////////
// COMP1521 24T1 --- Assignment 2: `space', a simple file archiver
// <https://www.cse.unsw.edu.au/~cs1521/24T1/assignments/ass2/index.html>
//
// Written by Melina Salardini (z5393518) on 4/4/2024.
//
// 2024-03-08   v1.1    Team COMP1521 <cs1521 at cse.unsw.edu.au>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>
#include "space.h"


#define MAX_CONTENT_SIZE 100000
#define LIST_GALAXY 'L'
#define CHECK_GALAXY 'C'
#define EXTRACT_GALAXY 'E'
#define RANDOM_NUM 10
#define MAGIC_NUM_FORMAT 'c'
#define STAR_FORMAT_6 '6'
#define STAR_FORMAT_7 '7'
#define STAR_FORMAT_8 '8'
#define COSTOM_PERM_1 ".pl"
#define COSTOM_PERM_2 ".py"
#define COSTOM_PERM_3 ".sh"



// FUNCTION PROTOTYPES 
void read_data(FILE *file, uint8_t *data, size_t size);

uint16_t read_content_length (FILE *file);

uint8_t calculate_hash(uint8_t magic_number, uint8_t star_format,
                        const char* permissions, uint16_t pathname_length,
                        const char* pathname, uint64_t content_length,
                        uint8_t* content);

void processing_star(FILE *file, int long_listing, char function_name);

void check_galaxy_result (const char* pathname, uint8_t calculated_hash,
                            uint8_t hash);

void list_galaxy_result (const char* permissions, uint8_t star_format,
                        uint64_t content_length, const char* pathname,
                        int long_listing);

void extract_galaxy_result (const char* pathname, uint8_t* content,
                            uint64_t content_length);

void extract_base_path(const char *pathname, char *base_path);

void add_file_to_galaxy(FILE *galaxy_file, const char *file_path, int format);

void add_directory_to_galaxy(FILE *galaxy_file, const char *dir_path,
                                const char *base_path, int format);

void check_galaxy(char *galaxy_pathname);

void list_galaxy(char *galaxy_pathname, int long_listing);

void extract_galaxy(char *galaxy_pathname);

void create_galaxy(char *galaxy_pathname, int append, int format,
                   int n_pathnames, char *pathnames[n_pathnames]);

// print the files & directories stored in galaxy_pathname (subset 0)
//
// if long_listing is non-zero then file/directory permissions, formats & sizes
// are also printed (subset 0)

void list_galaxy(char *galaxy_pathname, int long_listing) {
    // open the galaxy for reading
    FILE *galaxy_file = fopen(galaxy_pathname, "rb");
    if (galaxy_file == NULL) {
        perror("Error opening the galaxy file.\n");
        exit(1);
    }

    // read the stars of the galaxy till the end of the file
    while (1) {
        char function_name = LIST_GALAXY;
        processing_star(galaxy_file, long_listing, function_name);

        // exit the while loop is the file ended
        int next_char = fgetc(galaxy_file);
        if (next_char == '\n' || next_char == EOF) {
            break;
        } else {
            ungetc(next_char, galaxy_file);
        }
    }
    // closing the file
    if (fclose(galaxy_file) != 0) {
        fprintf(stderr, "Error closing the file.\n");
        exit(1);
    }
}


// check the files & directories stored in galaxy_pathname (subset 1)
//
// prints the files & directories stored in galaxy_pathname with a message
// either, indicating the hash byte is correct, or indicating the hash byte
// is incorrect, what the incorrect value is and the correct value would be

void check_galaxy(char *galaxy_pathname) {
    // open the galaxy for reading
    FILE *galaxy_file = fopen(galaxy_pathname, "rb");
    if (galaxy_file == NULL) {
        perror("Error opening the galaxy file.\n");
        exit(1);
    }

    // read the stars of the galaxy till the end of the file
    while (1) {
        char function_name = CHECK_GALAXY;
        processing_star(galaxy_file, RANDOM_NUM, function_name);

        // exit the while loop is the file ended
        int next_char = fgetc(galaxy_file);
        if (next_char == '\n' || next_char == EOF) {
            break;
        } else {
            ungetc(next_char, galaxy_file);
        }
    }
    // closing the file
    if (fclose(galaxy_file) != 0) {
        fprintf(stderr, "Error closing the file.\n");
        exit(1);
    }
}


// extract the files/directories stored in galaxy_pathname (subset 1 & 3)

void extract_galaxy(char *galaxy_pathname) {
    // open the galaxy for reading
    FILE *galaxy_file = fopen(galaxy_pathname, "rb");
    if (galaxy_file == NULL) {
        perror("Error opening the galaxy file.\n");
        exit(1);
    }

    // read the stars of the galaxy till the end of the file
    while (1) {
        char function_name = EXTRACT_GALAXY;
        processing_star(galaxy_file, RANDOM_NUM, function_name);

        // exit the while loop is the file ended
        int next_char = fgetc(galaxy_file);
        if (next_char == '\n' || next_char == EOF) {
            break;
        } else {
            ungetc(next_char, galaxy_file);
        }
    }
    // closing the file
    if (fclose(galaxy_file) != 0) {
        fprintf(stderr, "Error closing the file.\n");
        exit(1);
    }
}


// create galaxy_pathname containing the files or directories specified in
// pathnames (subset 2 & 3)
//
// if append is zero galaxy_pathname should be over-written if it exists
// if append is non-zero galaxys should be instead appended to galaxy_pathname
//                       if it exists
//
// format specifies the galaxy format to use, it must be one STAR_FMT_6,
// STAR_FMT_7 or STAR_FMT_8
// ADD YOUR EXTRA FUNCTIONS HERE

void create_galaxy(char *galaxy_pathname, int append, int format,
                   int n_pathnames, char *pathnames[n_pathnames]) {

    FILE *galaxy_file;

    // determine if we should append or make a galaxy
    if (append) {
        galaxy_file = fopen(galaxy_pathname, "ab");
    } else {
        galaxy_file = fopen(galaxy_pathname, "wb");
    }

    if (galaxy_file == NULL) {
        perror("Error opening the galaxy_file.\n");
        exit(1);
    }

    // loop through each pathname and add to the galaxy_file
    for (int i = 0; i < n_pathnames; i++) {
        char dir_path[1000];
        snprintf(dir_path, sizeof(dir_path), "%s", pathnames[i]);

        char base_path[1000];
        extract_base_path(pathnames[i], base_path);

        struct stat s;
        if (stat(base_path, &s) != 0) {
            perror(base_path);
            exit(1);
        }
        
        // check if it is a directory of a file and continue accordingly
        if (S_ISDIR(s.st_mode)) {
            add_directory_to_galaxy(galaxy_file, dir_path, base_path, format);
        } else {
            add_file_to_galaxy(galaxy_file, pathnames[i], format);
        }
    }
    if (fclose(galaxy_file) != 0) {
        fprintf(stderr, "Error closing the galaxy file.\n");
        exit(1);
    }
}


// helper function to read data using "fread"

void read_data(FILE *file, uint8_t *data, size_t size) {
    size_t read_count = fread(data, sizeof(uint8_t), size, file);
    if (read_count != size) {
        fprintf(stderr, "Error reading data.\n");
        exit(1);
    }
}

// helper function to read the content length

uint16_t read_content_length (FILE *file) {
    uint16_t content_length = 0;
    uint8_t temp_byte;

    for (int i = 0; i < 6; i++) {
        // Read each byte of the 48-bit content length
        temp_byte = 0;
        read_data(file, &temp_byte, sizeof(uint8_t));
        // Combine bytes to form the 48-bit content length
        content_length |= ((uint64_t)temp_byte << (i * 8));
    }

    return content_length;
}

// helper function to calculate the hash using "galaxy_hash" function

uint8_t calculate_hash(uint8_t magic_number, uint8_t star_format,
                       const char* permissions, uint16_t pathname_length,
                       const char* pathname, uint64_t content_length,
                       uint8_t* content) {
                        
    uint8_t calculated_hash = 0;
    
    calculated_hash = galaxy_hash(calculated_hash, magic_number);
    calculated_hash = galaxy_hash(calculated_hash, star_format);
    
    for (int i = 0; i < 10; i++) {
        calculated_hash = galaxy_hash(calculated_hash, permissions[i]);
    }
    
    calculated_hash = galaxy_hash(calculated_hash,
                                   (uint8_t)(pathname_length & 0xFF));
    calculated_hash = galaxy_hash(calculated_hash,
                                   (uint8_t)(pathname_length >> 8));
    
    for (int i = 0; i < pathname_length; i++) {
        calculated_hash = galaxy_hash(calculated_hash, pathname[i]);
    }
    
    for (int i = 0; i < 6; i++) {
        calculated_hash = galaxy_hash(calculated_hash,
                                (uint8_t)((content_length >> (i * 8)) & 0xFF));
    }
    
    for (uint64_t i = 0; i < content_length; i++) {
        calculated_hash = galaxy_hash(calculated_hash, content[i]);
    }
    
    return calculated_hash;
}

// helper function to read different bytes of a file or directory
// and check if the formating is correct

void processing_star (FILE *file, int long_listing, char function_name) {

    // read the magic number and make sure the formating is correct
    uint8_t magic_number = fgetc(file);
    if (magic_number != MAGIC_NUM_FORMAT) {
        fprintf(stderr, "error: incorrect first star byte: 0x%02x should be 0x63\n", magic_number);
        exit(1);
    }
    // read the star format and make sure the formating is correct
    uint8_t star_format = fgetc(file);
    if (star_format != STAR_FORMAT_6 && star_format != STAR_FORMAT_7
        && star_format != STAR_FORMAT_8) {
        fprintf(stderr, "Error invalid file format for star.\n");
        exit(1);
    }
    // read permissions and make sure the formating is correct
    char permissions[11];
    read_data(file, (uint8_t *)permissions, 10);
    permissions[10] = '\0';

    //reading the pathname_length and make sure the length is correctly formated
    uint16_t pathname_length = 0;
    read_data(file, (uint8_t *)&pathname_length, sizeof(uint16_t));

    //reading the pathname and making sure the length is same as pathname_length
    char pathname[1000];
    read_data(file, (uint8_t *)pathname, pathname_length);
    pathname[pathname_length] = '\0';

    // Read the content_length
    uint64_t content_length = read_content_length(file);

    uint8_t *content = (uint8_t *)malloc(content_length * sizeof(uint8_t));
    if (content == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for content.\n");
        free(content);
        exit(1);
    }

    // Read the content from the file
    read_data(file, content, content_length);

    // read the hash
    uint8_t hash = fgetc(file);
    if (hash == 0) {
        fprintf(stderr, "Error reading hash.\n");
        free(content);
        exit(1);
    }

    // calculate the hash
    uint8_t calculated_hash = calculate_hash(magic_number, star_format,
                                            permissions, pathname_length,
                                            pathname, content_length, content);


    if (function_name == CHECK_GALAXY) {
        check_galaxy_result(pathname, calculated_hash, hash);
    } else if (function_name == LIST_GALAXY) {
        list_galaxy_result(permissions, star_format, content_length,
                            pathname, long_listing);
    } else if (function_name == EXTRACT_GALAXY) {
        extract_galaxy_result(pathname, content, content_length);
    }

    free(content);
}

// helper function to print out the result of the check_galaxy function

void check_galaxy_result (const char* pathname,
                          uint8_t calculated_hash, uint8_t hash) {

    if (calculated_hash == hash) {
        printf("%s - correct hash\n", pathname);
    } else {
        printf("%s - incorrect hash 0x%02x should be 0x%02x\n",
                pathname, calculated_hash, hash);
    }
}

// helper function to print the result of the list_galaxy function

void list_galaxy_result (const char* permissions, uint8_t star_format,
                        uint64_t content_length, const char* pathname,
                        int long_listing) {

    if (long_listing) {
        printf("%s  %c  %5lu  %s\n", permissions, star_format,
                                    content_length, pathname);
    } else {
        printf("%s\n", pathname);
    }
}

// helper function to print the result of the extract_galaxy funciton

void extract_galaxy_result (const char* pathname, uint8_t* content,
                            uint64_t content_length) {


    FILE *extracted_file = fopen(pathname, "wb");
    if (extracted_file == NULL) {
        fprintf(stderr, "Error creating extracted file.\n");
        free(content);
        exit(1);
    }
    fwrite(content, sizeof(uint8_t), content_length, extracted_file);
    fclose(extracted_file);

    mode_t file_permission = 0644;
    char *extension = strrchr(pathname, '.');
    if (extension != NULL) {
        if (strcmp(extension, COSTOM_PERM_1) == 0 ||
            strcmp(extension, COSTOM_PERM_2) == 0 ||
            strcmp(extension, COSTOM_PERM_3) == 0) {

            file_permission = 0755;
        }
    }

    // Set file permission
    if (chmod(pathname, file_permission) == -1) {
        fprintf(stderr, "Error setting file permission.");
        free(content);
        exit(1);
    }

    // print the required format for extracting files
    printf("Extracting: %s\n", pathname);
}

// helper function the seprate the base path from the full path

void extract_base_path(const char *pathname, char *base_path) {
    const char *slash_position = strchr(pathname, '/');
    if (slash_position != NULL) {
        strncpy(base_path, pathname, slash_position - pathname);
        base_path[slash_position - pathname] = '\0';
    } else {
        strcpy(base_path, pathname);
    }
}

// helper function to add a file to the galaxy according to the star formating

void add_file_to_galaxy(FILE *galaxy_file, const char *file_path, int format) {
    FILE *add_file = fopen(file_path, "rb");
    if (add_file == NULL) {
        fprintf(stderr, "Error opening %s file.\n", file_path);
        exit(1);
    }

    struct stat s;
    if (stat(file_path, &s) != 0) {
        perror(file_path);
        exit(1);
    }

    uint64_t content_length = s.st_size;
    uint16_t pathname_length = strlen(file_path);

    // getting the permission and convert it into string
    char permissions[11];
    permissions[0]= (S_ISDIR(s.st_mode)) ? 'd' : '-';
    permissions[1]= (s.st_mode & S_IRUSR) ? 'r' : '-';
    permissions[2]= (s.st_mode & S_IWUSR) ? 'w' : '-';
    permissions[3]= (s.st_mode & S_IXUSR) ? 'x' : '-';
    permissions[4]= (s.st_mode & S_IRGRP) ? 'r' : '-';
    permissions[5]= (s.st_mode & S_IWGRP) ? 'w' : '-';
    permissions[6]= (s.st_mode & S_IXGRP) ? 'x' : '-';
    permissions[7]= (s.st_mode & S_IROTH) ? 'r' : '-';
    permissions[8]= (s.st_mode & S_IWOTH) ? 'w' : '-';
    permissions[9]= (s.st_mode & S_IXOTH) ? 'x' : '-';
    permissions[10]= '\0';

    fputc(MAGIC_NUM_FORMAT, galaxy_file);

    fputc(format, galaxy_file);

    fwrite(permissions, sizeof(char), 10, galaxy_file);

    fputc(pathname_length & 0xFF, galaxy_file);
    fputc((pathname_length >> 8) & 0xFF, galaxy_file);

    fwrite(file_path, sizeof(char), pathname_length, galaxy_file);

    for (int j = 0; j < 6; j++) {

        fputc(content_length >> (j * 8) & 0xFF, galaxy_file);
    }

    // write the content
    int c;
    while ((c = fgetc(add_file)) != EOF) {
        fputc(c, galaxy_file);
    }


    // calculate the hash for the current star
    uint8_t calculated_hash = 0;
    calculated_hash = galaxy_hash(calculated_hash, MAGIC_NUM_FORMAT);
    calculated_hash = galaxy_hash(calculated_hash, format);
    for (int j = 0; j < 10; j++) {
        calculated_hash = galaxy_hash(calculated_hash, permissions[j]);
    }
    calculated_hash = galaxy_hash(calculated_hash, (uint8_t)(pathname_length & 0xFF));
    calculated_hash = galaxy_hash(calculated_hash, (uint8_t)(pathname_length >> 8));

    // calculate the hash for file_path
    for (int i = 0; i < pathname_length; i++) {
        calculated_hash = galaxy_hash(calculated_hash, file_path[i]);
    }

    // calculate hash for the content length
    for (int j = 0; j < 6; j++) {
        calculated_hash = galaxy_hash(calculated_hash, (uint8_t)((content_length >> (j * 8)) & 0xFF));
    }

    // calculate hash for the content
    rewind(add_file);
    while ((c = fgetc(add_file)) != EOF) {
        calculated_hash = galaxy_hash(calculated_hash, (uint8_t)c);
    }

    fputc(calculated_hash, galaxy_file);

    fclose(add_file);

    printf("Adding: %s\n", file_path);
}

// helper function to add a directory to a galaxy acoording to the star formating

void add_directory_to_galaxy(FILE *galaxy_file, const char *dir_path,
                                const char *base_path, int format) {
    DIR *dirp = opendir(base_path);
    if (dirp == NULL) {
        perror(base_path);
        exit(1);
    }

    add_file_to_galaxy(galaxy_file, base_path, format);

    struct dirent *de;
    while ((de = readdir(dirp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        // Check if de->d_name is contained within dir_path
        if (strstr(dir_path, de->d_name) == NULL) {
            continue;
        }

        char updated_base_path[1000];
        snprintf(updated_base_path, sizeof(updated_base_path), "%s/%s",
                base_path, de->d_name);

        add_file_to_galaxy(galaxy_file, updated_base_path, format);

    }
    add_file_to_galaxy(galaxy_file, dir_path, format);

    closedir(dirp);
}