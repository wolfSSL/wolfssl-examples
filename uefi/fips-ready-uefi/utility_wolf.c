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


/* Wrapper for fprintf function */
/* Provides a formatted output to a specified stream */
int uefi_fprintf(FILE* stream, const char* format, ...)
{
    /* Add implementation here */
    return 0;
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


/* USER FILE IO SECTION */




ssize_t read(int fd, void *buf, size_t cnt)
{
    uefi_printf("open\n");
    return -1;
}


ssize_t write(int fd, const void* buf, size_t cnt)
{
    uefi_printf("close\n");
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
            uefi_printf_debug("After UEFI Wrapper");
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

/* Moves the file pointer for the stream to a specified location, */
/* returns 0 on success or -1 on failure */
int fseek(FILE* stream, long offset, int whence)
{
    uefi_printf("fseek\n");
    return -1;
}

/* Returns the current position of the file pointer in the stream, */
/* or -1L on failure */
long ftell(FILE* stream)
{
    uefi_printf("ftell\n");
    return -1L;
}


/* Reads data from the stream into ptr, */
/* returns the number of items read or 0 on failure */
size_t fread(void* ptr, size_t size, size_t count, FILE* stream)
{
    size_t ret = 0; /* Number of items successfully read */
    EFI_FILE_HANDLE* fPtr = NULL;
    uint64_t fSize = 0;

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

    /* Attempt to read from the file */
    uefi_printf_debug("Before Wrapper\n");
    uefi_printf_debug("Size is %d\n", fSize);
    uefi_call_wrapper((*fPtr)->Read, 3, *fPtr, &fSize, ptr);
    uefi_printf_debug("After Wrapper\n");

    uefi_printf_debug("Leaving custom fread\n");
    return fSize;
}




/* Writes data from ptr to the stream, */
/* returns the number of items written or 0 on failure */
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream)
{
    size_t ret = 0; /* Number of items successfully read */
    EFI_FILE_HANDLE* fPtr = NULL;

    uefi_printf_debug("Inside custom fwrite\n");
    if (stream == NULL) {
        uefi_printf("Bad File Pointer Argument\n");
        return 0;
    }
    fPtr = (EFI_FILE_HANDLE*)stream;

    uefi_printf_debug("Before Wrapper\n");
    uefi_call_wrapper((*fPtr)->Write, 3, *fPtr, count, ptr);
    uefi_printf_debug("Inside custom fwrite\n");

    uefi_printf_debug("Leaving custom fwrite");
    return count;
}

/* Opens a directory stream for the directory specified by name, */
/* returns a DIR pointer or NULL on failure */
DIR* opendir(const char* name)
{
    uefi_printf("opendir\n");
    return NULL;
}

/* Reads the next directory entry from dirp, */
/* returns a pointer to a dirent structure or NULL on failure */
struct dirent* readdir(DIR* dirp)
{
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


