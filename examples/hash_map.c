#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <stdint.h>
#include <time.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void normal_hash(void) {
    float f;
    char* a = "AB";
    char b[] = {'A', 'B', '\0'};
    char* c = malloc(1900);
    c[0] = 'A'; c[1] = 'B'; c[2] = '\0';

    struct { char* key; double value; } *hash = NULL;
    f=10.5; hmput(hash,a , f);
    f=20.4; hmput(hash,b , f);
    f=50.3; hmput(hash,c , f);

    for (int i=0; i < hmlen(hash); ++i)
      printf("%f ", hash[i].value);

    printf("\n");
    // fflush(stdout);
}

void string_hash(void) {
    float f;
    char* a = "AB";
    char b[] = {'A', 'B', '\0'};
    char* c = malloc(1900);
    c[0] = 'A'; c[1] = 'B'; c[2] = '\0';

    struct { char* key; double value; } *hash = NULL;
    f=10.5; shput(hash,a , f);
    f=20.4; shput(hash,b , f);
    f=50.3; shput(hash,c , f);

    for (int i=0; i < shlen(hash); ++i)
      printf("%f ", hash[i].value);

    printf("\n");
    // fflush(stdout);
}

int main(void) {
    normal_hash();
    string_hash();
}
