#ifndef UTILITY_WOLF_H
#define UTILITY_WOLF_H

#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <string.h>


/* Custom Types */
typedef enum {
    READ,
    WRITE,
    READWRITE,
    OPENDIR,
    NONE,
    NULL_ARG,
} FILE_OPS;

/* Custom Functions */


/* Done */

/* UEFI RNG */
int uefi_random_gen(char* output, unsigned int sz);


/* Logging Functions */


/* Memory Allocation Functions */

void *uefi_malloc_wolfssl(size_t n);
void *uefi_realloc_wolfssl(void* ptr, size_t n);
void uefi_free_wolfssl(void* ptr);

void *XMALLOC(size_t n, void* heap, int type);
void *XREALLOC(void *p, size_t n, void* heap, int type);
void XFREE(void *p, void* heap, int type);


/* Random Utility Functions */
void parseAndReplace(const char* msg, char* temp, const char* search, const char* replace);
void removeLeadingDot(char* str);
void addPrefix(char** str, const char* prefix, int isStack);
void char8_to_char16(const char* str8, wchar_t* str16);

/* Need to review TODO */

/* Logging Functions */
void logging_cb(const int logLevel, const char *const logMessage);
int uefi_printf_wolfssl(const char* msg, ...);



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
int uefi_fprintf(FILE* stream, const char* format, ...);
void uefi_strerr(const char* message);
int uefi_vsnprintf(char* buffer, size_t size, const char* format, va_list args);
int uefi_snprintf_wolfssl(char* buffer, size_t n,
                          const char* format, ...);

#endif