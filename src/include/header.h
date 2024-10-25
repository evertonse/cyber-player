#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define REC_FMT "{ %.2f, %.2f, %.2f, %.2f }"
#define REC_ARG(rec)  (rec).x, (rec).y, (rec).width, (rec).height


#define MAX_FILENAME_LENGTH 2048

void player_unpause(void *ctx);

void player_set_volume(void *ctx, double volume);

void player_pause(void *ctx);

void player_load_file(void *ctx, const char *file_path);


typedef struct {
    double start;
    double end;
} Segment;

typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    Segment* segments;
    int segment_count;
    double volume;
} FileProgress;


// Function to write the data to a binary file
void store_progress_data(const char* filename, FileProgress* progress_array, int array_size);

// Function to read the data from a binary file
int load_progress_data(const char* filename, FileProgress** progress_array);

void print_all_file_progress();

