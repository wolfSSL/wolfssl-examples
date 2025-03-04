/* memory_bucket_optimizer.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include wolfSSL headers */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>

/* Check if static memory is enabled */
#ifdef WOLFSSL_STATIC_MEMORY
    #define STATIC_MEMORY_ENABLED 1
#else
    #define STATIC_MEMORY_ENABLED 0
    #warning "This tool is designed to work with wolfSSL's static memory feature."
    #warning "Please rebuild wolfSSL with --enable-staticmemory to use the optimized configurations."
#endif

/* Maximum number of unique allocation sizes to track */
#define MAX_ALLOC_SIZES 1024
/* Maximum number of buckets to create */
#define MAX_BUCKETS 16
/* Minimum bucket size */
#define MIN_BUCKET_SIZE 16
/* Maximum waste ratio before creating a new bucket */
#define MAX_WASTE_RATIO 0.2

/* Structure to track allocation sizes and frequencies */
typedef struct {
    size_t size;
    int frequency;
} AllocInfo;

/* Structure to hold bucket configuration */
typedef struct {
    size_t size;
    int count;
    size_t waste;
    int dist;
} BucketInfo;

/* Compare function for sorting allocation sizes */
static int compare_alloc_sizes(const void* a, const void* b) {
    return (int)(((AllocInfo*)a)->size - ((AllocInfo*)b)->size);
}

/* Compare function for sorting bucket sizes */
static int compare_bucket_sizes(const void* a, const void* b) {
    return (int)(((BucketInfo*)a)->size - ((BucketInfo*)b)->size);
}

/* Parse memory allocation logs and collect unique sizes */
static int parse_memory_logs(const char* filename, AllocInfo* allocs, 
                           int* num_allocs) {
    FILE* fp;
    char line[256];
    size_t size;
    int i;
    *num_allocs = 0;

    fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "Alloc: %zu", &size) == 1) {
            /* Check if size already exists */
            for (i = 0; i < *num_allocs; i++) {
                if (allocs[i].size == size) {
                    allocs[i].frequency++;
                    break;
                }
            }
            
            /* Add new size if not found */
            if (i == *num_allocs && *num_allocs < MAX_ALLOC_SIZES) {
                allocs[*num_allocs].size = size;
                allocs[*num_allocs].frequency = 1;
                (*num_allocs)++;
            }
        }
    }

    fclose(fp);
    
    /* Sort allocation sizes from smallest to largest */
    qsort(allocs, *num_allocs, sizeof(AllocInfo), compare_alloc_sizes);
    
    return 0;
}

/* Calculate optimal bucket sizes based on allocation patterns */
static int optimize_buckets(AllocInfo* allocs, int num_allocs, 
                          BucketInfo* buckets, int* num_buckets) {
    int i, j;
    size_t total_waste = 0;
    *num_buckets = 0;

    /* Initialize first bucket with smallest allocation size */
    if (num_allocs > 0) {
        buckets[0].size = allocs[0].size > MIN_BUCKET_SIZE ? 
                         allocs[0].size : MIN_BUCKET_SIZE;
        buckets[0].count = allocs[0].frequency;
        buckets[0].waste = 0;
        buckets[0].dist = 2; /* Start with small distribution */
        *num_buckets = 1;
    }

    /* Process remaining allocation sizes */
    for (i = 1; i < num_allocs; i++) {
        size_t best_waste = (size_t)-1;
        int best_bucket = -1;

        /* Find best existing bucket */
        for (j = 0; j < *num_buckets; j++) {
            if (allocs[i].size <= buckets[j].size) {
                size_t waste = (buckets[j].size - allocs[i].size) * 
                             allocs[i].frequency;
                if (waste < best_waste) {
                    best_waste = waste;
                    best_bucket = j;
                }
            }
        }

        /* Create new bucket if waste is too high */
        if (best_bucket == -1 || 
            (float)best_waste / (allocs[i].size * allocs[i].frequency) > 
            MAX_WASTE_RATIO) {
            if (*num_buckets < MAX_BUCKETS) {
                buckets[*num_buckets].size = allocs[i].size;
                buckets[*num_buckets].count = allocs[i].frequency;
                buckets[*num_buckets].waste = 0;
                buckets[*num_buckets].dist = 7; /* Default distribution */
                (*num_buckets)++;
            }
        }
        else {
            buckets[best_bucket].count += allocs[i].frequency;
            buckets[best_bucket].waste += best_waste;
            total_waste += best_waste;
        }
    }

    /* Sort buckets by size */
    qsort(buckets, *num_buckets, sizeof(BucketInfo), compare_bucket_sizes);

    /* Adjust distribution values based on usage patterns */
    for (i = 0; i < *num_buckets; i++) {
        if (buckets[i].count > 100) {
            buckets[i].dist = 8; /* High usage */
        }
        else if (buckets[i].count < 10) {
            buckets[i].dist = 2; /* Low usage */
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    AllocInfo allocs[MAX_ALLOC_SIZES];
    BucketInfo buckets[MAX_BUCKETS];
    int num_allocs = 0;
    int num_buckets = 0;
    int i;

    if (!STATIC_MEMORY_ENABLED) {
        printf("Warning: This tool is designed to work with wolfSSL's static "
               "memory feature.\n");
        printf("Please rebuild wolfSSL with --enable-staticmemory to use the "
               "optimized configurations.\n\n");
    }

    if (argc != 2) {
        printf("Usage: %s <memory_log_file>\n", argv[0]);
        printf("\nExample:\n");
        printf("  1. Build wolfSSL with static memory and logging:\n");
        printf("     cd ../../../wolfssl\n");
        printf("     ./configure --enable-staticmemory --enable-memorylog\n");
        printf("     make\n\n");
        printf("  2. Run the example client to generate memory logs:\n");
        printf("     ./examples/client/client -h google.com -d -p 443 -g\n\n");
        printf("  3. Run this tool with the memory log file:\n");
        printf("     %s memory_log.txt\n", argv[0]);
        return 1;
    }

    /* Parse memory allocation logs */
    if (parse_memory_logs(argv[1], allocs, &num_allocs) != 0) {
        return 1;
    }

    printf("Found %d unique allocation sizes\n\n", num_allocs);

    /* Print allocation sizes and frequencies */
    printf("Allocation Sizes and Frequencies:\n");
    printf("Size    Count\n");
    printf("----    -----\n");
    for (i = 0; i < num_allocs; i++) {
        printf("%-8zu %d\n", allocs[i].size, allocs[i].frequency);
    }
    printf("\n");

    /* Optimize bucket sizes */
    if (optimize_buckets(allocs, num_allocs, buckets, &num_buckets) != 0) {
        return 1;
    }

    /* Print optimized bucket configuration */
    printf("Optimized Bucket Sizes and Distribution:\n");
    printf("Size    Count   Waste   Dist\n");
    printf("----    -----   -----   ----\n");
    for (i = 0; i < num_buckets; i++) {
        printf("%-8zu %-7d %-7zu %d\n", 
               buckets[i].size, buckets[i].count, 
               buckets[i].waste, buckets[i].dist);
    }
    printf("\n");

    /* Print WOLFMEM_BUCKETS and WOLFMEM_DIST macros */
    printf("#define WOLFMEM_BUCKETS ");
    for (i = 0; i < num_buckets; i++) {
        printf("%zu%s", buckets[i].size, 
               i < num_buckets - 1 ? "," : "\n");
    }

    printf("#define WOLFMEM_DIST ");
    for (i = 0; i < num_buckets; i++) {
        printf("%d%s", buckets[i].dist, 
               i < num_buckets - 1 ? "," : "\n");
    }

    return 0;
}
