
/* The struct format I'm going to be using for this project */

/* Struct includes metadata for the directory to be archived

- the name of the directory being archived
- an array of file names
- an array containing file sizes
- the number of files in the directory

*/ 

struct meta_data_for_directory
{
    char directory_name[32];
    char filenames[100][32];
    off_t file_sizes[100];
    int number_of_files;
}typedef meta_data_for_directory;
