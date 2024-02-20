
#include <efi.h>
#include <efilib.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/test/test.h>

#define STR_SIZE 512

#ifndef WAIT_FOR_GDB
#define WAIT_FOR_GDB 0
#endif

#define uefi_printf(_f_, ...) Print(L##_f_, ##__VA_ARGS__)

void char8_to_char16(const char* str8, wchar_t* str16)
{
	size_t i;
	size_t size_str8 = strlen(str8);
	for (i = 0; i < size_str8; ++i) {
		str16[i] = (wchar_t)str8[i];
	}
	str16[i] = '\0';
}

void logging_cb(const int logLevel, const char *const logMessage)
{
	wchar_t str16[STR_SIZE];
	char8_to_char16(logMessage, str16);
    uefi_printf("%s", str16);
}

void *XMALLOC(size_t n, void* heap, int type)
{
	return AllocateZeroPool(n);
}

void *XREALLOC(void *p, size_t n, void* heap, int type)
{
    FreePool(p);
    p = NULL;
    return AllocateZeroPool(n);
}

void XFREE(void *p, void* heap, int type)
{
    return FreePool(p);
}

/* TODO: remove dependencies in random.c to open/read/close */
int open (const char *__file, int __oflag)
{
    uefi_printf("open\n");
    return -1;
}

ssize_t read (int __fd, void *__buf, size_t __nbytes)
{
    uefi_printf("read\n");
    return -1;
}

int close(int __fd)
{
    uefi_printf("close\n");
    return -1;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_LOADED_IMAGE *loaded_image = NULL;
    volatile int debug = 1;
    EFI_STATUS status;
    int ret;

	InitializeLib(ImageHandle, SystemTable);
    wolfSSL_Debugging_ON();
    wolfSSL_SetLoggingCb(logging_cb);

    status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol,
                               3,
                              ImageHandle,
                              &LoadedImageProtocol,
                               (void **)&loaded_image);
    Print(L"status: 0x%lx\n", status);
    Print(L"Image base: 0x%lx\n", loaded_image->ImageBase);

#if WAIT_FOR_GDB
    /* to debug from gdb:
     *
     * 1. run qemu with -s option. Take note of Image base value printed by the
     * app.
     * 2. run gdb, use command: symbol-file wolfcrypt.elf -o $image_base
     * with image based value from the print above.
     * 3. set variable debug = 0 to exit the loop and continue the debugging */
    while(debug) {};
#else
    (void)debug;
#endif

    ret = wolfcrypt_test(NULL);
    Print(L"ret: %d\n", ret);

	return EFI_SUCCESS;
}
