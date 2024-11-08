#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<unistd.h>
#include<fcntl.h>
#include <sys/stat.h>
#include "final_archive_structs.h"

#define BUFFER_SIZE (8 * 1024)

/* Various function prototypes for the archival process */ 
int write_file_to_archive(char* file_to_be_written, char* file_to_write_to);
off_t get_file_size(char* filename);
meta_data_for_directory add_metadata(char* directory_name);

/* get_file_size takes in a filename to get the size and returns the size in bytes. This data
   will be used to populate the filesize field in the metadata stucture.
   The current plan for this is to use it within add_metadata. Probably
   can also be used for debugging purposes. */ 
off_t get_file_size(char* filename) {
    off_t size = 0;

    // Open the file in read-only mode
    int file = open(filename, O_RDONLY);

    // Check if the file opening was successful
    if (file == -1) {
        perror("Error opening file");
        return -1;
    }

    // Use lseek to move the file offset to the end of the file
    off_t result = lseek(file, 0, SEEK_END);

    // Check if lseek was successful
    if (result == -1) {
        perror("Error getting file size");
        close(file);
        return -1;
    }

    // Assign the file size to the 'size' variable
    size = result;

    // Close the file
    if (close(file) == -1) {
        perror("Error closing file");
        return -1;
    }

    // Return the file size
    return size;
}

/* add_metadata takes in a directory name as input and returns a struct filled with metadata
   from the files in the directory. This is going to most likely be the 
   majority of this assignment... bookkeeping ***Need to figure out what to return on an error/issue*** */ 
meta_data_for_directory add_metadata(char* directory_name) {
    meta_data_for_directory directory_info;

    // Open the directory for reading
    DIR *d = opendir(directory_name);

    // Check if the directory opening was successful
    if (d) {
        int i = 0;
        struct dirent *dir;

        // Iterate through each entry in the directory
        while ((dir = readdir(d)) != NULL) {
            char* filename = dir->d_name;

            // Skip entries for the current and parent directories
            if (strcmp(filename, "..") == 0 || strcmp(filename, ".") == 0) {
                continue;
            }

            // Build the full path for the file
            char full_path[256];  // Adjust the size accordingly
            snprintf(full_path, sizeof(full_path), "%s/%s", directory_name, filename);

            // Copy the full path to the filenames array in the directory_info struct
            strcpy(directory_info.filenames[i], strdup(full_path));

            // Get the size of the file using the get_file_size function
            int size = get_file_size(full_path);

            // Assign the file size to the file_sizes array in the directory_info struct
            directory_info.file_sizes[i] = size;
            i++;
        }

        // Close the directory
        closedir(d);

        // Set the number of files in the directory_info struct
        directory_info.number_of_files = i;

        // Copy the directory name to the directory_name field in the directory_info struct
        strncpy(directory_info.directory_name, directory_name, sizeof(directory_info.directory_name) - 1);
        directory_info.directory_name[sizeof(directory_info.directory_name) - 1] = '\0';  // Ensure null-termination

        // Return the populated directory_info struct
        return directory_info;
    } else {
        // Print an error message and return an empty meta_data_for_directory struct
        perror("Error opening directory");
        meta_data_for_directory empty;
        return empty;
    }
}

/* write_file_to_archive takes in two files, the first is the file to be written (The file being added
   to the archive) and the file being written to (the archive file). 
   *** Shouldn't need to wory about adding size, just use sizeof. Can be proven wrong though. ****/ 
int write_file_to_archive(char* file_to_be_written, char* file_to_write_to) {
    // Open the source file for reading
    int readingfile = open(file_to_be_written, O_RDONLY);

    // Check if the source file opening was successful
    if (readingfile == -1) {
        perror("Error opening source file");
        return -1;
    }

    // Open the destination file for writing or create it if it doesn't exist
    int archivefile = open(file_to_write_to, O_WRONLY | O_CREAT | O_APPEND);

    // Check if the destination file opening was successful
    if (archivefile == -1) {
        perror("Error opening or creating destination file");
        close(readingfile);
        return -1;
    }

    // Allocate memory for a buffer to read and write data
    char *buffer = malloc(BUFFER_SIZE);

    // Check if memory allocation was successful
    if (buffer == NULL) {
        perror("Error allocating memory for buffer");
        close(readingfile);
        close(archivefile);
        return -1;
    }

    ssize_t bytes_read, bytes_written;

    // Read data from the source file and write it to the destination file
    while ((bytes_read = read(readingfile, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(archivefile, buffer, bytes_read);

        // Check if writing to the destination file was successful
        if (bytes_written != bytes_read) {
            perror("Error writing to destination file");
            free(buffer);
            close(readingfile);
            close(archivefile);
            return -1;
        }
    }

    // Check for read errors
    if (bytes_read == -1) {
        perror("Error reading from source file");
        free(buffer);
        close(readingfile);
        close(archivefile);
        return -1;
    }

    // Free the buffer and close the files
    free(buffer);
    close(readingfile);
    close(archivefile);

    // Return 0 to indicate success
    return 0;
}

// Main function for archive process

int main(int argc, char** argv) {
    // Check if the correct number of command-line arguments is provided
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <archive_filename>\n", argv[0]);
        return -1;  // Return an error code
    }

    // Extract directory and archive file names from command-line arguments
    char* directory = argv[1];
    char* archive_filename = argv[2];

    // Obtain metadata for the files in the specified directory
    meta_data_for_directory file_metadata = add_metadata(directory);

    // Open the archive file for reading and writing, or create it if it doesn't exist
    int archive = open(archive_filename, O_RDWR | O_CREAT | O_APPEND, 0666);

    // Check if the archive file opening was successful
    if (archive == -1) {
        perror("Error opening or creating archive file");
        return -1;  // Return an error code
    }

    // Write metadata to the archive file
    write(archive, &file_metadata, sizeof(file_metadata));

    // Close the archive file
    close(archive);

    // Ensure add_metadata was successful
    if (file_metadata.number_of_files == 0 && file_metadata.file_sizes[0] == 0 ) {
        fprintf(stderr, "Error getting metadata from directory\n");
        return -1;  // Return an error code
    }

    // Iterate through each file and write its content to the archive file
    for (int i = 0; i < file_metadata.number_of_files; i++) {
        int result = write_file_to_archive(file_metadata.filenames[i], archive_filename);

        // Ensure write_file_to_archive was successful
        if (result == -1) {
            fprintf(stderr, "Error writing file to archive\n");
            return -1;  // Return an error code
        }
    }

    // Return 0 to indicate success
    return 0;
}
