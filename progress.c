#include <assert.h>

#include "header.h"
#include "stb_ds.h"

// Function to escape special characters in the filename
static void escape_filename(const char* input, char* output, size_t output_size) {
    size_t i, j;
    for (i = 0, j = 0; input[i] != '\0' && j < output_size - 1; i++, j++) {
        if (input[i] == '\\' || input[i] == '\0') {

            if (j >= output_size - 2) break;
            output[j++] = '\\';
        }
        output[j] = input[i];
    }
    output[j] = '\0';
}

// Function to unescape special characters in the filename
static void unescape_filename(const char* input, char* output, size_t output_size) {
    size_t i, j;
    for (i = 0, j = 0; input[i] != '\0' && j < output_size - 1; i++, j++) {
        if (input[i] == '\\') {

            i++;
            if (input[i] == '\0') break;
        }
        output[j] = input[i];
    }

    output[j] = '\0';
}


// Function to write the data to a binary file
void store_progress_data(const char* filename, FileProgress* progress_array, int array_size) {

    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file for writing");
        return;
    }

    // Write the number of FileProgress structures
    fwrite(&array_size, sizeof(int), 1, file);

    for (int i = 0; i < array_size; i++) {
        // Escape and write the filename
        char escaped_filename[MAX_FILENAME_LENGTH * 2];
        escape_filename(progress_array[i].filename, escaped_filename, sizeof(escaped_filename));
        uint16_t filename_length = strlen(escaped_filename);
        fwrite(&filename_length, sizeof(uint16_t), 1, file);
        fwrite(escaped_filename, sizeof(char), filename_length, file);

        // Write the number of segments
        fwrite(&progress_array[i].segment_count, sizeof(int), 1, file);

        // Write the segments
        fwrite(progress_array[i].segments, sizeof(Segment), progress_array[i].segment_count, file);
    }


    fclose(file);
}


// Function to read the data from a binary file
int load_progress_data(const char* filename, FileProgress** progress_array) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file for reading");
        return 0;

    }

    int array_size;
    fread(&array_size, sizeof(int), 1, file);

    // T* arraddnptr(T* a, int n)
    //   Appends n uninitialized items onto array at the end.
    //   Returns a pointer to the first uninitialized item added.

    *progress_array = arraddnptr(*progress_array, array_size);
    assert(arrlen(*progress_array) == array_size);
    // *progress_array = malloc(array_size * sizeof(FileProgress));
    if (!*progress_array) {
        perror("Memory allocation failed");
        fclose(file);
        return 0;
    }

    for (int i = 0; i < array_size; i++) {
        // Read and unescape the filename
        uint16_t filename_length;
        fread(&filename_length, sizeof(uint16_t), 1, file);
        char escaped_filename[MAX_FILENAME_LENGTH * 2];
        fread(escaped_filename, sizeof(char), filename_length, file);
        escaped_filename[filename_length] = '\0';
        unescape_filename(escaped_filename, (*progress_array)[i].filename, MAX_FILENAME_LENGTH);

        // Read the number of segments

        fread(&(*progress_array)[i].segment_count, sizeof(int), 1, file);

        Segment* segments = malloc((*progress_array)[i].segment_count * sizeof(Segment));
        // Read the segments
        fread(segments, sizeof(Segment), (*progress_array)[i].segment_count, file);
        (*progress_array)[i].segments = segments;
    }

    fclose(file);
    return array_size;
}


int test() {
    // Example usage
    Segment s1[] = {{5.0, 15.5}, {25.0, 35.5}, {40.0, 50.0}};
    Segment s2[] = {{0.0, 10.5}, {20.0, 30.5}};
    FileProgress progress_array[2] = {
        {
            .filename = "test\\file1.mp4",
            .segments = s1,
            .segment_count = 3
        },
        {
            .filename = "another file\\with\\backslashes.mp4",
            .segments = s2,
            .segment_count = 2
        }
    };

    const char* file = "db.bin";
    // Write data to file
    store_progress_data(file, progress_array, 2);

    // Read data from file
    FileProgress* read_array;
    int read_size = load_progress_data(file, &read_array);

    // Print read data
    for (int i = 0; i < read_size; i++) {

        printf("File: %s\n", read_array[i].filename);
        printf("Segments:\n");
        for (int j = 0; j < read_array[i].segment_count; j++) {
            printf("  %.2f - %.2f\n", read_array[i].segments[j].start, read_array[i].segments[j].end);
        }
        printf("\n");
    }

    // Free allocated memory
    free(read_array);

    return 0;
}


