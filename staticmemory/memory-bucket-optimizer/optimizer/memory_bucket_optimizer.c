/* memory_bucket_optimizer.c
 *
 * Copyright (C) 2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#include <limits.h> // Required for INT_MAX

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/memory.h>

#define MAX_ALLOC_SIZES 1000
#define MAX_LINE_LENGTH 1024
#define MAX_BUCKETS 16
#define MAX_ALLOCATIONS 10000

/* Memory overhead constants - these should match wolfSSL's actual values */
#define WOLFSSL_HEAP_SIZE 64      /* Approximate size of WOLFSSL_HEAP structure */
#define WOLFSSL_HEAP_HINT_SIZE 32 /* Approximate size of WOLFSSL_HEAP_HINT structure */

typedef struct {
    int size;
    int count;
    int max_concurrent;  // Maximum number of concurrent allocations of this size
} AllocSize;

typedef struct {
    int size;
    int timestamp;  // Simple counter for allocation order
    int active;     // 1 if allocated, 0 if freed
} AllocationEvent;

/* Function to calculate memory padding size per bucket */
int calculate_padding_size() {
    return wolfSSL_MemoryPaddingSz();
}

/* Function to calculate bucket size including padding */
int calculate_bucket_size_with_padding(int allocation_size) {
    return allocation_size + calculate_padding_size();
}

/* Function to calculate total memory overhead */
int calculate_total_overhead(int num_buckets) {
    /* Total overhead includes:
     * - WOLFSSL_HEAP structure
     * - WOLFSSL_HEAP_HINT structure  
     * - Alignment padding
     * Note: Padding is already included in bucket sizes
     */
    int total_overhead = WOLFSSL_HEAP_SIZE + 
                        WOLFSSL_HEAP_HINT_SIZE + 
                        (WOLFSSL_STATIC_ALIGN - 1);
    total_overhead += num_buckets * wolfSSL_MemoryPaddingSz();
    return total_overhead;
}

/* Function to parse memory allocation logs with concurrent usage tracking */
int parse_memory_logs(const char* filename, AllocSize* alloc_sizes, int* num_sizes) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    *num_sizes = 0;
    AllocationEvent events[MAX_ALLOCATIONS];
    int num_events = 0;
    int timestamp = 0;
    
    while (fgets(line, sizeof(line), file) && num_events < MAX_ALLOCATIONS) {
        /* Look for lines containing "Alloc:" or "Free:" */
        char* alloc_pos = strstr(line, "Alloc:");
        char* free_pos = strstr(line, "Free:");
        
        if (alloc_pos) {
            int size;
            /* Handle multiple formats:
             * Format 1: Alloc: 0x55fde046b490 -> 4 (11) at wolfTLSv1_3_client_method_ex:src/tls.c:16714
             * Format 2: [HEAP 0x1010e2110] Alloc: 0x101108a40 -> 1024 at simple_mem_test:18561
             * Format 3: (Using global heap hint 0x1010e2110) [HEAP 0x0] Alloc: 0x101107440 -> 1584 at _sp_exptmod_nct:14231
             */
            if (sscanf(alloc_pos, "Alloc: %*s -> %d", &size) == 1) {
                events[num_events].size = size;
                events[num_events].timestamp = timestamp++;
                events[num_events].active = 1;
                num_events++;
            }
        } else if (free_pos) {
            int size;
            /* Handle multiple formats:
             * Format 1: Free: 0x55fde046b490 -> 4 at wolfTLSv1_3_client_method_ex:src/tls.c:16714
             * Format 2: [HEAP 0x1010e2110] Free: 0x101108a40 -> 1024 at simple_mem_test:18576
             * Format 3: (Using global heap hint 0x1010e2110) [HEAP 0x0] Free: 0x101107440 -> 1584 at _sp_exptmod_nct:14462
             */
            if (sscanf(free_pos, "Free: %*s -> %d", &size) == 1) {
                events[num_events].size = size;
                events[num_events].timestamp = timestamp++;
                events[num_events].active = 0;
                num_events++;
            }
        }
    }
    
    fclose(file);
    
    /* Collect unique sizes from events */
    int unique_sizes[MAX_ALLOC_SIZES];
    int num_unique_sizes = 0;
    
    for (int i = 0; i < num_events; i++) {
        int size = events[i].size;
        int found = 0;
        
        /* Check if this size is already in our unique sizes list */
        for (int j = 0; j < num_unique_sizes; j++) {
            if (unique_sizes[j] == size) {
                found = 1;
                break;
            }
        }
        
        /* Add to unique sizes if not found */
        if (!found && num_unique_sizes < MAX_ALLOC_SIZES) {
            unique_sizes[num_unique_sizes++] = size;
        }
    }
    
    /* Calculate concurrent usage for each unique size */
    for (int s = 0; s < num_unique_sizes; s++) {
        int size = unique_sizes[s];
        int current_concurrent = 0;
        int max_concurrent = 0;
        int total_count = 0;
        
        for (int i = 0; i < num_events; i++) {
            if (events[i].size == size) {
                if (events[i].active) {
                    current_concurrent++;
                    total_count++;
                    if (current_concurrent > max_concurrent) {
                        max_concurrent = current_concurrent;
                    }
                } else {
                    current_concurrent--;
                    if (current_concurrent < 0) {
                        current_concurrent = 0; /* Handle mismatched free/alloc */
                    }
                }
            }
        }
        
        /* Only add sizes that were actually allocated */
        if (total_count > 0) {
            if (*num_sizes < MAX_ALLOC_SIZES) {
                alloc_sizes[*num_sizes].size = size;
                alloc_sizes[*num_sizes].count = total_count;
                alloc_sizes[*num_sizes].max_concurrent = max_concurrent;
                (*num_sizes)++;
            } else {
                printf("Warning: Maximum number of allocation sizes reached\n");
                break;
            }
        }
    }
    
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
void optimize_buckets(AllocSize* alloc_sizes, int num_sizes, int* buckets,
    int* dist, int* num_buckets)
{
    int i, j;

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
    
    /* Find the largest allocation size */
    int largest_size = 0;
    for (i = 0; i < num_sizes; i++) {
        if (alloc_sizes[i].size > largest_size) {
            largest_size = alloc_sizes[i].size;
        }
    }
    
    /* Calculate total allocations and find significant sizes */
    int total_allocations = 0;
    for (i = 0; i < num_sizes; i++) {
        total_allocations += alloc_sizes[i].count;
    }
    
    /* Determine significant allocation sizes (those that represent >1% of total allocations) */
    int significant_sizes[MAX_BUCKETS];
    int num_significant = 0;
    int min_threshold = total_allocations / 100; /* 1% threshold */
    
    for (i = 0; i < num_sizes && num_significant < MAX_BUCKETS - 1; i++) {
        if (alloc_sizes_by_freq[i].count >= min_threshold) {
            significant_sizes[num_significant++] = alloc_sizes_by_freq[i].size;
        }
    }
    
    /* Initialize bucket count */
    *num_buckets = 0;
    
    /* Always include the largest allocation size first (with padding) */
    buckets[*num_buckets] = calculate_bucket_size_with_padding(largest_size);
    
    /* Find the largest size in our data to get its concurrent usage */
    int largest_concurrent = 1; /* Default to 1 */
    for (i = 0; i < num_sizes; i++) {
        if (alloc_sizes[i].size == largest_size) {
            largest_concurrent = alloc_sizes[i].max_concurrent;
            break;
        }
    }
    
    /* Use actual concurrent usage, but minimum of 1 */
    dist[*num_buckets] = (largest_concurrent > 0) ? largest_concurrent : 1;
    (*num_buckets)++;
    
    /* Add significant allocation sizes, avoiding duplicates */
    for (i = 0; i < num_significant && *num_buckets < MAX_BUCKETS; i++) {
        int size = significant_sizes[i];
        
        /* Skip if this size is already included (like the largest size) */
        int already_included = 0;
        for (j = 0; j < *num_buckets; j++) {
            /* Compare original allocation sizes, not bucket sizes with padding */
            int bucket_data_size = buckets[j] - calculate_padding_size();
            if (bucket_data_size == size) {
                already_included = 1;
                break;
            }
        }
        
        if (!already_included) {
            buckets[*num_buckets] = calculate_bucket_size_with_padding(size);
            
            /* Calculate distribution based on concurrent usage and frequency */
            int count = 0;
            int concurrent = 1; /* Default to 1 */
            for (j = 0; j < num_sizes; j++) {
                if (alloc_sizes[j].size == size) {
                    count = alloc_sizes[j].count;
                    concurrent = alloc_sizes[j].max_concurrent;
                    break;
                }
            }
            
            /* Use concurrent usage as base, but scale based on frequency */
            int base_dist = (concurrent > 0) ? concurrent : 1;
            
            /* Scale distribution based on frequency */
            if (count > total_allocations / 10) {        /* >10% of allocations */
                dist[*num_buckets] = base_dist * 2; /* Double for high frequency */
            } else if (count > total_allocations / 20) { /* >5% of allocations */
                dist[*num_buckets] = base_dist + 2; /* Add 2 for medium frequency */
            } else if (count > total_allocations / 50) { /* >2% of allocations */
                dist[*num_buckets] = base_dist + 1; /* Add 1 for low frequency */
            } else {
                dist[*num_buckets] = base_dist; /* Use concurrent usage as is */
            }
            
            /* Cap distribution at reasonable maximum */
            if (dist[*num_buckets] > 16) {
                dist[*num_buckets] = 16;
            }
            
            (*num_buckets)++;
        }
    }
    
    /* If we still have space, add some medium-frequency sizes */
    if (*num_buckets < MAX_BUCKETS) {
        for (i = 0; i < num_sizes && *num_buckets < MAX_BUCKETS; i++) {
            int size = alloc_sizes_by_freq[i].size;
            
            /* Skip if already included */
            int already_included = 0;
            for (j = 0; j < *num_buckets; j++) {
                /* Compare original allocation sizes, not bucket sizes with padding */
                int bucket_data_size = buckets[j] - calculate_padding_size();
                if (bucket_data_size == size) {
                    already_included = 1;
                    break;
                }
            }
            
            if (!already_included && alloc_sizes_by_freq[i].count >= 3) {
                buckets[*num_buckets] = calculate_bucket_size_with_padding(size);
                dist[*num_buckets] = 2; /* Default to 2 buckets for medium frequency */
                (*num_buckets)++;
            }
        }
    }
    
    /* Sort buckets by size (ascending) */
    for (i = 0; i < *num_buckets - 1; i++) {
        for (j = 0; j < *num_buckets - i - 1; j++) {
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

/* Function to calculate memory efficiency metrics */
void calculate_memory_efficiency(AllocSize* alloc_sizes, int num_sizes,
    int* buckets, int* dist, int num_buckets)
{
    int i, j;
    float total_waste = 0.0;
    int total_allocations = 0;
    int allocations_handled = 0;
    
    printf("Memory Efficiency Analysis:\n");
    printf("Size    Count   Concurrent Bucket   Waste   Coverage\n");
    printf("----    -----   ---------- ------   -----   --------\n");
    
    for (i = 0; i < num_sizes; i++) {
        int size = alloc_sizes[i].size;
        int count = alloc_sizes[i].count;
        total_allocations += count;
        
        /* Find the smallest bucket that can fit this allocation */
        int best_bucket = -1;
        int min_waste = INT_MAX;
        
        for (j = 0; j < num_buckets; j++) {
            /* Bucket sizes now include padding, so we need to subtract it for comparison */
            int bucket_data_size = buckets[j] - calculate_padding_size();
            if (bucket_data_size >= size) {
                int waste = bucket_data_size - size;
                if (waste < min_waste) {
                    min_waste = waste;
                    best_bucket = j;
                }
            }
        }
        
        if (best_bucket >= 0) {
            allocations_handled += count;
            total_waste += (float)min_waste * count;
            printf("%-7d %-7d %-10d %-7d %-7d %s\n", 
                   size, count, alloc_sizes[i].max_concurrent, buckets[best_bucket], min_waste, "✓");
        } else {
            printf("%-7d %-7d %-10d %-7s %-7s %s\n", 
                   size, count, alloc_sizes[i].max_concurrent, "N/A", "N/A", "✗");
        }
    }
    
    printf("\nEfficiency Summary:\n");
    printf("Total allocations: %d\n", total_allocations);
    printf("Allocations handled: %d (%.1f%%)\n", 
           allocations_handled, (float)allocations_handled * 100 / total_allocations);
    printf("Total memory waste: %.2f bytes\n", total_waste);
    printf("Average waste per allocation: %.2f bytes\n", 
           total_waste / total_allocations);
    
    /* Calculate total memory needed for buckets */
    int total_bucket_memory = 0;
    for (i = 0; i < num_buckets; i++) {
        total_bucket_memory += buckets[i] * dist[i];
    }
    
    /* Calculate total overhead */
    int total_overhead = calculate_total_overhead(num_buckets);
    int total_memory_needed = total_bucket_memory + total_overhead;
    
    printf("Total bucket memory: %d bytes\n", total_bucket_memory);
    printf("Memory overhead: %d bytes\n", total_overhead);
    printf("  - Padding per bucket: %d bytes (included in bucket sizes)\n", calculate_padding_size());
    printf("  - Total padding: %d bytes (included in bucket sizes)\n", calculate_padding_size() * num_buckets);
    printf("  - Heap structures: %d bytes\n", WOLFSSL_HEAP_SIZE + WOLFSSL_HEAP_HINT_SIZE);
    printf("  - Alignment: %d bytes\n", WOLFSSL_STATIC_ALIGN - 1);
    printf("Total memory needed: %d bytes\n", total_memory_needed);
    
    /* Calculate efficiency based on actual data vs total memory */
    float data_memory = 0;
    for (i = 0; i < num_sizes; i++) {
        data_memory += alloc_sizes[i].size * alloc_sizes[i].count;
    }
    printf("Data memory: %.0f bytes\n", data_memory);
}

/* Function to provide buffer size recommendations */
void print_buffer_recommendations(int* buckets, int* dist, int num_buckets)
{
    int total_bucket_memory = 0, total_overhead, total_memory_needed, i;

    for (i = 0; i < num_buckets; i++) {
        total_bucket_memory += buckets[i] * dist[i];
    }
    
    total_overhead = calculate_total_overhead(num_buckets);
    total_memory_needed = total_bucket_memory + total_overhead;
    
    printf("\nBuffer Size Recommendations:\n");
    printf("============================\n");
    printf("Minimum buffer size needed: %d bytes\n", total_memory_needed);
    printf("Recommended buffer size: %d bytes (add 10%% safety margin)\n", 
           (int)(total_memory_needed * 1.1));
    
    printf("\nUsage in wolfSSL application:\n");
    printf("============================\n");
    printf("// Allocate buffer\n");
    printf("byte staticBuffer[%d];\n", total_memory_needed);
    printf("\n// Load static memory\n");
    printf("WOLFSSL_HEAP_HINT* heapHint = NULL;\n");
    printf("if (wc_LoadStaticMemory_ex(&heapHint, %d, bucket_sizes, bucket_dist,\n", num_buckets);
    printf("    staticBuffer, %d, 0, 0) != 0) {\n", total_memory_needed);
    printf("    // Handle error\n");
    printf("}\n");
    printf("\n// Use in wolfSSL context\n");
    printf("wolfSSL_CTX_load_static_memory(&method, NULL, staticBuffer,\n");
    printf("    %d, 0, 1);\n", total_memory_needed);
}

int main(int argc, char** argv)
{
    int i;

    if (argc != 2) {
        printf("Usage: %s <memory_log_file>\n", argv[0]);
        return 1;
    }
    
    AllocSize alloc_sizes[MAX_ALLOC_SIZES];
    int num_sizes = 0;
    
    /* Parse memory allocation logs */
    if (parse_memory_logs(argv[1], alloc_sizes, &num_sizes) != 0) {
        return 1;
    }
    
    printf("Found %d unique allocation sizes\n\n", num_sizes);
    
    /* Sort allocation sizes */
    qsort(alloc_sizes, num_sizes, sizeof(AllocSize), compare_alloc_sizes);
    
    /* Print allocation sizes, frequencies, and concurrent usage */
    printf("Allocation Sizes, Frequencies, and Concurrent Usage:\n");
    printf("Size    Count   Max Concurrent\n");
    printf("----    -----   --------------\n");
    for (i = 0; i < num_sizes; i++) {
        printf("%-7d %-7d %d\n", alloc_sizes[i].size, alloc_sizes[i].count, alloc_sizes[i].max_concurrent);
    }
    printf("\n");
    
    /* Optimize bucket sizes */
    int buckets[MAX_BUCKETS];
    int dist[MAX_BUCKETS];
    int num_buckets = 0;
    
    optimize_buckets(alloc_sizes, num_sizes, buckets, dist, &num_buckets);
    
    /* Print optimized bucket sizes and distribution */
    printf("Optimized Bucket Sizes and Distribution:\n");
    printf("Data Size + Padding = Bucket Size    Dist\n");
    printf("----------------------------------------\n");
    
    for (i = 0; i < num_buckets; i++) {
        int data_size = buckets[i] - calculate_padding_size();
        printf("%-7d + %-7d = %-7d        %d\n", 
               data_size, calculate_padding_size(), buckets[i], dist[i]);
    }
    printf("\n");
    
    /* Print WOLFMEM_BUCKETS and WOLFMEM_DIST macros */
    printf("WOLFMEM_BUCKETS and WOLFMEM_DIST Macros:\n");
    printf("#define WOLFMEM_BUCKETS ");
    for (i = 0; i < num_buckets; i++) {
        printf("%d", buckets[i]);
        if (i < num_buckets - 1) {
            printf(",");
        }
    }
    printf("\n");
    
    printf("#define WOLFMEM_DIST ");
    for (i = 0; i < num_buckets; i++) {
        printf("%d", dist[i]);
        if (i < num_buckets - 1) {
            printf(",");
        }
    }
    printf("\n");

    /* Calculate and print memory efficiency metrics */
    calculate_memory_efficiency(alloc_sizes, num_sizes, buckets, dist,
        num_buckets);
    
    /* Print buffer size recommendations */
    print_buffer_recommendations(buckets, dist, num_buckets);
    
    return 0;
}
