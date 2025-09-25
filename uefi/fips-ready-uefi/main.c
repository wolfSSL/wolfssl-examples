
#include <efi.h>
#include <efilib.h>
#include <user_settings.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/test/test.h>
#include <utility_wolf.h>


#ifndef WOLFSSL_USER_SETTINGS
    #error "USER SETTINGS not set"
#endif

#ifndef WAIT_FOR_GDB
#define WAIT_FOR_GDB 0
#endif

#define uefi_printf uefi_printf_wolfssl
#define uefi_snprintf(_buf_, _size_, _fmt_, ...) SPrint((_buf_), (_size_), L##_fmt_, ##__VA_ARGS__)
FILE* stdout = NULL;
FILE* stderr = NULL;


EFI_LOADED_IMAGE* loaded_image;
//EFI_SYSTEM_TABLE* stdout_uefi;

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    loaded_image = NULL;
    volatile int debug = 1;
    EFI_STATUS status;
    int ret;

    InitializeLib(ImageHandle, SystemTable);
    stdout = (FILE*)(SystemTable->ConOut);
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

    fipsEntry();
    //ret = wolfcrypt_test(NULL);
    ret = benchmark_test(NULL);
    Print(L"ret: %d\n", ret);

    return EFI_SUCCESS;
}
