#ifndef UTILITY_WOLF_H
#define UTILITY_WOLF_H

#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <string.h>
#include <uchar.h>



/* Custom Types */
typedef enum {
    READ,
    WRITE,
    READWRITE,
    OPENDIR,
    NONE,
    NULL_ARG,
} FILE_OPS;

/* Done */

/* UEFI RNG */
int uefi_random_gen(char* output, unsigned int sz);


/* Logging Functions */

/* These use AsciiPrint and AsciiVSprint and simply wrap to produce a result */
/* as an int instead of a unsigned int */
int uefi_printf_wolfssl(const char* msg, ...);
int uefi_snprintf_wolfssl(char* buffer, size_t n, const char* format, ...);
int uefi_vsnprintf_wolfssl(char* buffer, size_t size, const char* format,
                                va_list args);
int uefi_vprintf_wolfssl(const char* msg, va_list args);
int uefi_fprintf_uefi(FILE* stream, const char* format, ...);

int uefi_wolfssl_fflush(FILE* stream);

/* Memory Allocation Functions */
void* uefi_malloc_wolfssl(size_t n);
void* uefi_realloc_wolfssl(void* ptr, size_t n);
void uefi_free_wolfssl(void* ptr);
void* XMALLOC(size_t n, void* heap, int type);
void* XREALLOC(void *p, size_t n, void* heap, int type);
void XFREE(void *p, void* heap, int type);

void* uefi_memcpy_wolfssl(void* dest, const void* src, size_t len);
void* uefi_memset_wolfssl(void* str, int c, size_t n);
int uefi_strncmp_wolfssl(const char* s1, const char* s2, size_t n);

/* Utility Functions */
unsigned long uefi_time_wolfssl(unsigned long* timer);
unsigned long convertToEpochUefi(EFI_TIME ts);
int uefi_timeStruct_wolfssl(EFI_TIME* timeStruct);
int parseAndReplace(const char* msg, char* temp, const char* search, const char* replace);
void char8_to_char16(const char* str8, char16_t* str16);
void char8_to_char16_ex(const char* str8, char16_t* str16, int n);
unsigned int calculateBufferSize(const char* msg, va_list args);

/* Need to review TODO */

/* Logging Functions */
void logging_cb(const int logLevel, const char *const logMessage);



/* Not Implemented Functions */
int create(char *filename, mode_t mode);
int open (const char *__file, int __oflag);
int close(int __fd);

/* Needed for USER IO functions */

/* Functional */
FILE_OPS getFileOperation(const char* mode);
EFI_FILE_HANDLE getVolume(void);
uint64_t fileSize(EFI_FILE_HANDLE FileHandle);



/* TODO */
ssize_t read(int fd, void *buf, size_t cnt);
ssize_t write(int fd, const void* buf, size_t cnt);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
DIR* opendir(const char* name);
struct dirent* readdir(DIR* dirp);
int closedir(DIR* dirp);
int stat(const char* path, struct stat* buf);
void uefi_strerr(const char* message);
static int write_integer(FILE* stream, int value);
static int write_string(FILE* stream, const char* str);

#endif