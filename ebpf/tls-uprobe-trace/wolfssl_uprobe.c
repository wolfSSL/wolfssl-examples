/* wolfssl_uprobe.c
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
#include <stdbool.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct wolfssl_event {
    __u32 type;  // 0 = write, 1 = read
    __u32 pid;
    __u32 tid;
    __u64 ssl_ptr;
    __u64 count;  // bytes written or read
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
    struct wolfssl_event *e = data;

    if (e->type == 0) {
        // Write event
        printf("=== wolfSSL_write() INTERCEPTED ===\n");
        printf("Process: %s (PID: %u, TID: %u)\n", e->comm, e->pid, e->tid);
        printf("SSL Pointer: 0x%llx\n", e->ssl_ptr);
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
    } else if (e->type == 1) {
        // Read event
        printf("=== wolfSSL_read() INTERCEPTED (RETURN) ===\n");
        printf("Process: %s (PID: %u, TID: %u)\n", e->comm, e->pid, e->tid);
        printf("Bytes Read: %llu bytes\n", e->count);
        printf("Decrypted Data (first %zu bytes): ", strlen(e->data));

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
        printf("============================================\n\n");
    }
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

static long find_symbol_offset(const char *binary_path, const char *symbol_name)
{
    Elf *elf = NULL;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    int fd = -1;
    long offset = -1;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "Failed to init libelf\n");
        return -1;
    }

    fd = open(binary_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", binary_path, strerror(errno));
        return -1;
    }

    elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        fprintf(stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
        goto cleanup;
    }

    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr)
            continue;
        if (shdr.sh_type != SHT_SYMTAB && shdr.sh_type != SHT_DYNSYM)
            continue;

        Elf_Data *data = elf_getdata(scn, NULL);
        if (!data)
            continue;

        int symbols = shdr.sh_size / shdr.sh_entsize;
        for (int i = 0; i < symbols; i++) {
            GElf_Sym sym;
            if (!gelf_getsym(data, i, &sym))
                continue;
            const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
            if (!name)
                continue;
            if (strcmp(name, symbol_name) == 0) {
                offset = sym.st_value;
                goto cleanup;
            }
        }
    }

cleanup:
    if (elf)
        elf_end(elf);
    if (fd >= 0)
        close(fd);

    if (offset < 0)
        fprintf(stderr, "Failed to locate symbol %s in %s\n", symbol_name, binary_path);

    return offset;
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_program *prog_write, *prog_read_enter, *prog_read_exit;
    struct bpf_link *link_write, *link_read_enter, *link_read_exit;
    struct perf_buffer *pb;
    int map_fd;
    int err;
    char *library_path = "/usr/local/lib/libwolfssl.so.44";
    pid_t target_pid = -1;  // -1 means attach to all processes

    if (argc > 1) {
        library_path = argv[1];
    }

    /* Set up libbpf logging callback */
    libbpf_set_print(libbpf_print_fn);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("Loading eBPF program to trace wolfSSL_write()...\n");

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

    printf("Target library: %s\n", library_path);
    printf("Attaching to function: wolfSSL_write\n");
    printf("Press Ctrl+C to stop.\n\n");

    obj = bpf_object__open_file("wolfssl_uprobe.bpf.o", NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "Failed to open BPF object file\n");
        return 1;
    }

    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        goto cleanup;
    }

    // Find all BPF programs
    prog_write = bpf_object__find_program_by_name(obj, "trace_wolfssl_write_enter");
    if (!prog_write) {
        fprintf(stderr, "Failed to find wolfSSL_write BPF program\n");
        err = 1;
        goto cleanup;
    }

    prog_read_enter = bpf_object__find_program_by_name(obj, "trace_wolfssl_read_enter");
    if (!prog_read_enter) {
        fprintf(stderr, "Failed to find wolfSSL_read entry BPF program\n");
        err = 1;
        goto cleanup;
    }

    prog_read_exit = bpf_object__find_program_by_name(obj, "trace_wolfssl_read_exit");
    if (!prog_read_exit) {
        fprintf(stderr, "Failed to find wolfSSL_read exit BPF program\n");
        err = 1;
        goto cleanup;
    }

    long write_offset = find_symbol_offset(library_path, "wolfSSL_write");
    if (write_offset < 0) {
        err = 1;
        goto cleanup;
    }
    long read_offset = find_symbol_offset(library_path, "wolfSSL_read");
    if (read_offset < 0) {
        err = 1;
        goto cleanup;
    }

    // Attach uprobe/uretprobe to wolfSSL functions
    link_write = bpf_program__attach_uprobe(prog_write, false, target_pid, library_path,
                                            write_offset);
    if (libbpf_get_error(link_write)) {
        fprintf(stderr, "Failed to attach uprobe to wolfSSL_write\n");
        fprintf(stderr, "Make sure:\n");
        fprintf(stderr, "  1. The library path is correct: %s\n", library_path);
        fprintf(stderr, "  2. wolfSSL_write symbol exists in the library\n");
        fprintf(stderr, "  3. You have sufficient permissions (run with sudo)\n");
        err = 1;
        goto cleanup;
    }

    // Attach uprobe to wolfSSL_read entry
    link_read_enter = bpf_program__attach_uprobe(prog_read_enter, false, target_pid, library_path,
                                                 read_offset);
    if (libbpf_get_error(link_read_enter)) {
        fprintf(stderr, "Failed to attach uprobe to wolfSSL_read (entry)\n");
        err = 1;
        goto cleanup_write;
    }

    // Attach uretprobe to wolfSSL_read exit
    link_read_exit = bpf_program__attach_uprobe(prog_read_exit, true, target_pid, library_path,
                                                read_offset);
    if (libbpf_get_error(link_read_exit)) {
        fprintf(stderr, "Failed to attach uretprobe to wolfSSL_read (exit)\n");
        err = 1;
        goto cleanup_read_enter;
    }

    map_fd = bpf_object__find_map_fd_by_name(obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "Failed to find events map\n");
        err = 1;
        goto cleanup_read_exit;
    }

    struct perf_buffer_opts pb_opts = {
        .sample_cb = handle_event,
        .lost_cb = handle_lost_events,
    };

    pb = perf_buffer__new(map_fd, 8, &pb_opts);
    if (libbpf_get_error(pb)) {
        fprintf(stderr, "Failed to create perf buffer\n");
        err = 1;
        goto cleanup_read_exit;
    }

    printf("Successfully loaded and attached eBPF programs!\n");
    printf("Monitoring wolfSSL_write() and wolfSSL_read() calls...\n\n");

    while (running) {
        err = perf_buffer__poll(pb, 100);
        if (err < 0 && err != -EINTR) {
            fprintf(stderr, "Error polling perf buffer: %d\n", err);
            break;
        }
    }

    printf("\nShutting down...\n");

    perf_buffer__free(pb);
cleanup_read_exit:
    bpf_link__destroy(link_read_exit);
cleanup_read_enter:
    bpf_link__destroy(link_read_enter);
cleanup_write:
    bpf_link__destroy(link_write);
cleanup:
    bpf_object__close(obj);
    return err;
}
