/* write_tracer.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct write_event {
    __u32 pid;
    __u32 tid;
    int fd;
    __u64 count;
    char comm[16];
    char data[256];
};

static volatile bool running = true;

static void sig_handler(int sig)
{
    running = false;
}

static void handle_event(void *ctx, int cpu, void *data, __u32 data_sz)
{
    struct write_event *e = data;
    
    printf("=== WRITE SYSCALL INTERCEPTED ===\n");
    printf("Process: %s (PID: %u, TID: %u)\n", e->comm, e->pid, e->tid);
    printf("File Descriptor: %d\n", e->fd);
    printf("Write Count: %llu bytes\n", e->count);
    printf("Data (first %zu bytes): ", strlen(e->data));
    
    for (int i = 0; i < strlen(e->data) && i < 64; i++) {
        char c = e->data[i];
        if (c >= 32 && c <= 126) {
            printf("%c", c);
        } else {
            printf("\\x%02x", (unsigned char)c);
        }
    }
    printf("\n");
    
    printf("Data (hex): ");
    for (int i = 0; i < strlen(e->data) && i < 32; i++) {
        printf("%02x ", (unsigned char)e->data[i]);
    }
    printf("\n");
    printf("=====================================\n\n");
}

static void handle_lost_events(void *ctx, int cpu, __u64 lost_cnt)
{
    printf("Lost %llu events on CPU #%d!\n", lost_cnt, cpu);
}

static int bump_memlock_rlimit(void)
{
    struct rlimit rlim_new = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };

    if (setrlimit(RLIMIT_MEMLOCK, &rlim_new)) {
        fprintf(stderr, "Failed to increase RLIMIT_MEMLOCK limit: %s\n", strerror(errno));
        fprintf(stderr, "You may need to run this program with sudo or adjust system limits.\n");
        return -1;
    }
    return 0;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    /* Only print warnings and errors */
    if (level <= LIBBPF_WARN)
        return vfprintf(stderr, format, args);
    return 0;
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    struct perf_buffer *pb;
    int map_fd;
    int err;

    /* Set up libbpf logging callback */
    libbpf_set_print(libbpf_print_fn);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("Loading eBPF program to trace write() syscalls...\n");

    /* Check if we're running as root */
    if (geteuid() != 0) {
        fprintf(stderr, "Error: This program must be run as root (use sudo).\n");
        fprintf(stderr, "Example: sudo %s\n", argv[0]);
        return 1;
    }

    /* Bump RLIMIT_MEMLOCK to allow BPF map creation */
    if (bump_memlock_rlimit()) {
        fprintf(stderr, "Failed to increase rlimit\n");
        return 1;
    }

    printf("Press Ctrl+C to stop.\n\n");

    obj = bpf_object__open_file("write_tracer.bpf.o", NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "Failed to open BPF object file\n");
        return 1;
    }
    
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        goto cleanup;
    }
    
    prog = bpf_object__find_program_by_name(obj, "trace_write_enter");
    if (!prog) {
        fprintf(stderr, "Failed to find BPF program\n");
        err = 1;
        goto cleanup;
    }
    
    link = bpf_program__attach(prog);
    if (libbpf_get_error(link)) {
        fprintf(stderr, "Failed to attach BPF program\n");
        err = 1;
        goto cleanup;
    }
    
    map_fd = bpf_object__find_map_fd_by_name(obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "Failed to find events map\n");
        err = 1;
        goto cleanup_link;
    }
    
    struct perf_buffer_opts pb_opts = {
        .sample_cb = handle_event,
        .lost_cb = handle_lost_events,
    };
    
    pb = perf_buffer__new(map_fd, 8, &pb_opts);
    if (libbpf_get_error(pb)) {
        fprintf(stderr, "Failed to create perf buffer\n");
        err = 1;
        goto cleanup_link;
    }
    
    printf("Successfully loaded and attached eBPF program!\n");
    printf("Monitoring write() syscalls...\n\n");
    
    while (running) {
        err = perf_buffer__poll(pb, 100);
        if (err < 0 && err != -EINTR) {
            fprintf(stderr, "Error polling perf buffer: %d\n", err);
            break;
        }
    }
    
    printf("\nShutting down...\n");
    
    perf_buffer__free(pb);
cleanup_link:
    bpf_link__destroy(link);
cleanup:
    bpf_object__close(obj);
    return err;
}
