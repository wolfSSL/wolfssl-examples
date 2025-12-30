/* wolfssl_tracer.bpf.c
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

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

// Define the tracepoint structure based on kernel BTF
struct trace_entry {
    unsigned short type;
    unsigned char flags;
    unsigned char preempt_count;
    int pid;
};

struct trace_event_raw_sys_enter {
    struct trace_entry ent;
    long int id;
    long unsigned int args[6];
};

struct write_event {
    __u32 pid;
    __u32 tid;
    int fd;
    __u64 count;
    char comm[16];
    char data[256];
};

struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
} events SEC(".maps");

// Use syscall tracepoint with BPF_PROG macro for proper argument extraction
SEC("tp/syscalls/sys_enter_write")
int trace_write_enter(struct trace_event_raw_sys_enter *ctx)
{
    struct write_event event = {};
    __u64 pid_tgid;
    __u32 pid, tid;
    char comm[16];
    long ret;
    unsigned int fd;
    char *buf;
    unsigned long count;

    pid_tgid = bpf_get_current_pid_tgid();
    pid = pid_tgid >> 32;
    tid = (__u32)pid_tgid;

    bpf_get_current_comm(&comm, sizeof(comm));

    // Filter: only trace "client-tcp" process
    // Unrolled string comparison for verifier
    if (comm[0] != 'c' || comm[1] != 'l' || comm[2] != 'i' ||
        comm[3] != 'e' || comm[4] != 'n' || comm[5] != 't' ||
        comm[6] != '-' || comm[7] != 't' || comm[8] != 'c' || comm[9] != 'p') {
        return 0;
    }

    // The syscall arguments are stored in ctx->args[]
    // args[0] = fd, args[1] = buf, args[2] = count
    fd = (unsigned int)ctx->args[0];
    buf = (char *)ctx->args[1];
    count = (unsigned long)ctx->args[2];

    event.pid = pid;
    event.tid = tid;
    event.fd = fd;
    event.count = count;

    bpf_get_current_comm(&event.comm, sizeof(event.comm));

    // Read user buffer data with bounds checking
    if (count > 0 && buf != 0) {
        unsigned long len = count;
        if (len > 255) {
            len = 255;
        }

        // Use bpf_probe_read_user to safely read from user space
        ret = bpf_probe_read_user(event.data, len, buf);
        if (ret < 0) {
            event.data[0] = '\0';
        } else if (len < 256) {
            event.data[len] = '\0';
        }
    } else {
        event.data[0] = '\0';
    }

    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &event, sizeof(event));
    return 0;
}
