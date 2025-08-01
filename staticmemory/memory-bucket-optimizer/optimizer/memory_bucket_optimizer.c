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

#define MAX_LINE_LENGTH 1024
#define MAX_UNIQUE_BUCKETS 9  /* Maximum number of unique bucket sizes to create */

/* Configuration notes:
 * - MAX_UNIQUE_BUCKETS: Limits the total number of unique bucket sizes
 *   This helps control memory overhead and bucket management complexity
 *   Default: 9 buckets (can be adjusted based on memory constraints)
 */

/* Memory overhead constants - these should match wolfSSL's actual values */
#define WOLFSSL_HEAP_SIZE 64      /* Approximate size of WOLFSSL_HEAP structure */
#define WOLFSSL_HEAP_HINT_SIZE 32 /* Approximate size of WOLFSSL_HEAP_HINT structure */

/* Linked list node for allocation events */
typedef struct AllocationEventNode {
    int size;
    int timestamp;  // Simple counter for allocation order
    int active;     // 1 if allocated, 0 if freed
    struct AllocationEventNode* next;
} AllocationEventNode;
AllocationEventNode* event_head = NULL;

/* Linked list node for unique allocation sizes */
typedef struct AllocSizeNode {
    int size;
    int count;
    int concurrent;
    int max_concurrent;  // Maximum number of concurrent allocations of this size
    struct AllocSizeNode* next; /* next in list of sizes sorted by size */
    struct AllocSizeNode* nextFreq; /* sorted by count size descending */
} AllocSizeNode;

/* Linked list for allocation events */
typedef struct {
    AllocationEventNode* head;
    AllocationEventNode* tail;
    int count;
} AllocationEventList;

/* Linked list for unique allocation sizes */
typedef struct {
    AllocSizeNode* head;
    int count;
} AllocSizeList;

/* Helper functions for linked lists */
AllocationEventNode* create_allocation_event_node(int size, int timestamp, int active) {
    AllocationEventNode* node = (AllocationEventNode*)malloc(sizeof(AllocationEventNode));
    if (node) {
        node->size = size;
        node->timestamp = timestamp;
        node->active = active;
        node->next = NULL;
    }
    return node;
}

void add_allocation_event(AllocationEventNode** list, int size, int timestamp, int active) {
    AllocationEventNode* node = create_allocation_event_node(size, timestamp, active);
    if (node) {
        if (*list == NULL) {
            event_head = node;
            *list = node;
        } else {
            (*list)->next = node;
            *list = node;  // Update the list pointer to point to the new node
        }
    }
}

AllocSizeNode* create_alloc_size_node(int size) {
    AllocSizeNode* node = (AllocSizeNode*)malloc(sizeof(AllocSizeNode));
    if (node) {
        node->size = size;
        node->count = 0;
        node->concurrent = 0;
        node->max_concurrent = 0;
        node->next = NULL;
        node->nextFreq = NULL;
    }
    return node;
}

AllocSizeNode* find_or_create_alloc_size(AllocSizeNode** list, int size) {
    AllocSizeNode* current = *list;
    AllocSizeNode* previous = NULL;
    
    /* Look for existing size */
    while (current) {
        if (current->size == size) {
            return current;
        }
        current = current->next;
    }
    
    /* Create new size node */
    AllocSizeNode* node = create_alloc_size_node(size);
    if (node) {
        /* insert node into list ordered from largest size first to smallest */
        current  = *list;
        if (current == NULL) {
            *list = node;
        }
        else {
            while (current != NULL) {
                if (current->size < size) {
                    node->next = current;
                    if (previous != NULL) {
                        previous->next = node;
                    }
                    else {
                        *list = node;
                    }
                    break;
                }
                previous = current;
                current = current->next;
            }
            /* If we reached the end of the list, append the node */
            if (current == NULL) {
                previous->next = node;
            }
        }
    }
    return node;
}

void free_allocation_event_list(AllocationEventNode* list) {
    AllocationEventNode* current = list;
    while (current) {
        AllocationEventNode* next = current->next;
        free(current);
        current = next;
    }
}

void free_alloc_size_list(AllocSizeNode* list) {
    AllocSizeNode* current = list;
    while (current) {
        AllocSizeNode* next = current->next;
        free(current);
        current = next;
    }
}

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
int parse_memory_logs(const char* filename, AllocationEventNode** events,
    int* peak_heap_usage, int* buckets)
{
    int current_heap_usage = 0;
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int timestamp = 0;
    *peak_heap_usage = 0; /* Initialize peak heap usage */
    
    while (fgets(line, sizeof(line), file)) {
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
                /* Here we begin the bucket list, as a simple tracking of
                 * largest allocs encountered. */
                int i;
                for (i = 0; i < MAX_UNIQUE_BUCKETS; i++) {
                    if (size > buckets[i]) {
                        buckets[i] = size;
                        break;
                    }
                }
                current_heap_usage += size;
                if (current_heap_usage > *peak_heap_usage) {
                    *peak_heap_usage = current_heap_usage;
                }
                add_allocation_event(events, size, timestamp++, 1);
            }
        } else if (free_pos) {
            int size;
            /* Handle multiple formats:
             * Format 1: Free: 0x55fde046b490 -> 4 at wolfTLSv1_3_client_method_ex:src/tls.c:16714
             * Format 2: [HEAP 0x1010e2110] Free: 0x101108a40 -> 1024 at simple_mem_test:18576
             * Format 3: (Using global heap hint 0x1010e2110) [HEAP 0x0] Free: 0x101107440 -> 1584 at _sp_exptmod_nct:14462
             */
            if (sscanf(free_pos, "Free: %*s -> %d", &size) == 1) {
                current_heap_usage -= size;
                if (current_heap_usage < 0) {
                    current_heap_usage = 0;
                }
                add_allocation_event(events, size, timestamp++, 0);
            }
        }
    }
    
    fclose(file);
    return 0;
}


/* This goes through all the events and finds unique allocations and the max
 * concurent use of each unique allocation */
static void find_max_concurent_allocations(AllocSizeNode** alloc_sizes)
{
    AllocationEventNode* current = event_head;
    while (current != NULL) {
        if (current->active) {
            AllocSizeNode* alloc_size = find_or_create_alloc_size(alloc_sizes, current->size);
            alloc_size->concurrent++;
            alloc_size->count++;
            if (alloc_size->max_concurrent < alloc_size->concurrent) {
                alloc_size->max_concurrent = alloc_size->concurrent;
            }
        }
        else {
            AllocSizeNode* alloc_size = find_or_create_alloc_size(alloc_sizes, current->size);
            alloc_size->concurrent--;
        }
        current = current->next;
    }
}


/* This function makes sure that for every alloc there is a bucket avilable */
static void set_distributions(int* buckets, int* dist, int num_buckets)
{
    AllocationEventNode* current = event_head;
    int max_concurrent_use[num_buckets];
    int current_use[num_buckets];
    int i;

    /* Initialize arrays to zero */
    memset(max_concurrent_use, 0, sizeof(max_concurrent_use));
    memset(current_use, 0, sizeof(current_use));

    while (current != NULL) {
        /* find bucket this would go in */
        for (i = 0; i < num_buckets; i++) {
            if (current->size <= (buckets[i] - wolfSSL_MemoryPaddingSz())) {
                break;
            }
        }

        /* Only process if we found a valid bucket */
        if (i < num_buckets) {
            if (current->active) {
                current_use[i] += 1;
                if (current_use[i] > max_concurrent_use[i]) {
                    max_concurrent_use[i] = current_use[i];
                }
            }
            else {
                current_use[i] -= 1;
            }
        } else {
            printf("ERROR: allocation size %d is larger than all bucket sizes!\n",
                current->size);
            printf("This indicates a bug in the bucket optimization algorithm.\n");
            printf("Largest bucket size: %d, allocation size: %d\n", 
                   num_buckets > 0 ? buckets[num_buckets-1] : 0, current->size);
            exit(1);
        }
        current = current->next;
    }

    for (i = 0; i < num_buckets; i++) {
        dist[i] = max_concurrent_use[i];
    }
}

static void sort_alloc_by_frequency(AllocSizeNode* alloc_sizes,
    AllocSizeNode** sorted)
{
    AllocSizeNode* max;
    AllocSizeNode* current;
    AllocSizeNode* tail;
    int current_count = 0;
    int current_upper_bound = INT_MAX;

    *sorted = NULL;  // Initialize to NULL
    
    do {
        max = NULL;
        current = alloc_sizes;  // Reset current to beginning of list
        while (current != NULL) {
            if (current->count > current_count && current->size < current_upper_bound) {
                current_count = current->count;
                max = current;
            }
            current = current->next;
        }

        if (max == NULL) {
            break;  // No more nodes to process
        }

        current_upper_bound = max->size;
        if (*sorted == NULL) {
            *sorted = max;
            tail    = max;
        }
        else {
            tail->nextFreq = max;
            tail           = max;
        }
        tail->nextFreq = NULL;
    } while (max != NULL);
}

/* Function to optimize bucket sizes */
/* 
 * Optimization heuristic:
 * - Always include the largest allocation size
 * - For other sizes, only create a new bucket if the waste from using
 *   existing buckets is >= padding size per bucket
 * - This reduces bucket management overhead when waste is minimal
 * - Limited to MAX_UNIQUE_BUCKETS total unique bucket sizes
 */
void optimize_buckets(AllocSizeNode* alloc_sizes, AllocSizeNode* alloc_sizes_by_freq,
    int num_sizes, int* buckets, int* dist, int* num_buckets)
{
    int i, j;
    AllocSizeNode* current;
    
    /* Calculate total allocations and find significant sizes */
    int total_allocations = 0;
    current = alloc_sizes;
    while (current != NULL) {
        total_allocations += current->count;
        current = current->next;
    }
    
    /* Determine significant allocation sizes (those that represent >1% of total allocations) */
    int significant_sizes[MAX_UNIQUE_BUCKETS];
    int num_significant = 0;
    int min_threshold = total_allocations / 100; /* 1% threshold */
    
    /* Populate significant sizes from alloc_sizes */
    current = alloc_sizes;
    while (current != NULL && num_significant < MAX_UNIQUE_BUCKETS) {
        if (current->count >= min_threshold) {
            significant_sizes[num_significant++] = current->size;
        }
        current = current->next;
    }

    /* Initialize bucket count */
    *num_buckets = 0;
    
    /* Always include the largest allocation sizes (with padding) */
    current = alloc_sizes;
    for (i = 0; i < MAX_UNIQUE_BUCKETS/2 && current != NULL; i++) {
        buckets[*num_buckets] = calculate_bucket_size_with_padding(current->size);
        dist[*num_buckets] = current->max_concurrent;
        (*num_buckets)++;
        current = current->next;
    }
    
    /* Fill out the other half based on max concurent use */
    for (i = *num_buckets; i < MAX_UNIQUE_BUCKETS; i++) {
        int max_concurrent = 0;
        AllocSizeNode* max = NULL;

        current = alloc_sizes;
        while (current != NULL) {
            if (current->max_concurrent > max_concurrent) {
                /* Skip if already included */
                int already_included = 0;
                for (j = 0; j < *num_buckets; j++) {
                    /* Compare original allocation sizes, not bucket sizes with padding */
                    int bucket_data_size = buckets[j] - calculate_padding_size();
                    if (bucket_data_size == current->size) {
                        already_included = 1;
                        break;
                    }
                }
                if (!already_included) {
                    max_concurrent = current->max_concurrent;
                    max = current;
                }
            }
            current = current->next;
        }
        if (max != NULL) {
            buckets[*num_buckets] = calculate_bucket_size_with_padding(max->size);
            dist[*num_buckets] = max->max_concurrent;
            *num_buckets += 1;
        }
        else {
            break;
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
    
    set_distributions(buckets, dist, *num_buckets);
    
    /* Print optimization summary */
    printf("Optimization Summary:\n");
    printf("Padding size per bucket: %d bytes\n", calculate_padding_size());
    printf("Maximum unique buckets allowed: %d\n", MAX_UNIQUE_BUCKETS);
    printf("Total buckets created: %d\n", *num_buckets);
    if (*num_buckets >= MAX_UNIQUE_BUCKETS) {
        printf("Note: Reached maximum bucket limit (%d). Some allocations may use larger buckets.\n", MAX_UNIQUE_BUCKETS);
    }
    printf("Note: Allocations with waste < padding size use existing buckets to reduce overhead\n");
    printf("Note: Bucket limit helps balance memory efficiency vs. management overhead\n\n");
}

/* Function to calculate memory efficiency metrics */
void calculate_memory_efficiency(AllocSizeNode* alloc_sizes, int num_sizes,
    int* buckets, int* dist, int num_buckets)
{
    AllocSizeNode* current = alloc_sizes;
    int i, j;
    float total_waste = 0.0;
    int total_allocations = 0;
    int allocations_handled = 0;
    int padding_size = calculate_padding_size();
    
    printf("Memory Efficiency Analysis:\n");
    printf("Note: Allocations with waste < %d bytes (padding size) use existing buckets\n", padding_size);
    printf("Size    Count   Concurrent Bucket   Waste   Coverage\n");
    printf("----    -----   ---------- ------   -----   --------\n");
    
    for (i = 0; i < num_sizes && current != NULL; i++) {
        int size = current->size;
        int count = current->count;
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
                   size, count, current->max_concurrent, buckets[best_bucket], min_waste, "✓");
        } else {
            printf("%-7d %-7d %-10d %-7s %-7s %s\n", 
                   size, count, current->max_concurrent, "N/A", "N/A", "✗");
        }
        current = current->next;
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
    int total_num_buckets = 0;
    for (i = 0; i < num_buckets; i++) {
        total_bucket_memory += buckets[i] * dist[i];
        total_num_buckets += dist[i];
    }
    
    /* Calculate total overhead */
    int total_overhead = calculate_total_overhead(total_num_buckets);
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
    current = alloc_sizes;
    for (i = 0; i < num_sizes && current != NULL; i++) {
        data_memory += current->size * current->count;
        current = current->next;
    }
    printf("Data memory: %.0f bytes\n", data_memory);
}

/* Function to provide buffer size recommendations */
void print_buffer_recommendations(int* buckets, int* dist, int num_buckets)
{
    int total_bucket_memory = 0, total_overhead = 0, total_memory_needed, i;

    for (i = 0; i < num_buckets; i++) {
        total_bucket_memory += buckets[i] * dist[i];
        total_overhead += dist[i] * wolfSSL_MemoryPaddingSz();
    }

    total_overhead += sizeof(WOLFSSL_HEAP_HINT) + sizeof(WOLFSSL_HEAP) +
        WOLFSSL_STATIC_ALIGN;
    total_memory_needed = total_bucket_memory + total_overhead;

    printf("\nBuffer Size Recommendations:\n");
    printf("============================\n");
    printf("Minimum buffer size needed: %d bytes\n", total_memory_needed);

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
    int buckets[MAX_UNIQUE_BUCKETS];
    int dist[MAX_UNIQUE_BUCKETS];
    int num_sizes = 0;
    int peak_heap_usage = 0;
    int num_buckets = 0;
    AllocationEventNode* events = NULL;
    AllocSizeNode* alloc_sizes = NULL;
    AllocSizeNode* alloc_sizes_by_freq = NULL;
    AllocSizeNode* current;

    if (argc != 2) {
        printf("Usage: %s <memory_log_file>\n", argv[0]);
        return 1;
    }
    
    /* Initialize buckets array to 0 */
    memset(buckets, 0, sizeof(buckets));
    
    /* Parse memory allocation logs */
    if (parse_memory_logs(argv[1], &events, &peak_heap_usage, buckets) != 0) {
        return 1;
    }
    

    find_max_concurent_allocations(&alloc_sizes);
    sort_alloc_by_frequency(alloc_sizes, &alloc_sizes_by_freq);

    current = alloc_sizes;
    while (current!= NULL) {
        num_sizes++;
        current = current->next;
    }

    printf("Found %d unique allocation sizes\n", num_sizes);
    printf("Peak heap usage: %d bytes (maximum concurrent memory usage)\n\n", peak_heap_usage);

    /* Print allocation sizes, frequencies, and concurrent usage */
    printf("Allocation Sizes, Frequencies, and Concurrent Usage:\n");
    printf("Size    Count   Max Concurrent\n");
    printf("----    -----   --------------\n");
    current = alloc_sizes;
    while (current != NULL) {
        printf("%-7d %-7d %d\n", current->size, current->count, current->max_concurrent);
        current = current->next;
    }
    printf("\n");
    
    /* Optimize bucket sizes */
    optimize_buckets(alloc_sizes, alloc_sizes_by_freq, num_sizes, buckets, dist, &num_buckets);
    
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
    
    /* Clean up events list */
    free_allocation_event_list(events);
    free_alloc_size_list(alloc_sizes);
    /* alloc_sizes_by_freq is the same nodes as alloc_sizes */

    return 0;
}
