#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<unistd.h>
#include<fcntl.h>
#include <sys/stat.h>
#include "final_archive_structs.h"

#define BUFFER_SIZE (8 * 1024)

/* 
   createfile takes in 3 parameters: 
   - filename: the name of the file to be created.
   - size: the size of the file.
   - file_to_read_from: the file descriptor for an open archive file.
   
   The function creates a new file with the given filename, reads 'size' bytes from the open 
   archive file, and writes them to the new file.
*/
int createfile(char* filename, off_t size, int file_to_read_from) {
    // Open or create the destination file
    int new_file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (new_file == -1) {
        perror("Error creating destination file");
        close(file_to_read_from);
        return -1;
    }

    // Allocate memory for a buffer to read data
    char *buffer = malloc(size);
    if (buffer == NULL) {
        perror("Error allocating memory for buffer");
        close(file_to_read_from);
        close(new_file);
        return -1;
    }

    // Read 'size' bytes from the archive file
    ssize_t bytes_read = read(file_to_read_from, buffer, size);

    // Check for read errors
    if (bytes_read == -1) {
        perror("Error reading from source file");
        free(buffer);
        close(file_to_read_from);
        close(new_file);
        return -1;
    }

    // Write the read data to the destination file
    ssize_t bytes_written = write(new_file, buffer, bytes_read);

    // Check for write errors
    if (bytes_written != bytes_read) {
        perror("Error writing to destination file");
        free(buffer);
        close(file_to_read_from);
        close(new_file);
        return -1;
    }

    // Free the buffer and close the files
    free(buffer);
    close(new_file);
    return 0;
}

int main(int argc, char** argv) {
    // Check if the correct number of command-line arguments is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <archive_filename>\n", argv[0]);
        return 1;  // Return an error code
    }

    // Extract archive file name from command-line arguments
    char* unarchive = argv[1];
    int unarchivefile = open(unarchive, O_RDONLY);

    // Check if the archive file opening was successful
    if (unarchivefile == -1) {
        perror("Error opening archive file");
        return 1;  // Return an error code
    }

    meta_data_for_directory file_info;

    // Read meta_data_for_directory structure from the archive file
    ssize_t bytes_read = read(unarchivefile, &file_info, sizeof(struct meta_data_for_directory));

    // Check for read errors
    if (bytes_read == -1) {
        perror("Error reading from archive file");
        close(unarchivefile);
        return 1;  // Return an error code
    }

    // Create a directory with the specified name
    char *directory_name = file_info.directory_name;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if (mkdir(directory_name, mode) == -1) {
        perror("Error creating directory");
        close(unarchivefile);
        exit(EXIT_FAILURE);
    }

    // Move the file pointer to the beginning of the file content
    int cur = sizeof(file_info);
    lseek(unarchivefile, cur, SEEK_SET);

    // Iterate through each file in the archive and create it
    for (int i = 0; i < file_info.number_of_files; i++) {
        // No need to open the archive file again; use the existing file descriptor
        lseek(unarchivefile, cur, SEEK_SET);

        off_t filesize = file_info.file_sizes[i];
        
        
        // Create the file with the specified size and content
        createfile(file_info.filenames[i], filesize, unarchivefile);

        // Move the file pointer to the next file in the archive
        cur += (int)filesize;
    }

    // Close the archive file
    close(unarchivefile);
    return 0;
}