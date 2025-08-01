/* memory_bucket_tester.c
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/memory.h>

/* Memory overhead constants - these should match wolfSSL's actual values */
#ifndef WOLFSSL_STATIC_ALIGN
#define WOLFSSL_STATIC_ALIGN 8
#endif
#ifndef WOLFSSL_HEAP_SIZE
#define WOLFSSL_HEAP_SIZE 64      /* Approximate size of WOLFSSL_HEAP structure */
#endif
#ifndef WOLFSSL_HEAP_HINT_SIZE
#define WOLFSSL_HEAP_HINT_SIZE 32 /* Approximate size of WOLFSSL_HEAP_HINT structure */
#endif

#define MAX_ALLOCATIONS 10000
#define MAX_LINE_LENGTH 1024
#define MAX_BUCKETS 16

typedef struct {
    int size;
    int is_alloc;  /* 1 for alloc, 0 for free */
    void* ptr;     /* For tracking allocated pointers */
} AllocationEvent;

typedef struct {
    int size;
    int count;
} BucketConfig;

/* Function to parse bucket configuration from command line */
int parse_bucket_config(int argc, char** argv, int start_idx, 
                       BucketConfig* buckets, int* num_buckets, int* total_buffer_size) {
    if (start_idx >= argc) {
        printf("Error: No bucket configuration provided\n");
        return -1;
    }
    
    *num_buckets = 0;
    *total_buffer_size = 0;
    int i = start_idx;
    
    while (i < argc && *num_buckets < MAX_BUCKETS) {
        if (strcmp(argv[i], "--buckets") == 0) {
            i++;
            if (i < argc) {
                char* bucket_str = argv[i];
                char* token = strtok(bucket_str, ",");
                
                while (token != NULL && *num_buckets < MAX_BUCKETS) {
                    buckets[*num_buckets].size = atoi(token);
                    buckets[*num_buckets].count = 1; /* Default count */
                    (*num_buckets)++;
                    token = strtok(NULL, ",");
                }
                i++;
            }
        } else if (strcmp(argv[i], "--dist") == 0) {
            i++;
            if (i < argc) {
                char* dist_str = argv[i];
                char* token = strtok(dist_str, ",");
                int dist_idx = 0;
                
                while (token != NULL && dist_idx < *num_buckets) {
                    buckets[dist_idx].count = atoi(token);
                    dist_idx++;
                    token = strtok(NULL, ",");
                }
                i++;
            }
        } else if (strcmp(argv[i], "--buffer-size") == 0) {
            i++;
            if (i < argc) {
                *total_buffer_size = atoi(argv[i]);
                i++;
            }
        } else {
            i++;
        }
    }
    
    return 0;
}

/* Function to parse memory allocation logs */
int parse_memory_logs(const char* filename, AllocationEvent* events, int* num_events) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    *num_events = 0;
    
    while (fgets(line, sizeof(line), file) && *num_events < MAX_ALLOCATIONS) {
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
                events[*num_events].size = size;
                events[*num_events].is_alloc = 1;
                events[*num_events].ptr = NULL;
                (*num_events)++;
            }
        } else if (free_pos) {
            int size;
            /* Handle multiple formats:
             * Format 1: Free: 0x55fde046b490 -> 4 at wolfTLSv1_3_client_method_ex:src/tls.c:16714
             * Format 2: [HEAP 0x1010e2110] Free: 0x101108a40 -> 1024 at simple_mem_test:18576
             * Format 3: (Using global heap hint 0x1010e2110) [HEAP 0x0] Free: 0x101107440 -> 1584 at _sp_exptmod_nct:14462
             */
            if (sscanf(free_pos, "Free: %*s -> %d", &size) == 1) {
                events[*num_events].size = size;
                events[*num_events].is_alloc = 0;
                events[*num_events].ptr = NULL;
                (*num_events)++;
            }
        }
    }
    
    fclose(file);
    return 0;
}

/* Function to replay allocation sequence */
int replay_allocation_sequence(AllocationEvent* events, int num_events, 
    WOLFSSL_HEAP_HINT* heap_hint )
{
    int i;
    int success_count = 0;
    int failure_count = 0;
    
    for (i = 0; i < num_events; i++) {
        if (events[i].is_alloc) {
            /* Try to allocate memory */
            void* ptr = XMALLOC(events[i].size, heap_hint, DYNAMIC_TYPE_TMP_BUFFER);
            if (ptr == NULL) {
                printf("FAILURE: malloc failed for size %d at event %d\n", 
                       events[i].size, i);
                failure_count++;
                return -1; /* Exit on first failure */
            } else {
                events[i].ptr = ptr;
                success_count++;
                printf("SUCCESS: Allocated %d bytes at event %d\n", events[i].size, i);
            }
        } else {
            /* Find the corresponding allocation to free */
            int found = 0;
            for (int j = i - 1; j >= 0; j--) {
                if (events[j].is_alloc && events[j].size == events[i].size && 
                    events[j].ptr != NULL) {
                    XFREE(events[j].ptr, heap_hint, DYNAMIC_TYPE_TMP_BUFFER);
                    events[j].ptr = NULL;
                    events[i].ptr = NULL;
                    found = 1;
                    printf("SUCCESS: Freed %d bytes at event %d\n", events[i].size, i);
                    break;
                }
            }
            if (!found) {
                printf("WARNING: No matching allocation found for free of size %d at event %d\n", 
                       events[i].size, i);
            }
        }
    }
    
    printf("\nReplay Summary:\n");
    printf("Total events: %d\n", num_events);
    printf("Successful allocations: %d\n", success_count);
    printf("Failed allocations: %d\n", failure_count);
    
    if (failure_count > 0) {
        printf("TEST FAILED: Some allocations failed\n");
        return -1;
    } else {
        printf("TEST PASSED: All allocations succeeded\n");
        return 0;
    }
}

/* Function to calculate required buffer size */
int calculate_required_buffer_size(BucketConfig* buckets, int num_buckets) {
    int total_size = 0;
    int i;
    
    for (i = 0; i < num_buckets; i++) {
        total_size += buckets[i].size * buckets[i].count;
    }
    
    /* Add overhead for wolfSSL heap structures */
    total_size += 64 + 32 + (WOLFSSL_STATIC_ALIGN - 1);
    
    return total_size;
}

void print_usage(const char* program_name) {
    printf("Usage: %s <log_file> --buckets \"<size1>,<size2>,...\" --dist \"<dist1>,<dist2>,...\" --buffer-size <total_size>\n", program_name);
    printf("\n");
    printf("Arguments:\n");
    printf("  <log_file>     Path to the memory allocation log file\n");
    printf("  --buckets      Bucket sizes (comma-separated in quotes)\n");
    printf("  --dist         Distribution counts for each bucket (comma-separated in quotes)\n");
    printf("  --buffer-size  Total buffer size to use (in bytes)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s test.log --buckets \"1024,256,128\" --dist \"2,4,8\" --buffer-size 8192\n", program_name);
    printf("  %s test.log --buckets \"1584,1024,256,128,32\" --dist \"2,2,4,2,1\" --buffer-size 16384\n", program_name);
    printf("  %s test.log --buckets \"1024,256,128\" --dist \"2,2,4,2,1,3,1,16,1\" --buffer-size 4096\n", program_name);
    printf("\n");
    printf("The tester will:\n");
    printf("  1. Parse the allocation log file\n");
    printf("  2. Replay the exact same allocation sequence\n");
    printf("  3. Use the provided bucket configuration and buffer size\n");
    printf("  4. Fail if any XMALLOC fails\n");
}

int main(int argc, char** argv) {
    if (argc < 6) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* log_file = argv[1];
    BucketConfig buckets[MAX_BUCKETS];
    int num_buckets = 0;
    int total_buffer_size = 0;
    
    /* Parse bucket configuration */
    if (parse_bucket_config(argc, argv, 2, buckets, &num_buckets, &total_buffer_size) != 0) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (num_buckets == 0) {
        printf("Error: No buckets configured\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (total_buffer_size <= 0) {
        printf("Error: Invalid buffer size (%d). Must be greater than 0.\n",
            total_buffer_size);
        print_usage(argv[0]);
        return 1;
    }
    
    /* Parse allocation events */
    AllocationEvent events[MAX_ALLOCATIONS];
    int num_events = 0;
    
    if (parse_memory_logs(log_file, events, &num_events) != 0) {
        return 1;
    }
    
    printf("Parsed %d allocation events from %s\n", num_events, log_file);
    
    /* Use provided buffer size */
    int buffer_size = total_buffer_size;
    printf("Using provided buffer size: %d bytes\n\n", buffer_size);
    
    /* Convert bucket config to wolfSSL format */
    unsigned int bucket_sizes[MAX_BUCKETS];
    unsigned int bucket_dist[MAX_BUCKETS];
    int i;
    
    for (i = 0; i < num_buckets; i++) {
        bucket_sizes[i] = (unsigned int)buckets[i].size;
        bucket_dist[i] = (unsigned int)buckets[i].count;
    }
    
    /* Allocate static buffer */
    byte* static_buffer = (byte*)malloc(buffer_size);
    if (!static_buffer) {
        printf("Error: Failed to allocate static buffer\n");
        return 1;
    }
    
    /* Initialize static memory */
    WOLFSSL_HEAP_HINT* heap_hint = NULL;
    int ret = wc_LoadStaticMemory_ex(&heap_hint, num_buckets, bucket_sizes, 
                                   bucket_dist, static_buffer, buffer_size, 0, 0);
    if (ret != 0) {
        printf("Error: Failed to load static memory (ret=%d)\n", ret);
        free(static_buffer);
        return 1;
    }
    
    /* Display heap hint information if available */
    if (heap_hint != NULL) {
        printf("Heap hint initialized successfully\n");
        printf("Heap hint address: %p\n", (void*)heap_hint);
        
        /* Try to access the heap structure if available */
        if (heap_hint->memory != NULL) {
            WOLFSSL_MEM_STATS mem_stats;
            /* Extract bucket information from heap structure */
            WOLFSSL_HEAP* heap = (WOLFSSL_HEAP*)heap_hint->memory;
            
            wolfSSL_GetMemStats(heap, &mem_stats);

            /* Print actual buckets created in the heap */
            printf("Actual buckets created in heap:\n");
            for (i = 0; i < WOLFMEM_MAX_BUCKETS; i++) {
                printf("  Bucket %d: size=%d, count=%d\n", i,
                    mem_stats.blockSz[i], mem_stats.avaBlock[i]);
            }
        } else {
            printf("Heap structure not accessible\n");
        }
        
        printf("Static memory system ready for allocation testing\n");
    } else {
        printf("Warning: Heap hint is NULL\n");
    }
    printf("=====================================\n\n");
    fflush(stdout);
    
    /* Replay allocation sequence */
    ret = replay_allocation_sequence(events, num_events, heap_hint);
    
    /* Cleanup */
    free(static_buffer);
    
    return ret;
} 