#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



#define MAX_FILENAME_LENGTH 1024

typedef struct {
    double start;
    double end;
} Segment;

typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    Segment* segments;
    int segment_count;
} FileProgress;


// Function to write the data to a binary file
void store_progress_data(const char* filename, FileProgress* progress_array, int array_size);

// Function to read the data from a binary file
int load_progress_data(const char* filename, FileProgress** progress_array);

