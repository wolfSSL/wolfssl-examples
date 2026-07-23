#!/bin/sh
# Compile the wolfSSL Mynewt header path against $1 (a wolfSSL source tree).
# A full newt build needs the Apache Mynewt RTOS; the port bug is entirely in
# wolfSSL's settings.h/types.h, so we compile just that path with the RTOS
# headers it pulls in stubbed. The bug: the Mynewt block sets XMALLOC_USER yet
# defines XFREE as a macro, so types.h's "extern void XFREE(...)" has the macro
# expand into it. Fix: XMALLOC_USER -> XMALLOC_OVERRIDE.
set -eu

WOLFSSL="$1"
d=$(mktemp -d)
trap 'rm -rf "$d"' EXIT

mkdir -p "$d/os" "$d/mn_socket" "$d/fs"
cat > "$d/os/os_malloc.h" <<'EOF'
#ifndef OS_MALLOC_H
#define OS_MALLOC_H
#include <stdlib.h>
static inline void* os_malloc(size_t n){ return malloc(n); }
static inline void* os_realloc(void* p, size_t n){ return realloc(p, n); }
static inline void  os_free(void* p){ free(p); }
#endif
EOF
printf '#ifndef OS_TIME_H\n#define OS_TIME_H\n#endif\n'   > "$d/os/os_time.h"
printf '#ifndef MN_SOCKET_H\n#define MN_SOCKET_H\n#endif\n' > "$d/mn_socket/mn_socket.h"
printf '#ifndef FS_H\n#define FS_H\n#endif\n'             > "$d/fs/fs.h"
: > "$d/user_settings.h"
cat > "$d/probe.c" <<'EOF'
#define WOLFSSL_APACHE_MYNEWT
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>
int main(void){ char* p = (char*)XMALLOC(8, NULL, 0); XFREE(p, NULL, 0); return 0; }
EOF

# Fixed on wolfSSL master 2026-07-20; refs predating that (e.g. v5.9.2-stable)
# still carry the bug, so patch them. The sed is idempotent -- a no-op once a
# ref has the fix -- so this whole script drops out when the pinned stable bumps.
if gcc -I"$d" -I"$WOLFSSL" -c "$d/probe.c" -o /dev/null 2>/dev/null; then
    echo "this ref already has the upstream XMALLOC_OVERRIDE fix"
else
    echo "this ref predates the upstream fix; applying XMALLOC_OVERRIDE"
    perl -0pi -e 's/(#define NO_ERROR_STRINGS\n\s*)#define XMALLOC_USER(\s*\n\s*#define XMALLOC\(sz, heap, type\)\s+\(\(void\)\(heap\), \(void\)\(type\), os_malloc)/${1}#define XMALLOC_OVERRIDE${2}/' \
        "$WOLFSSL/wolfssl/wolfcrypt/settings.h"
    grep -q XMALLOC_OVERRIDE "$WOLFSSL/wolfssl/wolfcrypt/settings.h"
fi

gcc -I"$d" -I"$WOLFSSL" -c "$d/probe.c" -o /dev/null
echo "PASS: wolfSSL Mynewt port header path compiles"
