/* memory_bucket_optimizer.c
 *
 * Copyright (C) 2025 wolfSSL Inc.
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
#include <ctype.h>

#define MAX_ALLOC_SIZES 1000
#define MAX_LINE_LENGTH 1024
#define MAX_BUCKETS 16

typedef struct {
    int size;
    int count;
} AllocSize;

/* Function to parse memory allocation logs */
int parse_memory_logs(const char* filename, AllocSize* alloc_sizes, int* num_sizes) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    *num_sizes = 0;
    
    while (fgets(line, sizeof(line), file)) {
        /* Look for lines starting with "Alloc:" */
        if (strncmp(line, "Alloc:", 6) == 0) {
            int size;
            /* Try to extract the size from the line */
            /* Format: Alloc: 0x55fde046b490 -> 4 (11) at wolfTLSv1_3_client_method_ex:src/tls.c:16714 */
            if (sscanf(line, "Alloc: %*s -> %d", &size) == 1) {
                /* Check if this size already exists in our array */
                int i;
                for (i = 0; i < *num_sizes; i++) {
                    if (alloc_sizes[i].size == size) {
                        alloc_sizes[i].count++;
                        break;
                    }
                }
                
                /* If not found, add it */
                if (i == *num_sizes) {
                    if (*num_sizes < MAX_ALLOC_SIZES) {
                        alloc_sizes[*num_sizes].size = size;
                        alloc_sizes[*num_sizes].count = 1;
                        (*num_sizes)++;
                    } else {
                        printf("Warning: Maximum number of allocation sizes reached\n");
                    }
                }
            }
        }
    }
    
    fclose(file);
    return 0;
}

/* Function to compare allocation sizes for sorting */
int compare_alloc_sizes(const void* a, const void* b) {
    return ((AllocSize*)a)->size - ((AllocSize*)b)->size;
}

/* Function to compare allocation counts for sorting (descending) */
int compare_alloc_counts(const void* a, const void* b) {
    return ((AllocSize*)b)->count - ((AllocSize*)a)->count;
}

/* Function to optimize bucket sizes */
void optimize_buckets(AllocSize* alloc_sizes, int num_sizes, int* buckets, int* dist, int* num_buckets) {
    /* Make a copy of the allocation sizes for frequency sorting */
    AllocSize* alloc_sizes_by_freq = (AllocSize*)malloc(num_sizes * sizeof(AllocSize));
    if (!alloc_sizes_by_freq) {
        printf("Error: Memory allocation failed\n");
        *num_buckets = 0;
        return;
    }
    
    memcpy(alloc_sizes_by_freq, alloc_sizes, num_sizes * sizeof(AllocSize));
    
    /* Sort by frequency (descending) */
    qsort(alloc_sizes_by_freq, num_sizes, sizeof(AllocSize), compare_alloc_counts);
    
    /* Select the most frequent allocation sizes as buckets (up to MAX_BUCKETS) */
    *num_buckets = (num_sizes < MAX_BUCKETS) ? num_sizes : MAX_BUCKETS;
    
    /* Copy the selected bucket sizes */
    for (int i = 0; i < *num_buckets; i++) {
        buckets[i] = alloc_sizes_by_freq[i].size;
        /* Distribution is based on frequency */
        dist[i] = (alloc_sizes_by_freq[i].count > 10) ? 8 : 
                 (alloc_sizes_by_freq[i].count > 5) ? 4 : 2;
    }
    
    /* Sort buckets by size (ascending) */
    for (int i = 0; i < *num_buckets - 1; i++) {
        for (int j = 0; j < *num_buckets - i - 1; j++) {
            if (buckets[j] > buckets[j + 1]) {
                /* Swap bucket sizes */
                int temp = buckets[j];
                buckets[j] = buckets[j + 1];
                buckets[j + 1] = temp;
                
                /* Swap distribution values */
                temp = dist[j];
                dist[j] = dist[j + 1];
                dist[j + 1] = temp;
            }
        }
    }
    
    free(alloc_sizes_by_freq);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <memory_log_file>\n", argv[0]);
        return 1;
    }
    
    printf("Warning: This tool is designed to work with wolfSSL's static memory feature.\n");
    printf("Please rebuild wolfSSL with --enable-staticmemory to use the optimized configurations.\n\n");
    
    AllocSize alloc_sizes[MAX_ALLOC_SIZES];
    int num_sizes = 0;
    
    /* Parse memory allocation logs */
    if (parse_memory_logs(argv[1], alloc_sizes, &num_sizes) != 0) {
        return 1;
    }
    
    printf("Found %d unique allocation sizes\n\n", num_sizes);
    
    /* Sort allocation sizes */
    qsort(alloc_sizes, num_sizes, sizeof(AllocSize), compare_alloc_sizes);
    
    /* Print allocation sizes and frequencies */
    printf("Allocation Sizes and Frequencies:\n");
    printf("Size    Count\n");
    printf("----    -----\n");
    for (int i = 0; i < num_sizes; i++) {
        printf("%-7d %d\n", alloc_sizes[i].size, alloc_sizes[i].count);
    }
    printf("\n");
    
    /* Optimize bucket sizes */
    int buckets[MAX_BUCKETS];
    int dist[MAX_BUCKETS];
    int num_buckets = 0;
    
    optimize_buckets(alloc_sizes, num_sizes, buckets, dist, &num_buckets);
    
    /* Print optimized bucket sizes and distribution */
    printf("Optimized Bucket Sizes and Distribution:\n");
    printf("Size    Count   Wasted  Dist\n");
    printf("----    -----   ------  ----\n");
    
    float total_waste = 0.0;
    for (int i = 0; i < num_buckets; i++) {
        /* Find the closest allocation size that fits in this bucket */
        int closest_size = 0;
        int closest_count = 0;
        for (int j = 0; j < num_sizes; j++) {
            if (alloc_sizes[j].size <= buckets[i] && 
                alloc_sizes[j].size > closest_size) {
                closest_size = alloc_sizes[j].size;
                closest_count = alloc_sizes[j].count;
            }
        }
        
        float waste = (float)(buckets[i] - closest_size);
        total_waste += waste * closest_count;
        
        printf("%-7d %-7d %-7.2f %d\n", buckets[i], closest_count, 
               waste, dist[i]);
    }
    
    printf("\nTotal waste: %.2f bytes\n\n", total_waste);
    
    /* Print WOLFMEM_BUCKETS and WOLFMEM_DIST macros */
    printf("WOLFMEM_BUCKETS and WOLFMEM_DIST Macros:\n");
    printf("#define WOLFMEM_BUCKETS ");
    for (int i = 0; i < num_buckets; i++) {
        printf("%d", buckets[i]);
        if (i < num_buckets - 1) {
            printf(",");
        }
    }
    printf("\n");
    
    printf("#define WOLFMEM_DIST ");
    for (int i = 0; i < num_buckets; i++) {
        printf("%d", dist[i]);
        if (i < num_buckets - 1) {
            printf(",");
        }
    }
    printf("\n");
    
    return 0;
}
