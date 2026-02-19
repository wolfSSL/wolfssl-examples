#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <string.h>
#include <uchar.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <utility_wolf.h>

#define STR_SIZE 512
#define MSG_BUFFER_TMP 1024

#define uefi_printf AsciiPrint /* Native Print */

/* Used to debug verbosely to see where code fails, or gets hung */
#ifdef UEFI_VERBOSE_DEBUG
    #define uefi_printf_debug uefi_printf_wolfssl
#else
    #define uefi_printf_debug(...) ((void)0)
#endif

#ifdef UEFI_LOG_PRINTF

extern FILE* printfLog;

#endif

/* Needed from main */
extern EFI_LOADED_IMAGE *loaded_image;
/* Function to parse and replace a search string with a replacement string */
/* used to find options such %s and replace with %a, but is setup to parse */
/* any needed string */

int parseAndReplace(const char* msg, char* temp, const char* search,
                        const char* replace)
{
    size_t searchLen = XSTRLEN(search);
    size_t replaceLen = XSTRLEN(replace);
    size_t tempIndex = 0;
    size_t msgIndex = 0;
    if (searchLen == 0) {
        return -1; /* Invalid search string */
    }

    while (msg[msgIndex] != '\0' && tempIndex < MSG_BUFFER_TMP - 1) {
        /* Check if the search string matches at the current position */
        if (XSTRNCMP(&msg[msgIndex], search, searchLen) == 0) {
            /* Ensure there's enough space in temp for the replacement */
            if (tempIndex + replaceLen >= MSG_BUFFER_TMP - 1) {
                break;
            }

            /* Copy the replacement string */
            XMEMCPY(&temp[tempIndex], replace, replaceLen);
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
    return 0;
}

unsigned int calculateBufferSize(const char* msg, va_list args)
{
    va_list args_copy;
    unsigned int size = 0;
    const char* p = msg;

    if (msg == NULL) {
        return 0;
    }

    va_copy(args_copy, args);
    size = XSTRLEN(msg) + 1;

    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++; // Move past '%'
            if (*p == 's' || *p == 'a') { // Handle strings
                char* str = va_arg(args_copy, char*);
                if (str) {
                    size += XSTRLEN(str); // Add the length of the string
                }
            } else if (*p == 'd' || *p == 'u' || *p == 'x') {
                va_arg(args_copy, int); // Skip integers
            } else if (*p == 'f') {
                va_arg(args_copy, double); // Skip doubles
            }
        }
        p++;
    }

    va_end(args_copy);
    return size;
}

int uefi_vsnprintf_wolfssl(char* buffer, size_t size, const char* format,
                                va_list args)
{
    int result;
    char* tempFormat = NULL;
    va_list argsCpy;
    va_copy(argsCpy, args);
    unsigned int fmtSize = strlena(format);
#ifdef UEFI_VECTOR_TEST
    int check = 1;
#endif
    if (buffer == NULL || format == NULL) {
        AsciiPrint("Null Buffer given to snprintf");
        return -1;
    }

    tempFormat = (char*)XMALLOC(2 * fmtSize*sizeof(char), NULL,
                                    DYNAMIC_TYPE_TMP_BUFFER);
    if (tempFormat == NULL) {
        AsciiPrint("Failed to allocate temp buffer");
        return -1;
    }


    /* Need to replace certain charaters */
    parseAndReplace(format, tempFormat, "%s", "%a");

/* Needed to run benchmark utility */
#ifdef UEFI_BENCHMARK
    parseAndReplace(tempFormat, tempFormat, "%8s", "%8a");
    parseAndReplace(tempFormat, tempFormat, "%15s", "%15a");
    parseAndReplace(tempFormat, tempFormat, "%-2s", "%-2a");
    parseAndReplace(tempFormat, tempFormat, "%-5s", "%-5a");
    parseAndReplace(tempFormat, tempFormat, "%-6s", "%-6a");
    parseAndReplace(tempFormat, tempFormat, "%-9s", "%-9a");
    parseAndReplace(tempFormat, tempFormat, "%-16s", "%-16a");
    parseAndReplace(tempFormat, tempFormat, "%-24s", "%-24a");
#endif

/* Needed for Internal vector tests */
#ifdef UEFI_VECTOR_TEST
    check = parseAndReplace(tempFormat, tempFormat, "%1.15g", "%1.0f");
    check = parseAndReplace(tempFormat, tempFormat, "%1.17g", "%1.0f");
#endif

#ifdef NO_MAIN_OPTEST_DRIVER
    parseAndReplace(tempFormat, tempFormat, "%25s", "%25a");
    parseAndReplace(tempFormat, tempFormat, "%10s", "%10a");
#ifdef OPTEST_INVALID_LOGGING_ENABLED
    parseAndReplace(tempFormat, tempFormat, "%*s", "%*a");
#endif
#endif


    /* Pass the variadic arguments to AsciiVSPrintf */
    result = (int)AsciiVSPrint(buffer, size, tempFormat, args);

    /* Needed for internal vector tests */
#ifdef UEFI_VECTOR_TEST
    if (check == 0) {
        parseAndReplace(buffer, buffer, ".0", " \b");
    }
#endif

    va_end(argsCpy);
    XFREE(tempFormat, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    return result;
}

int uefi_snprintf_wolfssl(char* buffer, size_t n, const char* format, ...)
{
    int result;
    char* tempFormat = NULL;
    va_list args;
    if (buffer == NULL || format == NULL) {
        AsciiPrint("Null Buffer given to snprintf");
        return -1;
    }

    /* Initialize args to store all values after 'format' */
    va_start(args, format);

    /* Just pass to vsnprintf */
    result = uefi_vsnprintf_wolfssl(buffer, n, format, args);

    /* Clean up the va_list */
    va_end(args);

    return result;
}

/* Wrap because AsciiPrint returns unsigned int, printf return a int normally */
int uefi_printf_wolfssl(const char* msg, ...)
{
    int ret = 0;
    va_list args, argsCpy;
    char* temp = NULL;
    size_t size;

    if (msg == NULL) {
        AsciiPrint("NULL sent to uefi_printf_wolfssl");
        return -1;
    }

    /* Initialize variadic argument list */
    va_start(args, msg);
    va_copy(argsCpy, args);

    /* Estimate needed buffer and add a temp size for args */
    size = ((XSTRLEN(msg) + calculateBufferSize(msg, argsCpy) + MSG_BUFFER_TMP));

    temp = (char*)XMALLOC(2 * size*sizeof(char), NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (temp == NULL) {
        AsciiPrint("Failed to allocate temp buffer\n");
        return -1;
    }

    /* Pass to vsnprintf */
    uefi_vsnprintf_wolfssl(temp, size, msg, args);

    /* Use AsciiPrint to print the formatted message */
    ret = (int)AsciiPrint("%a", temp);
#ifdef UEFI_LOG_PRINTF
    if (printfLog != NULL) {
        fwrite(temp, strlena(temp), 1, printfLog);
    }
    else {
        Print(L"Logging file in NULL prt\n");
    }
#endif
    /* Clean up */
    va_end(argsCpy);
    va_end(args);
    XFREE(temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}

int uefi_vprintf_wolfssl(const char* msg, va_list args)
{
    int ret = 0;
    int size = 0;
    char* temp = NULL;
    va_list argsCpy;

    if (msg == NULL) {
        AsciiPrint("NULL sent to uefi_vprintf_wolfssl");
        return -1;
    }

    /* Initialize variadic argument list */
    va_copy(argsCpy, args);

    /* Estimate needed buffer and add a temp size for args */
    size = ((XSTRLEN(msg)+calculateBufferSize(msg, argsCpy)+MSG_BUFFER_TMP));

    temp = (char*)XMALLOC(2 * size*sizeof(char), NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (temp == NULL) {
        AsciiPrint("Failed to allocate temp buffer\n");
        return -1;
    }

    uefi_vsnprintf_wolfssl(temp, size, msg, args);

    /* Use AsciiPrint to print the formatted message */
    ret = (int)AsciiPrint("%a", temp);

    /* Clean up */
    va_end(argsCpy);
    XFREE(temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}

int uefi_fprintf_wolfssl(FILE* stream, const char* format, ...)
{
    unsigned int ret = 0;

    if (stream == NULL || format == NULL) {
        return -1;
    }

    AsciiPrint("fPrintf() Not Implemented\n");

    va_list args;
    va_start(args, format);


    va_end(args);
    return ret;
}

/* Helper function to write an integer to the stream */
static int write_integer(FILE* stream, int value) {
    char buffer[12];
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
    size_t len = XSTRLEN(str);
    size_t written = fwrite(str, sizeof(char), len, stream);
    return (written == len) ? len : -1;
}

/* Convert standard 8 bit char to 16 bit wide char */
void char8_to_char16(const char* str8, char16_t* str16)
{
    char8_to_char16_ex(str8, str16, XSTRLEN(str8));
    return;
}

/* Convert standard 8 bit char to 16 bit wide char only copy based on n */
void char8_to_char16_ex(const char* str8, char16_t* str16, int n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        str16[i] = (char16_t)str8[i];
    }
    str16[i] = L'\0';
    return;
}

/* Returns the filesize of given handle */
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


int uefi_wolfssl_fflush(FILE* stream)
{
    size_t ret = 0; /* Number of items successfully read */
    EFI_FILE_HANDLE* fPtr = NULL;
    uefi_printf_debug("Inside custom fflush\n");
    if (stream == NULL) {
        uefi_printf_wolfssl("NULL given to FFLUSH\n");
        return -1;
    }
    if (stream == stdout) {
        SIMPLE_TEXT_OUTPUT_INTERFACE* uefi_stdout = NULL;
        uefi_stdout = (SIMPLE_TEXT_OUTPUT_INTERFACE*)stream;
        uefi_call_wrapper(uefi_stdout->OutputString, 1, uefi_stdout, L"\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;


    /* Attempt to read from the file */
    uefi_call_wrapper((*fPtr)->Flush, 1, *fPtr);

    return 0;
}

void exit(int n)
{
    Exit(n, 0, (CHAR16*)"Exiting Called");
}

void logging_cb(const int logLevel, const char *const logMessage)
{
	char16_t str16[STR_SIZE];
	char8_to_char16(logMessage, str16);
    uefi_printf_wolfssl("%s", str16);
}

void *XMALLOC(size_t n, void* heap, int type)
{
    void* newBuffer;
    newBuffer = AllocateZeroPool(n);
    if (newBuffer == NULL) {
        AsciiPrint("Malloc Failed: return NULL\n");
        return NULL;
    }
    return newBuffer;
}

/* Dirty Memcopy.... For proper we have to pass the old buffer size to UEFI */
/* To use ReallocatePool function..... */
void *XREALLOC(void *p, size_t n, void* heap, int type)
{
    void* newBuffer = NULL;
    (void)heap;
    (void)type;

    if (n == 0) {
        AsciiPrint("Size 0 given returning NULL...\n");
        FreePool(p);
        p = NULL;
        return NULL;
    }

    /* Allocate new Buffer */
    newBuffer = AllocateZeroPool(n);
    if (newBuffer == NULL) {
        AsciiPrint("Realloc Failed: return NULL\n");
        FreePool(p);
        return NULL;
    }
    if (p == NULL) {
        return newBuffer;
    }

    /* preform XMEMCPY on old buffer using n.... */
    XMEMCPY(newBuffer, p, n);

    /* Free Old Buffer */
    FreePool(p);
    p = NULL;
    return newBuffer;
}

void XFREE(void *p, void* heap, int type)
{
    FreePool(p);
    p = NULL;
    return;
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
    uefi_printf_wolfssl("Inside Read Open\n");
    return -1;
}


ssize_t write(int fd, const void* buf, size_t cnt)
{
    uefi_printf_wolfssl("Inside Write Close\n");
    return -1;
}

/* Opens a file specified by filename and mode, */
/* returns a FILE pointer or NULL on failure */
/* UEFI requires that the filepath be in wchar format aka unicode */
/* however fopen is specified as taking in a const char buffer */
/* so custom fopen will assume this is the case, but then convert */
/* the buffer to wchar */
FILE* fopen(const char* filename, const char* mode)
{
    EFI_FILE_HANDLE* fPtr = NULL;
    FILE_OPS option = NONE;
    EFI_FILE_HANDLE volume;
    EFI_STATUS status;
    uint64_t size;
    /* Temporary buffer pointer */
    char* temp;
    /* Since fopen is */
    char16_t* filename_w;

    uefi_printf_debug("Inside custom fopen\n");

    fPtr = (EFI_FILE_HANDLE*)XMALLOC(sizeof(EFI_FILE_HANDLE),
                                        NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (fPtr == NULL) {
        uefi_printf_wolfssl("Malloc failed: fPtr\n");
        return NULL;
    }

/* need if for some reason we need to replace "/" with "\" dos style paths */
#ifdef NEED_DYNAMIC_PATH_FIX_UEFI
    uefi_printf_wolfssl("Filename: %s\n", filename);
    /* Malloc out temp buffer to needed size */
    temp = (char*)XMALLOC(XSTRLEN(filename)*sizeof(char),
                            NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (temp == NULL) {
        uefi_printf_wolfssl("Malloc failed: temp filename\n");
        return NULL;
    }

    parseAndReplace(filename, temp, "/", "\\");
    uefi_printf_wolfssl("Filename After: %s\n", filename);
    //parseAndReplace(filename, temp, "./", "\\");
#else
    temp = (char*)filename;
#endif

#ifdef NEED_DYNAMIC_TYPE_FIX_UEFI
    filename_w = (char16_t*)XMALLOC(XSTRLEN(temp) * sizeof(char16_t),
                                    NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (filename_w == NULL) {
        uefi_printf_wolfssl("Malloc failed: temp filename wide\n");
        return NULL;
    }
    char8_to_char16(temp, filename_w);

    if (filename == NULL) {
        uefi_printf_wolfssl("Bad Argument given to fopen for filename\n");
        return NULL;
    }
#else
    filename_w = filename;
#endif
    /* Debugging file paths for UEFI */
    /* Use %a to properly print out the string */
    uefi_printf_debug("The file being open: %a\n", filename);
#ifdef NEED_DYNAMIC_PATH_FIX_UEFI
    uefi_printf_debug("The fixed file path being open is: %a\n", temp);
#endif
    /* filename_w is proper conversion so use */
    uefi_printf_debug("The wide file being open is: %s\n", filename_w);

    option = getFileOperation(mode);
    if (option == NULL_ARG) {
        uefi_printf_wolfssl("Bad Argument given to to fopen for mode\n");
        return NULL;
    }

    volume = getVolume();

    switch (option) {
        case READ:
            /* Debug for usage */
            uefi_printf_debug("Mode: read\n");
            uefi_call_wrapper(volume->Open, 5, volume, fPtr, filename_w,
                                EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | \
                                EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
            if (fPtr == NULL) {
                uefi_printf_wolfssl("File Pointer is NULL after Open Call\n");
                return NULL;
            }
            break;

        case WRITE:
            uefi_printf_debug("Mode: write\n");
            uefi_call_wrapper(volume->Open, 5, volume, fPtr, filename_w,
                                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
                                EFI_FILE_MODE_CREATE, 0);
            if (fPtr == NULL) {
                uefi_printf_wolfssl("File Pointer is NULL after Open Call\n");
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
            status = uefi_call_wrapper(volume->Open, 5, volume, fPtr,
                                filename_w, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
            if (fPtr == NULL || status != 0) {
                uefi_printf_wolfssl("File Pointer is NULL after Open Call\n");
                return NULL;
            }
            break;

        default:
            /* Mode not mapped or support yet for custom I/O */
            uefi_printf_wolfssl("Fopen Mode: %s is not supported\n", mode);
            return NULL;
    }

    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("Error Opening File: %r\n", status);
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
        uefi_printf_wolfssl("NULL pointer given");
        return -1;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    uefi_call_wrapper((*fPtr)->Close, 1, *fPtr);


    XFREE(fPtr, NULL, DYNAMIC_TYPE_TMP_BUFFER);
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

    /* Validate the file stream */
    if (stream == NULL) {
        uefi_printf_wolfssl("Bad File Pointer Argument\n");
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
                uefi_printf_wolfssl("Failed to get current position: %r\n", status);
                return -1;
            }
            newPosition = currentPosition + offset;
            break;

        case SEEK_END:
            /* Get the file size by seeking to the end */
            status = uefi_call_wrapper((*fPtr)->SetPosition, 2, *fPtr, 0xFFFFFFFFFFFFFFFF);
            if (EFI_ERROR(status)) {
                uefi_printf_wolfssl("Failed to get file size: %r\n", status);
                return -1;
            }
            status = uefi_call_wrapper((*fPtr)->GetPosition, 2, *fPtr, &currentPosition);
            if (EFI_ERROR(status)) {
                uefi_printf_wolfssl("Failed to get end position: %r\n", status);
                return -1;
            }
            newPosition = currentPosition + offset;
            break;

        default:
            uefi_printf_wolfssl("Invalid 'whence' value\n");
            return -1;
    }

    /* Set the new file position */
    status = uefi_call_wrapper((*fPtr)->SetPosition, 2, *fPtr, newPosition);
    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("Failed to set position: %r\n", status);
        return -1;
    }

    /* Success */
    ret = 0;

    return ret;
}


/* Returns the current position of the file pointer in the stream, */
/* or -1L on failure */
long ftell(FILE* stream)
{
    EFI_FILE_HANDLE* fPtr = NULL;
    uint64_t fSize = 0;

    if (stream == NULL) {
        uefi_printf_wolfssl("Bad File Pointer Argument\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    fSize = fileSize(*(fPtr));
    if (fileSize == 0) {
        uefi_printf_wolfssl("File is of size 0\n");
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
    if (stream == NULL) {
        uefi_printf_wolfssl("Bad File Pointer Argument\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    fSize = fileSize(*(fPtr));
    if (fileSize == 0) {
        uefi_printf_wolfssl("File is of size 0\n");
        return 0;
    }
    if (ptr == NULL) {
        uefi_printf_debug("ptr is NULL\n");
    }
    /* Attempt to read from the file */
    uefi_call_wrapper((*fPtr)->Read, 3, *fPtr, &size, ptr);
    return size;
}

char *fgets(char* str, int n, FILE* stream)
{
    char* rPtr = NULL;
    char* tempRead = NULL;
    int ret = 0;
    int i = 0;
    UINTN len = 1;

    if (str == NULL || stream == NULL) {
        uefi_printf_wolfssl("NULL Arg given to fgets\n");
        return NULL;
    }

    tempRead = (char*)XMALLOC((n+1*sizeof(char)), NULL,
                                    DYNAMIC_TYPE_TMP_BUFFER);
    if (tempRead == NULL) {
        uefi_printf_wolfssl("Malloc Failed in FGETS\n");
        return NULL;
    }

    /* read in 1 char at a time */
    for (i = 0; i < n; i++) {
        len = 1;
        ret = ReadSimpleReadFile(stream, i, &len, &tempRead[i]);
        if (ret == 0) {
            /* Break out if nothing was read in */
            break;
        }
        if (tempRead[i] == '\n') {
            break;
        }
    }

    /* Nothing at all was read return NULL */
    if (i == 0) {
        rPtr = NULL;
    }
    else {
        if (tempRead[i] != '\0') {
            tempRead[i+1] = '\0';
        }
        rPtr = str;
    }

    XFREE(tempRead, NULL, DYNAMIC_TYPE_TMP_BUFFER);

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
                case 'l': { /* Handle 'l' length modifier */
                    f++; /* Move past 'l' */
                    if (*f == 'd') { /* Parse long integer */
                        long *longArg = va_arg(args, long *);
                        long value = 0;
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

                        /* Parse digits */
                        while (*s >= '0' && *s <= '9') {
                            value = value * 10 + (*s - '0');
                            s++;
                        }

                        *longArg = negative ? -value : value;
                        assigned++;
                    } else if (*f == 'u') { /* Parse unsigned long integer */
                        unsigned long *ulongArg = va_arg(args, unsigned long *);
                        unsigned long value = 0;

                        /* Skip leading whitespace */
                        while (*s == ' ' || *s == '\t') {
                            s++;
                        }

                        /* Parse digits */
                        while (*s >= '0' && *s <= '9') {
                            value = value * 10 + (*s - '0');
                            s++;
                        }

                        *ulongArg = value;
                        assigned++;
                    } else if (*f == 'f') { /* Parse double */
                        double *doubleArg = va_arg(args, double *);
                        double value = 0.0;
                        double fraction = 0.0;
                        double divisor = 10.0;
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
                            value = value * 10.0 + (*s - '0');
                            s++;
                        }

                        /* Parse fractional part */
                        if (*s == '.') {
                            s++;
                            while (*s >= '0' && *s <= '9') {
                                fraction += (*s - '0') / divisor;
                                divisor *= 10.0;
                                s++;
                            }
                        }

                        *doubleArg = (value + fraction) * (negative ? -1.0 : 1.0);
                        assigned++;
                    } else {
                        /* Unsupported 'l' format */
                        va_end(args);
                        return -1;
                    }
                    break;
                }
                default:
                    /* Unsupported format specifier */
                    uefi_printf_wolfssl("SSCANF Case Not Supported: %c\n", *f);
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
    uefi_printf_wolfssl("MKDIR not Implemented\n");
    return ret;
}

DIR *opendir(const char *name)
{
    DIR* ptr = NULL;
    uefi_printf_wolfssl("OPENDIR not Implemented\n");
    return ptr;
}

size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream)
{
    size_t ret = 0; /* Number of items successfully written */
    EFI_FILE_HANDLE* fPtr = NULL;
    //UINTN totalBytes = count * size * 8; /* Total bytes to write */
    EFI_STATUS status;
    uefi_printf_debug("Inside custom fwrite\n");

    /* Check for a valid stream */
    if (stream == NULL) {
        uefi_printf_wolfssl("Bad File Pointer Argument\n");
        return 0;
    }

    if (count > 0) {
        size = size*count;
    }

    fPtr = (EFI_FILE_HANDLE*)stream;

    /* Call EFI Write */
    status = uefi_call_wrapper((*fPtr)->Write, 3, *fPtr, &size, ptr);

    if (EFI_ERROR(status)) {
        /* Handle error */
        uefi_printf_wolfssl("EFI_FILE_PROTOCOL Write failed with status: %r\n", status);
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
    char16_t tempWide[8192];
    unsigned int size = 8192;
    EFI_FILE_HANDLE* fPtr = NULL;
    uint64_t fSize = 0;
    if (stream == NULL) {
        uefi_printf_wolfssl("Bad File Pointer Argument\n");
        return 0;
    }

#ifdef UEFI_READDIR /* work in progress */
    fPtr = (EFI_FILE_HANDLE*)stream;

    dirPtr = (struct dirent*)XMALLOC(sizeof(struct dirent), NULL,
                                            DYNAMIC_TYPE_TMP_BUFFER);
    if (dirPtr == NULL) {
        uefi_printf_wolfssl("Failed to malloc dirPtr");
        return NULL;
    }

    fSize = fileSize(*(fPtr));
    if (fileSize == 0) {
        uefi_printf_wolfssl("File is of size 0\n");
        return 0;
    }

    uefi_printf_debug("Size of file being read: %d\n", fSize);
    status = uefi_call_wrapper((*fPtr)->Read, 3, *fPtr, &fSize, tempWide);
    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("Returned %d\n\n", status);
        if (EFI_ERROR(status) == EFI_NO_MEDIA) {
            uefi_printf_wolfssl("Error: No medium found on the device.\n");
        } else if (EFI_ERROR(status) == EFI_DEVICE_ERROR) {
            uefi_printf_wolfssl("Error: Device reported an error.\n");
            uefi_printf_wolfssl("  - This could be due to an attempt to read a deleted file.\n");
            uefi_printf_wolfssl("  - Or the current file position is beyond the end of the file.\n");
        } else if (EFI_ERROR(status) == EFI_VOLUME_CORRUPTED) {
            uefi_printf_wolfssl("Error: File system is corrupted.\n");
        } else if (EFI_ERROR(status) == EFI_BUFFER_TOO_SMALL) {
            /* The buffer provided is too small */
            uefi_printf_wolfssl("Error: Buffer size is too small for the requested operation.\n");
        } else {
            uefi_printf_wolfssl("Error: Unknown EFI status: %r\n", status);
        }
        return NULL;
    }
    uefi_printf_debug("Leaving custom readdir\n");
#else
    uefi_printf_wolfssl("READDIR not implemented\n");
#endif
    return NULL;
}

/* Closes the directory stream associated with dirp, */
/* returns 0 on success or -1 on failure */
int closedir(DIR* dirp)
{
    uefi_printf_wolfssl("CLOSEDIR not implemented\n");
    return -1;
}

/* Retrieves information about the file or directory specified by path, */
/* returns 0 on success or -1 on failure */
int stat(const char* path, struct stat* buf)
{
    uefi_printf_wolfssl("STAT not implemented\n");
    return -1;
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

    if (XSTRCMP(mode, "r") == 0 || strcmp(mode, "rb") == 0){
        option = READ; /* Read-only mode */
    }
    else if (XSTRCMP(mode, "w") == 0|| strcmp(mode, "wb") == 0) {
        option = WRITE; /* Write-only mode */
    }
    else if (XSTRCMP(mode, "r+") == 0 || strcmp(mode, "w+") == 0 ||
                XSTRCMP(mode, "rb+") == 0 || strcmp(mode, "wb+") == 0) {
        option = READWRITE; /* Read-Write mode */
    }
    else if (XSTRCMP(mode, "od") == 0) {
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
/* Use to seed the HASH DRGB */


/* Functions just incase EFI_RNG Protocal Does not exist */
static unsigned int seed;

void set_seed(double new_seed) {
    seed = (unsigned int)new_seed;
}

static unsigned int pseudo_rng() {
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return seed;
}



int uefi_random_gen(char* output, unsigned int sz)
{
    EFI_STATUS status;
    EFI_RNG_PROTOCOL* rngInterface;
    EFI_GUID gEfiRngProtocolGuid = EFI_RNG_PROTOCOL_GUID;
    EFI_RNG_ALGORITHM rngAlgo = EFI_RNG_ALGORITHM_RAW;

    /* Locate the RNG protocol */
    status = LibLocateProtocol(&gEfiRngProtocolGuid, (void**)&rngInterface);
    if (EFI_ERROR(status)) {
        /* Use fake rng seeded by time if functions are not avaliable */
        uefi_printf_debug("No RNG Avaliable using Alternate\n");
        set_seed(current_time(0));
        for (int i = 0; i < sz; i++) {
            output[i] = (char)(pseudo_rng() & 0xFF);
        }
        return 0; /* Fallback to time */
    }

    /* Generate a random number */
    status = uefi_call_wrapper(rngInterface->GetRNG, 4, rngInterface,
                                    &rngAlgo, sz,
                                    output);
    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("RNG Failed\n");
        return -1; /* Fallback to 0 on error */
    }

    return 0;
}

/* Time function */

unsigned long convertToEpochUefi(EFI_TIME ts)
{
    /* Calculate the total days since the Unix epoch (1970-01-01) */
    unsigned long days_since_epoch =
        (ts.Year - 1970) * 365 +
        (ts.Year - 1969) / 4 -  /* Leap years */
        (ts.Year - 1901) / 100 +
        (ts.Year - 1601) / 400 +
        (367 * ts.Month - 362) / 12 +
        ts.Day - 1;

    /* Adjust for months after February for leap years */
    if (ts.Month <= 2) {
        days_since_epoch -= 1; /* Remove 1 day for pre-March months */
        if ((ts.Year % 4 == 0 && (ts.Year % 100 != 0 || ts.Year % 400 == 0))) {
            days_since_epoch += 1; /* Add back 1 day if it's a leap year */
        }
    }

    /* Calculate total seconds since epoch */
    unsigned long total_seconds =
        days_since_epoch * 86400 + /* Convert days to seconds */
        ts.Hour * 3600 +
        ts.Minute * 60 +
        ts.Second;

    /* Adjust for timezone if specified */
    if (ts.TimeZone != 2047) {
        total_seconds -= ts.TimeZone * 60; /* TimeZone is in minutes */
    }

    return total_seconds;
}


int uefi_timeStruct_wolfssl(EFI_TIME* timeStruct)
{
    EFI_STATUS status;

    status = uefi_call_wrapper(RT->GetTime, 2, timeStruct, NULL);
    if (EFI_ERROR(status)) {
        uefi_printf_wolfssl("Failed to get time\n");
        return -1; /* Fallback to 0 on error */
    }

    return 0;
}

unsigned long uefi_time_wolfssl(unsigned long* timer)
{
    EFI_TIME time_uefi;
    unsigned long epoch = 0;

    if (uefi_timeStruct_wolfssl(&time_uefi)) {
        uefi_printf_wolfssl("Failed to get time\n");
        return 0;
    }
    epoch = convertToEpochUefi(time_uefi);
    if (timer != NULL) {
        *timer = epoch;
    }
    else {
        uefi_printf_wolfssl("Bad timer argumnet\n");
    }

    return epoch;
}

unsigned long long current_time(int reset)
{
    (void)reset;
    unsigned long long seconds = 0;
    unsigned long long result  = 0;
    EFI_TIME time_uefi;

    if (uefi_timeStruct_wolfssl(&time_uefi)) {
        uefi_printf_wolfssl("Failed to get time\n");
        return 0;
    }

    /* Calculate days since epoch directly using the struct fields */
       long days_since_epoch =
        (time_uefi.Year - 1970) * 365 +
        (time_uefi.Year - 1969) / 4 -
        (time_uefi.Year - 1901) / 100 +
        (time_uefi.Year - 1601) / 400 +
        (367 * time_uefi.Month - 362) / 12 +
        time_uefi.Day - 1;

    /* Adjust for months after February */
    if (time_uefi.Month <= 2) {
        days_since_epoch -= 1;
        if ((time_uefi.Year % 4 == 0 && (time_uefi.Year % 100 != 0 || time_uefi.Year % 400 == 0))) {
            days_since_epoch += 1;
        }
    }

    /* Convert to total seconds */
    unsigned long long total_seconds =
        //(days_since_epoch * 86400) + // Uncomment to return epoch time
        (time_uefi.Hour * 3600) +
        (time_uefi.Minute * 60) +
        time_uefi.Second;

    /* Adjust for timezone if specified */
    if (time_uefi.TimeZone != 2047) {
        total_seconds -= time_uefi.TimeZone * 60;
    }

    /* Calculate seconds as a double */
#ifdef BENCH_MICROSECOND
    seconds = (total_seconds * 1000000) + (unsigned long long)(time_uefi.Nanosecond / 1000);
#else
    seconds = total_seconds + (unsigned long long)(time_uefi.Nanosecond / 1000000000);
#endif

    result = seconds;
    return result;
}

/* Need to convert to wide char buffer before we can send it to atoi for uefi */
int atoi(const char *str)
{
    int ret;
    char16_t* tempWide;

    tempWide = (char16_t*)XMALLOC(XSTRLEN(str)*sizeof(char16_t),
                                    NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (tempWide == NULL) {
        uefi_printf_wolfssl("UEFI wolfSSL Print Failed: wide char buffer allocation");
        return -1;
    }

    char8_to_char16(str, tempWide);
    ret = Atoi(tempWide);
    XFREE(tempWide, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}

void *uefi_memcpy_wolfssl(void *dst, const void *src, size_t n)
{
    size_t i;
    const char *s = (const char *)src;
    char *d = (char *)dst;

    /* is 32-bit aligned pointer */
    if (((size_t)dst & (sizeof(unsigned long)-1)) == 0 &&
        ((size_t)src & (sizeof(unsigned long)-1)) == 0)
    {
        while (n >= sizeof(unsigned long)) {
            *(unsigned long*)d = *(unsigned long*)s;
            d += sizeof(unsigned long);
            s += sizeof(unsigned long);
            n -= sizeof(unsigned long);
        }
    }
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *uefi_memmove_wolfssl(void *dst, const void *src, size_t n)
{
    size_t i;
    const char *s = (const char *)src;
    char *d = (char *)dst;

    if (d < s) {
        /* Copy forward */
        for (i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        /* Copy backward */
        for (i = n; i != 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    /* If d == s, do nothing */
    return dst;
}

void* uefi_memset_wolfssl(void *str, int c, size_t n)
{
    unsigned char* dst = (unsigned char*)str;
    unsigned char value = (unsigned char)c;

    if (dst == NULL || n == 0)
        return str;

    while (n-- > 0) {
        *dst++ = value;
    }

    return str;
}

int uefi_memcmp_wolfssl(const void *s1, const void *s2, size_t n)
{
    int ret = 0;
    if (n != 0) {
        const uint8_t *p1 = s1, *p2 = s2;
        do {
            if (*p1++ != *p2++) {
                ret = *--p1 - *--p2;
                break;
            }
        } while (--n != 0);
    }
    return ret;
}

int uefi_strlen_wolfssl(const char *s)
{
    int ret = -1;
    if (!s)
        return ret;
    while (s[++ret] != '\0')
        ;
    return ret;
}

/* do all allocation based on n */
int uefi_strncmp_wolfssl(const char *s1, const char *s2, size_t n)
{
    int ret = -1;
    char16_t* s1_temp;
    char16_t* s2_temp;
    int s1_len;
    int s2_len;

    if (s1 == NULL || s2 == NULL) {
        uefi_printf_wolfssl("Null Args to XSTRNCMP\n");
    }

    s1_len = n*sizeof(char16_t);
    s2_len = n*sizeof(char16_t);

    /* Nothing to compare */
    if ((s1_len == 0 && s2_len == 0) || n == 0) {
        return 0;
    }

    s1_temp = (char16_t*)XMALLOC(s1_len, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (s1_temp == NULL) {
        uefi_printf_wolfssl("Failed to malloc s1 in XSTRNCMP\n");
        return -1;
    }
    char8_to_char16_ex(s1, s1_temp, n);

    s2_temp = (char16_t*)XMALLOC(s2_len, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (s2_temp == NULL) {
        uefi_printf_wolfssl("Failed to malloc s2 in XSTRNCMP\n");
        XFREE(s1_temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        return -1;
    }
    char8_to_char16_ex(s2, s2_temp, n);

    ret = StrnCmp(s1_temp, s2_temp, n);

    XFREE(s1_temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(s2_temp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}

int uefi_strcmp_wolfssl(const char *s1, const char *s2)
{
    int i = 0;
    int ret = 0;
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        i++;
    }
    if (s1[i] == '\0' && s2[i] == '\0') {
        return 0;
    }
    if (s1[i] == '\0') {
        return -1;
    }
    return 1;
}

char *uefi_strncpy_wolfssl(char *dest, const char *src, size_t n)
{
    size_t i = 0;
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *uefi_strcpy_wolfssl(char *dest, const char *src)
{
    size_t i = 0;
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    for (i = 0; src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}
int uefi_strcasecmp_wolfssl(const char *s1, const char *s2)
{
    int diff = 0;

    while (!diff && *s1) {
        diff = (int)*s1 - (int)*s2;

        if ((diff == 'A' - 'a') || (diff == 'a' - 'A'))
            diff = 0;

        s1++;
        s2++;
    }

    return diff;
}

int uefi_strncasecmp_wolfssl(const char *s1, const char *s2, size_t n)
{
    int diff = 0;
    size_t i = 0;

    while (!diff && *s1) {
        diff = (int)*s1 - (int)*s2;

        if ((diff == 'A' - 'a') || (diff == 'a' - 'A'))
            diff = 0;

        s1++;
        s2++;
        if (++i > n)
            break;
    }
    return diff;
}

char *uefi_strnstr_wolfssl(const char *s1, const char *s2, size_t n)
{
    size_t i = 0;
    size_t j = 0;
    size_t len = 0;

    if (!s1 || !s2)
        return NULL;

    len = XSTRLEN(s2);

    if (len > n)
        return NULL;

    for (i = 0; i < n; i++) {
        if (s1[i] == s2[0]) {
            for (j = 0; j < len; j++) {
                if (s1[i + j] != s2[j])
                    break;
            }
            if (j == len)
                return (char *)(uintptr_t)(s1 + i);
        }
    }
    return NULL;
}

char *uefi_strncat_wolfssl(char *dest, const char *src, size_t n)
{
    size_t len = 0;
    size_t i = 0;

    if (!dest || !src)
        return dest;

    len = XSTRLEN(dest);

    if (len >= n)
        return dest;

    for (i = 0; i < (n - len) && src[i] != '\0'; i++) {
        dest[len + i] = src[i];
    }
    dest[len + i] = '\0';
    return dest;
}

unsigned int sleep(unsigned int seconds)
{
    unsigned int i = 0;
    for (i = 0; i < (1000000 * seconds); i++) {
    }
    return seconds;
}
