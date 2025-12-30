/* wolfssl_uprobe.bpf.c
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

struct wolfssl_event {
    __u32 type;  // 0 = write, 1 = read
    __u32 pid;
    __u32 tid;
    __u64 ssl_ptr;
    __u64 count;  // bytes written or read
    char comm[16];
    char data[256];
};

#ifdef __TARGET_ARCH_x86
struct pt_regs {
    __u64 r15, r14, r13, r12, rbp, rbx, r11, r10;
    __u64 r9, r8, rax, rcx, rdx, rsi, rdi, orig_rax;
    __u64 rip, cs, eflags, rsp, ss;
};
#endif

#ifdef __TARGET_ARCH_arm64
struct pt_regs {
    __u64 regs[31];
    __u64 sp;
    __u64 pc;
    __u64 pstate;
};
#endif

// Store wolfSSL_read arguments while function executes
struct read_args {
    void *buf;
    int sz;
};

struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
} events SEC(".maps");

// Map to store read function arguments indexed by thread ID
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u64);  // pid_tgid
    __type(value, struct read_args);
} read_args_map SEC(".maps");

// wolfSSL_write(WOLFSSL* ssl, const void* data, int sz)
// For ARM64 userspace functions:
//   x0 = first arg (ssl)
//   x1 = second arg (data)
//   x2 = third arg (sz)
SEC("uprobe/wolfSSL_write")
int trace_wolfssl_write_enter(struct pt_regs *ctx)
{
    struct wolfssl_event event = {};
    __u64 pid_tgid;
    __u32 pid, tid;
    char comm[16];
    long ret;
    void *ssl;
    void *data;
    int sz;

    pid_tgid = bpf_get_current_pid_tgid();
    pid = pid_tgid >> 32;
    tid = (__u32)pid_tgid;

    bpf_get_current_comm(&comm, sizeof(comm));

    // Filter: only trace "client-tls" process
    // Manual unrolled comparison to avoid loop issues with verifier
    if (comm[0] != 'c' || comm[1] != 'l' || comm[2] != 'i' ||
        comm[3] != 'e' || comm[4] != 'n' || comm[5] != 't' ||
        comm[6] != '-' || comm[7] != 't' || comm[8] != 'l' || comm[9] != 's') {
        return 0;
    }

    // Read function arguments from registers
    ssl  = (void *)PT_REGS_PARM1(ctx);
    data = (void *)PT_REGS_PARM2(ctx);
    sz   = (int)PT_REGS_PARM3(ctx);    

    event.type = 0;  // write event
    event.pid = pid;
    event.tid = tid;
    event.ssl_ptr = (__u64)ssl;
    event.count = (__u64)sz;

    bpf_get_current_comm(&event.comm, sizeof(event.comm));

    // Read user buffer data with bounds checking
    if (sz > 0 && data != 0) {
        unsigned long len = sz;
        if (len > 255) {
            len = 255;
        }

        // Use bpf_probe_read_user to safely read from user space
        ret = bpf_probe_read_user(event.data, len, data);
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

// wolfSSL_read(WOLFSSL* ssl, void* data, int sz)
// Entry probe - save the arguments for later use in return probe
SEC("uprobe/wolfSSL_read")
int trace_wolfssl_read_enter(struct pt_regs *ctx)
{
    __u64 pid_tgid;
    char comm[16];
    struct read_args args = {};

    pid_tgid = bpf_get_current_pid_tgid();
    bpf_get_current_comm(&comm, sizeof(comm));

    // Filter: only trace "client-tls" process
    if (comm[0] != 'c' || comm[1] != 'l' || comm[2] != 'i' ||
        comm[3] != 'e' || comm[4] != 'n' || comm[5] != 't' ||
        comm[6] != '-' || comm[7] != 't' || comm[8] != 'l' || comm[9] != 's') {
        return 0;
    }

    // Save buffer pointer and size for the return probe
    // x0 = ssl (not needed), x1 = buf, x2 = sz
    args.buf = (void *)PT_REGS_PARM2(ctx);
    args.sz  = (int)PT_REGS_PARM3(ctx);
    
    bpf_map_update_elem(&read_args_map, &pid_tgid, &args, BPF_ANY);
    return 0;
}

// wolfSSL_read return probe - read the decrypted data from the buffer
SEC("uretprobe/wolfSSL_read")
int trace_wolfssl_read_exit(struct pt_regs *ctx)
{
    struct wolfssl_event event = {};
    __u64 pid_tgid;
    __u32 pid, tid;
    char comm[16];
    struct read_args *args;
    int bytes_read;
    long ret;

    pid_tgid = bpf_get_current_pid_tgid();
    pid = pid_tgid >> 32;
    tid = (__u32)pid_tgid;

    bpf_get_current_comm(&comm, sizeof(comm));

    // Filter: only trace "client-tls" process
    if (comm[0] != 'c' || comm[1] != 'l' || comm[2] != 'i' ||
        comm[3] != 'e' || comm[4] != 'n' || comm[5] != 't' ||
        comm[6] != '-' || comm[7] != 't' || comm[8] != 'l' || comm[9] != 's') {
        return 0;
    }

    // Retrieve the saved arguments
    args = bpf_map_lookup_elem(&read_args_map, &pid_tgid);
    if (!args) {
        return 0; 
    }

    bytes_read = (int)PT_REGS_RC(ctx);

    // Only process successful reads
    if (bytes_read <= 0) {
        bpf_map_delete_elem(&read_args_map, &pid_tgid);
        return 0;
    }

    event.type = 1;  // read event
    event.pid = pid;
    event.tid = tid;
    event.ssl_ptr = 0;  // We didn't save SSL pointer, but could if needed
    event.count = (__u64)bytes_read;

    bpf_get_current_comm(&event.comm, sizeof(event.comm));

    // Read the decrypted data from the buffer
    if (bytes_read > 0 && args->buf != 0) {
        unsigned long len = bytes_read;
        if (len > 255) {
            len = 255;
        }

        // Use bpf_probe_read_user to safely read from user space
        ret = bpf_probe_read_user(event.data, len, args->buf);
        if (ret < 0) {
            event.data[0] = '\0';
        } else if (len < 256) {
            event.data[len] = '\0';
        }
    } else {
        event.data[0] = '\0';
    }

    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &event, sizeof(event));

    // Clean up the map entry
    bpf_map_delete_elem(&read_args_map, &pid_tgid);
    return 0;
}
