#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <string.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/test/test.h>
#include <utility_wolf.h>

#define STR_SIZE 512
#define MSG_BUFFER_TMP 1024

#define uefi_printf(_f_, ...) Print(L##_f_, ##__VA_ARGS__) /* Native Print */
#define uefi_snprintf(_buf_, _size_, _fmt_, ...) SPrint((_buf_), (_size_), L##_fmt_, ##__VA_ARGS__)

/* Used to debug verbosely to see where code fails, or gets hung */
#ifdef WOLFSSL_UEFI_VERBOSE_DEBUG
    #define uefi_printf_debug uefi_printf
#else
    #define uefi_printf_debug(...) ((void)0)
#endif

/* Needed from main */
extern EFI_LOADED_IMAGE *loaded_image;
FILE* stderr = NULL;


/* Function to parse and replace a search string with a replacement string */
/* used to find options such %s and replace with %a, but is setup to parse */
/* any needed string */

void parseAndReplace(const char* msg, char* temp, const char* search, const char* replace)
{
    size_t searchLen = strlen(search);
    size_t replaceLen = strlen(replace);
    size_t tempIndex = 0;
    size_t msgIndex = 0;

    while (msg[msgIndex] != '\0' && tempIndex < MSG_BUFFER_TMP - 1) {
        /* Check if the search string matches at the current position */
        if (strncmp(&msg[msgIndex], search, searchLen) == 0) {
            /* Ensure there's enough space in temp for the replacement */
            if (tempIndex + replaceLen >= MSG_BUFFER_TMP - 1) {
                break;
            }

            /* Copy the replacement string */
            memcpy(&temp[tempIndex], replace, replaceLen);
            tempIndex += replaceLen;

            /* Skip the search string length in the input */
            msgIndex += searchLen;
        } else {
            /* Copy the current character */
            temp[tempIndex++] = msg[msgIndex++];
        }
    }

    /* Null-terminate the output */
    temp[tempIndex] = '\0';
}

/* Utility to add a prefix to a string buffer */
/* Will need to know if the str buffer passed is on the heap or stack */
void addPrefix(char** str, const char* prefix, int isStack)
{
    size_t prefixLen = strlen(prefix);
    size_t strLen = strlen(*str);

    if (isStack) {
        /* Ensure the stack buffer has enough space */
        if (prefixLen + strLen + 1 > sizeof(*str)) {
            uefi_printf("Error: Not enough space in stack buffer\n");
            return;
        }

        /* Shift the original string to the right to make room for the prefix */
        for (size_t i = strLen; i != (size_t)-1; i--) {
            (*str)[i + prefixLen] = (*str)[i];
        }

        /* Copy the prefix to the start of the string */
        for (size_t i = 0; i < prefixLen; i++) {
            (*str)[i] = prefix[i];
        }
    }
    else {
        /* Allocate new memory for the combined string */
        char* newStr = (char*)XMALLOC((prefixLen + strLen + 1)*sizeof(char),
                                        NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (newStr == NULL) {
            uefi_printf("Error: Memory allocation failed\n");
            return;
        }

        /* Copy the prefix to the new string */
        strcpy(newStr, prefix);

        /* Append the original string */
        strcat(newStr, *str);

        /* Free the original string if it was allocated on the heap */
        XFREE(*str, NULL, DYNAMIC_TYPE_TMP_BUFFER);

        /* Update the pointer to point to the new string */
        *str = newStr;
    }
}

int uefi_snprintf_wolfssl(char* buffer, size_t n, const char* format, ...)
{
    va_list args; /* Declare a va_list to handle variadic arguments */
    int result;

    /* Initialize args to store all values after 'format' */
    va_start(args, format);

    /* Pass the variadic arguments to SPrintf */
    result = SPrint(buffer, n, format, args);

    /* Clean up the va_list */
    va_end(args);

    return result;
}


/* Main Print Function */
/* This will be used to wrap wolfSSL current printf outputs */
/* UEFI is a little funny so we need to replace the standard %s option to %a */
/* UEFI will treat %s as unicode arguments whilst %a will be treated as ascii */
/* Then we need to convert the final output to wide char's for UEFI */
/* This then gets sent to UEFI's VPrint instead of it's Print */
/* VPrint accepts va_list since we need to pass args through */
int uefi_printf_wolfssl(const char* msg, ...)
{
    int ret = 0;
    char* temp; /* Temporary buffer pointer */
    wchar_t* tempWide; /* Wide buffer pointer */

    if (!msg) {
        uefi_printf("Bad args to printf handler\n");
        return -1; /* Handle null input gracefully */
    }

    temp = (char*)XMALLOC(strlen(msg)*sizeof(char),
                            NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (temp == NULL) {
        uefi_printf("UEFI wolfSSL Print Failed: char buffer allocation\n");
        return -1;
    }

    tempWide = (wchar_t*)XMALLOC(strlen(msg)*sizeof(wchar_t),
                                    NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (tempWide == NULL) {
        uefi_printf("UEFI wolfSSL Print Failed: wide char buffer allocation");
        return -1;
    }


    /* Parse for %s and replace with %a */
    parseAndReplace(msg, temp, "%s", "%a");

    /* Convert to wide string */
    char8_to_char16(temp, tempWide);

    /* Handle variable arguments */
    va_list args;
    va_start(args, msg);
    ret = (int)VPrint(tempWide, args);
    va_end(args);

    XFREE(temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(tempWide, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}



/* Helper function to write an integer to the stream */
static int write_integer(FILE* stream, int value) {
    char buffer[12]; /* Enough to store a 32-bit int as a string */
    int len = 0;

    if (value < 0) {
        buffer[len++] = '-';
        value = -value;
    }

    int temp = value;
    int digits = 0;

    /* Count digits */
    do {
        temp /= 10;
        digits++;
    } while (temp > 0);

    /* Write digits to the buffer */
    for (int i = 0; i < digits; i++) {
        buffer[len + digits - 1 - i] = '0' + (value % 10);
        value /= 10;
    }
    len += digits;

    /* Write to the file */
    size_t written = fwrite(buffer, sizeof(char), len, stream);
    return (written == (size_t)len) ? len : -1;
}

/* Helper function to write a string to the stream */
static int write_string(FILE* stream, const char* str) {
    size_t len = strlen(str);
    size_t written = fwrite(str, sizeof(char), len, stream);
    return (written == len) ? len : -1;
}

int uefi_fprintf(FILE* stream, const char* format, ...)
{
    if (stream == NULL || format == NULL) {
        return -1;
    }

    uefi_printf("Inside custom fprintf\n");

    va_list args;
    va_start(args, format);

    int total_written = 0;

    while (*format) {
        if (*format == '%') {
            format++; /* Skip '%' */

            int written = 0;
            switch (*format) {
                case 'd': { /* Integer */
                    int value = va_arg(args, int);
                    written = write_integer(stream, value);
                    break;
                }
                case 's': { /* String */
                    const char* str = va_arg(args, const char*);
                    written = write_string(stream, str);
                    break;
                }
                case '%': { /* Literal '%' */
                    char percent = '%';
                    written = fwrite(&percent, sizeof(char), 1, stream);
                    break;
                }
                default:
                    /* Unsupported format specifier */
                    written = -1;
                    break;
            }

            if (written < 0) {
                va_end(args);
                return -1;
            }

            total_written += written;
        } else {
            /* Write literal characters */
            size_t written = fwrite(format, sizeof(char), 1, stream);
            if (written != 1) {
                va_end(args);
                return -1;
            }
            total_written++;
        }
        format++;
    }

    va_end(args);
    return total_written;
}


void removeLeadingDot(char* str)
{
    /* Check if the first character is '.' */
    if (str[0] == '.') {
        /* Shift all characters to the left, including the null terminator */
        size_t len = strlen(str);
        for (size_t i = 0; i < len; i++) {
            str[i] = str[i + 1];
        }
    }
}

/* Convert standard 8 bit char to 16 bit wide char */
void char8_to_char16(const char* str8, wchar_t* str16)
{
    size_t i;
    size_t size_str8 = strlen(str8);
    for (i = 0; i < size_str8; ++i) {
        str16[i] = (wchar_t)str8[i];
    }
    str16[i] = '\0';
}

uint64_t fileSize(EFI_FILE_HANDLE FileHandle)
{
    uint64_t ret;
    EFI_FILE_INFO       *FileInfo;         /* file information structure */
    /* get the file's size */
    FileInfo = LibFileInfo(FileHandle);
    ret = FileInfo->FileSize;
    FreePool(FileInfo);
    return ret;
}


int fflush(FILE* stream)
{
    size_t ret = 0; /* Number of items successfully read */
    EFI_FILE_HANDLE* fPtr = NULL;
    uefi_printf_debug("Inside custom fflush\n");
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return -1;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;


    /* Attempt to read from the file */
    uefi_printf_debug("Before Wrapper\n");
    uefi_call_wrapper((*fPtr)->Flush, 1, *fPtr);
    uefi_printf_debug("After Wrapper\n");

    uefi_printf_debug("Leaving custom fflush\n");
    return 0;
}

void exit(int n)
{
    Exit(n, 0, "Exiting Called");
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


void *uefi_malloc_wolfssl(size_t n)
{
    return XMALLOC(n, NULL, DYNAMIC_TYPE_TMP_BUFFER);
}

void *uefi_realloc_wolfssl(void* ptr, size_t n)
{
    return XREALLOC(ptr, n, NULL, DYNAMIC_TYPE_TMP_BUFFER);
}

void uefi_free_wolfssl(void* ptr)
{
    return XFREE(ptr, NULL, DYNAMIC_TYPE_TMP_BUFFER);
}

/* USER FILE IO SECTION */

double fabs(double x) {
    return (x < 0) ? -x : x;
}


ssize_t read(int fd, void *buf, size_t cnt)
{
    uefi_printf("Inside Read Open\n");
    return -1;
}


ssize_t write(int fd, const void* buf, size_t cnt)
{
    uefi_printf("Inside Write Close\n");
    return -1;
}

/* Opens a file specified by filename and mode, */
/* returns a FILE pointer or NULL on failure */
/* UEFI requires that the filepath be in wchar format aka unicode */
/* however fopen is specified as taking in a const char buffer */
/* so custom fopen will assume this is the case, but then convert */
/* the buffer to wchar */
#ifndef WOLFSSL_NEED_DYNAMIC_TYPE_FIX_UEFI
FILE* fopen(const wchar_t* filename, const char* mode)
#else
FILE* fopen(const char* filename, const char* mode)
#endif
{
    EFI_FILE_HANDLE* fPtr = NULL;
    FILE_OPS option = NONE;
    EFI_FILE_HANDLE volume;
    EFI_STATUS status;
    uint64_t size;
    /* Temporary buffer pointer */
    char* temp;
    /* Since fopen is */
    wchar_t* filename_w;

    uefi_printf_debug("Inside custom fopen\n");

    fPtr = (EFI_FILE_HANDLE*)XMALLOC(sizeof(EFI_FILE_HANDLE),
                                        NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (fPtr == NULL) {
        uefi_printf("Malloc failed: fPtr\n");
        return NULL;
    }

/* need if for some reason we need to replace "/" with "\" dos style paths */
#ifdef WOLFSSL_NEED_DYNAMIC_PATH_FIX_UEFI
    /* Malloc out temp buffer to needed size */
    temp = (char*)XMALLOC(strlen(filename)*sizeof(char),
                            NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (temp == NULL) {
        uefi_printf("Malloc failed: temp filename\n");
        return NULL;
    }

    parseAndReplace(filename, temp, "/", "/");
    removeLeadingDot(temp);
#else
    temp = (char*)filename;
#endif

#ifdef WOLFSSL_NEED_DYNAMIC_TYPE_FIX_UEFI
    filename_w = (wchar_t*)XMALLOC(strlen(temp) * sizeof(wchar_t),
                                    NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (filename_w == NULL) {
        uefi_printf("Malloc failed: temp filename wide\n");
        return NULL;
    }
    char8_to_char16(temp, filename_w);

    if (filename == NULL) {
        uefi_printf_wolfssl("Bad Argument given to fopen for filename\n");
        return NULL;
    }

    option = getFileOperation(mode);
    if (option == NULL_ARG) {
        uefi_printf_wolfssl("Bad Argument given to to fopen for mode\n");
        return NULL;
    }
#else
    filename_w = filename;
#endif
    /* Debugging file paths for UEFI */
    /* Use %a to properly print out the string */
    uefi_printf_debug("The file being open: %a\n", filename);
#ifdef WOLFSSL_NEED_DYNAMIC_PATH_FIX_UEFI
    uefi_printf_debug("The fixed file path being open is: %a\n", temp);
#endif
    /* filename_w is proper conversion so use */
    uefi_printf_debug("The wide file being open is: %s\n", filename_w);

    volume = getVolume();

    switch (option) {
        case READ:
            /* Debug for usage */
            uefi_printf_debug("Mode: read\n");
            uefi_printf_debug("Before UEFI Wrapper\n");
            uefi_call_wrapper(volume->Open, 5, volume, fPtr, filename_w,
                                EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | \
                                EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
            uefi_printf_debug("After UEFI Wrapper\n");
            if (fPtr == NULL) {
                uefi_printf("File Pointer is NULL after Open Call\n");
                return NULL;
            }
            break;

        case WRITE:
            uefi_printf_debug("Mode: write\n");
            uefi_printf_debug("Before UEFI Wrapper\n");
            uefi_call_wrapper(volume->Open, 5, volume, fPtr, filename_w,
                                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
                                EFI_FILE_MODE_CREATE, 0);
            uefi_printf_debug("After UEFI Wrapper\n");
            if (fPtr == NULL) {
                uefi_printf("File Pointer is NULL after Open Call\n");
                return NULL;
            }
            break;

        case READWRITE:
            /* custom I/O not written for this TODO if needed */
            uefi_printf_wolfssl("Mode: %s for read/write\n", mode);
            uefi_printf_wolfssl("Not Supported yet\n");
            return NULL;

        case OPENDIR:
            uefi_printf_debug("Mode: opendir\n");
            uefi_printf_debug("Before UEFI Wrapper\n");
            status = uefi_call_wrapper(volume->Open, 5, volume, fPtr,
                                filename_w, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
            uefi_printf_debug("After UEFI Wrapper\n");
            if (fPtr == NULL || status != 0) {
                uefi_printf("File Pointer is NULL after Open Call\n");
                return NULL;
            }
            break;

        default:
            /* Mode not mapped or support yet for custom I/O */
            uefi_printf_wolfssl("Mode: %s is not supported\n", mode);
            return NULL;
    }

    if (EFI_ERROR(status)) {
        uefi_printf("Error Opening File: %r\n", status);
        return 0; /* Return 0 to indicate failure */
    }

    XFREE(temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(filename_w, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    return (FILE*)fPtr;
}

/* Closes the file associated with stream, */
/* returns 0 on success or -1 on failure */
int fclose(FILE* stream)
{
    int ret = -1;
    (void)stream;
    EFI_FILE_HANDLE* fPtr = NULL;

    uefi_printf_debug("Inside custom fclose\n");
    if (stream == NULL) {
        uefi_printf("NULL pointer given");
        return -1;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    uefi_printf_debug("Before Wrapper\n");
    uefi_call_wrapper((*fPtr)->Close, 1, *fPtr);
    uefi_printf_debug("After Wrapper\n");


    XFREE(stream, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    stream = NULL;
    uefi_printf_debug("Leaving custom fclose\n");

    return ret;
}




int fseek(FILE* stream, long offset, int whence) {
    int ret = -1; /* Default to failure */
    EFI_FILE_HANDLE* fPtr = NULL;
    UINT64 currentPosition = 0;
    UINT64 newPosition = 0;
    EFI_STATUS status;

    /* Debug message */
    uefi_printf("Inside fseek\n");

    /* Validate the file stream */
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return -1;
    }

    fPtr = (EFI_FILE_HANDLE*)stream;

    /* Handle the 'whence' parameter */
    switch (whence) {
        case SEEK_SET:
            newPosition = offset;
            break;

        case SEEK_CUR:
            /* Get the current position */
            status = uefi_call_wrapper((*fPtr)->GetPosition, 2, *fPtr, &currentPosition);
            if (EFI_ERROR(status)) {
                uefi_printf("Failed to get current position: %r\n", status);
                return -1;
            }
            newPosition = currentPosition + offset;
            break;

        case SEEK_END:
            /* Get the file size by seeking to the end */
            status = uefi_call_wrapper((*fPtr)->SetPosition, 2, *fPtr, 0xFFFFFFFFFFFFFFFF);
            if (EFI_ERROR(status)) {
                uefi_printf("Failed to get file size: %r\n", status);
                return -1;
            }
            status = uefi_call_wrapper((*fPtr)->GetPosition, 2, *fPtr, &currentPosition);
            if (EFI_ERROR(status)) {
                uefi_printf("Failed to get end position: %r\n", status);
                return -1;
            }
            newPosition = currentPosition + offset;
            break;

        default:
            uefi_printf("Invalid 'whence' value\n");
            return -1;
    }

    /* Set the new file position */
    status = uefi_call_wrapper((*fPtr)->SetPosition, 2, *fPtr, newPosition);
    if (EFI_ERROR(status)) {
        uefi_printf("Failed to set position: %r\n", status);
        return -1;
    }

    /* Success */
    ret = 0;
    uefi_printf("Successfully moved file pointer to position: %llu\n", newPosition);

    return ret;
}


/* Returns the current position of the file pointer in the stream, */
/* or -1L on failure */
long ftell(FILE* stream)
{
    EFI_FILE_HANDLE* fPtr = NULL;
    uint64_t fSize = 0;

    uefi_printf("ftell\n");
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    fSize = fileSize(*(fPtr));
    if (fileSize == 0) {
        uefi_printf("File is of size 0\n");
        return 0;
    }


    return fSize;
}


/* Reads data from the stream into ptr, */
/* returns the number of items read or 0 on failure */
size_t fread(void* ptr, size_t size, size_t count, FILE* stream)
{
    size_t ret = 0; /* Number of items successfully read */
    EFI_FILE_HANDLE* fPtr = NULL;
    uint64_t fSize = 0;
    if (count > 0) {
        size = size*count;
    }
    uefi_printf_debug("Inside custom fread\n");
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    fSize = fileSize(*(fPtr));
    if (fileSize == 0) {
        uefi_printf("File is of size 0\n");
        return 0;
    }
    if (ptr == NULL) {
        uefi_printf_debug("ptr is NULL\n");
    }
    /* Attempt to read from the file */
    uefi_printf_debug("Before Wrapper\n");
    uefi_printf_debug("Size is %d\n", fSize);
    uefi_call_wrapper((*fPtr)->Read, 3, *fPtr, &size, ptr);
    uefi_printf_debug("After Wrapper\n");

    uefi_printf_debug("Leaving custom fread\n");
    return size;
}

char *fgets(char *str, int n, FILE *stream)
{
    char* rPtr = NULL;
    uefi_printf("Inside FGETS\n");
    return rPtr;
}

int sscanf(const char *str, const char *format, ...)
{
    const char *f = format;
    const char *s = str;
    va_list args;
    int assigned = 0;

    va_start(args, format);

    while (*f) {
        if (*f == '%') {
            f++; /* Move past '%' */

            switch (*f) {
                case 'd': { /* Parse integer */
                    int *intArg = va_arg(args, int *);
                    int value = 0;
                    int negative = 0; /* 0 for positive, 1 for negative */

                    /* Skip leading whitespace */
                    while (*s == ' ' || *s == '\t') {
                        s++;
                    }

                    /* Handle sign */
                    if (*s == '-') {
                        negative = 1;
                        s++;
                    } else if (*s == '+') {
                        s++;
                    }

                    /* Parse digits */
                    while (*s >= '0' && *s <= '9') {
                        value = value * 10 + (*s - '0');
                        s++;
                    }

                    *intArg = negative ? -value : value;
                    assigned++;
                    break;
                }
                case 'f': { /* Parse floating-point */
                    float *floatArg = va_arg(args, float *);
                    float value = 0.0f;
                    float fraction = 0.0f;
                    float divisor = 10.0f;
                    int negative = 0;

                    /* Skip leading whitespace */
                    while (*s == ' ' || *s == '\t') {
                        s++;
                    }

                    /* Handle sign */
                    if (*s == '-') {
                        negative = 1;
                        s++;
                    } else if (*s == '+') {
                        s++;
                    }

                    /* Parse integer part */
                    while (*s >= '0' && *s <= '9') {
                        value = value * 10.0f + (*s - '0');
                        s++;
                    }

                    /* Parse fractional part */
                    if (*s == '.') {
                        s++;
                        while (*s >= '0' && *s <= '9') {
                            fraction += (*s - '0') / divisor;
                            divisor *= 10.0f;
                            s++;
                        }
                    }

                    *floatArg = (value + fraction) * (negative ? -1.0f : 1.0f);
                    assigned++;
                    break;
                }
                case 's': { /* Parse string */
                    char *strArg = va_arg(args, char *);

                    /* Skip leading whitespace */
                    while (*s == ' ' || *s == '\t') {
                        s++;
                    }

                    /* Copy characters until whitespace or null */
                    while (*s && *s != ' ' && *s != '\t' && *s != '\n') {
                        *strArg++ = *s++;
                    }
                    *strArg = '\0'; /* Null-terminate the string */
                    assigned++;
                    break;
                }
                default:
                    /* Unsupported format specifier */
                    va_end(args);
                    return -1;
            }
        } else if (*s == *f) {
            /* Match literal character */
            s++;
        } else {
            /* Mismatch */
            break;
        }
        f++;
    }

    va_end(args);
    return assigned;
}

int mkdir(const char *pathname, mode_t mode)
{
    int ret = -1;
    uefi_printf("Inside MKDIR\n");
    return ret;
}

DIR *opendir(const char *name)
{
    DIR* ptr = NULL;
    uefi_printf("INSIDE OPENDIR\n");
    ptr = (DIR*)fopen(name, "od");
    return ptr;
}


#include <efi.h>
#include <efilib.h>

size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream)
{
    size_t ret = 0; /* Number of items successfully written */
    EFI_FILE_HANDLE* fPtr = NULL;
    //UINTN totalBytes = count * size * 8; /* Total bytes to write */
    EFI_STATUS status;
    uefi_printf("Size: %d\n", size);
    uefi_printf("Count: %d\n", count);
    uefi_printf_debug("Inside custom fwrite\n");

    /* Check for a valid stream */
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return 0;
    }

    if (count > 0) {
        size = size*count;
    }

    fPtr = (EFI_FILE_HANDLE*)stream;

    uefi_printf_debug("Before Wrapper\n");

    /* Call EFI Write */
    status = uefi_call_wrapper((*fPtr)->Write, 3, *fPtr, &size, ptr);

    if (EFI_ERROR(status)) {
        /* Handle error */
        uefi_printf("EFI_FILE_PROTOCOL Write failed with status: %r\n", status);
        return 0;
    }

    uefi_printf_debug("Write succeeded\n");

    uefi_printf_debug("Leaving custom fwrite\n");
    return size;
}

/* Opens a directory stream for the directory specified by name, */
/* returns a DIR pointer or NULL on failure */

/* Reads the next directory entry from dirp, */
/* returns a pointer to a dirent structure or NULL on failure */
struct dirent* readdir(DIR* stream)
{
    EFI_STATUS status;
    size_t ret = 0; /* Number of items successfully read */
    struct dirent* dirPtr;
    wchar_t tempWide[8192];
    unsigned int size = 8192;
    EFI_FILE_HANDLE* fPtr = NULL;
    uint64_t fSize = 0;
    uefi_printf("INSIDE READDIR\n");
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    dirPtr = (struct dirent*)XMALLOC(sizeof(struct dirent), NULL,
                                            DYNAMIC_TYPE_TMP_BUFFER);
    if (dirPtr == NULL) {
        uefi_printf("Failed to malloc dirPtr");
        return NULL;
    }

    fSize = fileSize(*(fPtr));
    if (fileSize == 0) {
        uefi_printf("File is of size 0\n");
        return 0;
    }


    uefi_printf_debug("Size is %d\n", fSize);
    status = uefi_call_wrapper((*fPtr)->Read, 3, *fPtr, &fSize, tempWide);
    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("Returned %d\n\n", status);
        if (EFI_ERROR(status) == EFI_NO_MEDIA) {
            /* The device has no medium */
            uefi_printf_wolfssl("Error: No medium found on the device.\n");
        } else if (EFI_ERROR(status) == EFI_DEVICE_ERROR) {
            /* Multiple possible causes for EFI_DEVICE_ERROR */
            uefi_printf_wolfssl("Error: Device reported an error.\n");
            uefi_printf_wolfssl("  - This could be due to an attempt to read a deleted file.\n");
            uefi_printf_wolfssl("  - Or the current file position is beyond the end of the file.\n");
        } else if (EFI_ERROR(status) == EFI_VOLUME_CORRUPTED) {
            /* The file system structures are corrupted */
            uefi_printf_wolfssl("Error: File system is corrupted.\n");
        } else if (EFI_ERROR(status) == EFI_BUFFER_TOO_SMALL) {
            /* The buffer provided is too small */
            uefi_printf_wolfssl("Error: Buffer size is too small for the requested operation.\n");
        } else {
            /* Unhandled error code */
            uefi_printf_wolfssl("Error: Unknown EFI status: %r\n", status);
        }
        return NULL;
    }
    Print("%s\n", tempWide);
    





    status = uefi_call_wrapper((*fPtr)->Read, 3, *fPtr, &fSize, tempWide);
    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("Returned %d\n\n", status);
        if (EFI_ERROR(status) == EFI_NO_MEDIA) {
            /* The device has no medium */
            uefi_printf_wolfssl("Error: No medium found on the device.\n");
        } else if (EFI_ERROR(status) == EFI_DEVICE_ERROR) {
            /* Multiple possible causes for EFI_DEVICE_ERROR */
            uefi_printf_wolfssl("Error: Device reported an error.\n");
            uefi_printf_wolfssl("  - This could be due to an attempt to read a deleted file.\n");
            uefi_printf_wolfssl("  - Or the current file position is beyond the end of the file.\n");
        } else if (EFI_ERROR(status) == EFI_VOLUME_CORRUPTED) {
            /* The file system structures are corrupted */
            uefi_printf_wolfssl("Error: File system is corrupted.\n");
        } else if (EFI_ERROR(status) == EFI_BUFFER_TOO_SMALL) {
            /* The buffer provided is too small */
            uefi_printf_wolfssl("Error: Buffer size is too small for the requested operation.\n");
        } else {
            /* Unhandled error code */
            uefi_printf_wolfssl("Error: Unknown EFI status: %r\n", status);
        }
        return NULL;
    }
    Print("%s\n", tempWide);











    uefi_printf_debug("After Wrapper\n");

    uefi_printf_debug("Leaving custom readdir\n");
    uefi_printf("readdir\n");
    return NULL;
}

/* Closes the directory stream associated with dirp, */
/* returns 0 on success or -1 on failure */
int closedir(DIR* dirp)
{
    uefi_printf("closedir\n");
    return -1;
}

/* Retrieves information about the file or directory specified by path, */
/* returns 0 on success or -1 on failure */
int stat(const char* path, struct stat* buf)
{
    uefi_printf("stat\n");
    return -1;
}

int uefi_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{

    return 0;
}

/* Function to map modes to FILE_OPS enum*/
FILE_OPS getFileOperation(const char* mode)
{ 
    FILE_OPS option = NONE; /* Assume mode is not supported */
    if (mode == NULL) {
        uefi_printf_wolfssl("Bad Argument for Mode was NULL\n");
        option = NULL_ARG;
        return option;
    }

    if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0){
        option = READ; /* Read-only mode */
    }
    else if (strcmp(mode, "w") == 0|| strcmp(mode, "wb") == 0) {
        option = WRITE; /* Write-only mode */
    }
    else if (strcmp(mode, "r+") == 0 || strcmp(mode, "w+") == 0 ||
                strcmp(mode, "rb+") == 0 || strcmp(mode, "wb+") == 0) {
        option = READWRITE; /* Read-Write mode */
    }
    else if (strcmp(mode, "od") == 0) {
        option = OPENDIR; /* Read-Write mode */
    }
    return option;
}

EFI_FILE_HANDLE getVolume(void)
{
    EFI_LOADED_IMAGE *image = loaded_image;
    return LibOpenRoot(image->DeviceHandle);
}



/* RNG SECTION */

int uefi_random_gen(char* output, unsigned int sz)
{
    EFI_STATUS status;
    EFI_RNG_PROTOCOL* rngInterface;
    EFI_GUID gEfiRngProtocolGuid = EFI_RNG_PROTOCOL_GUID;
    EFI_RNG_ALGORITHM rngAlgo = EFI_RNG_ALGORITHM_RAW;

    /* Locate the RNG protocol */
    uefi_printf_debug("Before RNG Locate Call");
    status = LibLocateProtocol(&gEfiRngProtocolGuid, (void**)&rngInterface);
    if (EFI_ERROR(status)) {
        uefi_printf("Locate Protocol Failed\n");
        return -1; /* Fallback to 0 on error */
    }
    uefi_printf_debug("After RNG Locate Call");

    /* Generate a random number */
    uefi_printf_debug("Before Uefi Wrapper Call");
    status = uefi_call_wrapper(rngInterface->GetRNG, 4, rngInterface,
                                    &rngAlgo, sz,
                                    output);
    if (EFI_ERROR(status)) {
        uefi_printf("RNG Failed\n");
        return -1; /* Fallback to 0 on error */
    }
    uefi_printf_debug("After Uefi Wrapper Call");

    return 0;
}

double pow(double base, int exp) {
    double result = 1.0;

    /* Handle negative exponents */
    if (exp < 0) {
        base = 1.0 / base;
        exp = -exp;
    }

    /* Iterative exponentiation */
    while (exp) {
        if (exp % 2 == 1) {  /* If the exponent is odd */
            result *= base;
        }
        base *= base;  /* Square the base */
        exp /= 2;      /* Reduce the exponent by half */
    }

    return result;
}


/* Need to convert to wide char buffer before we can send it to atoi for uefi */
int atoi(const char *str)
{
    int ret;
    wchar_t* tempWide;

    tempWide = (wchar_t*)XMALLOC(strlen(str)*sizeof(wchar_t),
                                    NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (tempWide == NULL) {
        uefi_printf("UEFI wolfSSL Print Failed: wide char buffer allocation");
        return -1;
    }

    char8_to_char16(str, tempWide);
    ret = Atoi(tempWide);
    XFREE(tempWide, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}