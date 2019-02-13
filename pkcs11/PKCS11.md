# Using PKCS #11 with wolfSSL

## Initializing PKCS #11 library

In order to use a PKCS #11 device it is necessary to load the device specific PKCS #11 shared (or dynamic) library.

The wolfSSL API `wc_Pkcs11_Initialize()` takes the path to the library and initializes a Pkcs11Dev instance for accessing tokens.

```
/**
 * Load library, get function list and initialize PKCS#11.
 *
 * @param  dev     [in]  Device object.
 * @param  library [in]  Library name including path.
 * @param  heap    [in]  Heap hint.
 * @return  BAD_FUNC_ARG when dev or library are NULL pointers.
 *          BAD_PATH_ERROR when dynamic library cannot be opened.
 *          WC_INIT_E when the initialization PKCS#11 fails.
 *          WC_HW_E when unable to get PKCS#11 function list.
 *          0 on success.
 */
int wc_Pkcs11_Initialize(Pkcs11Dev* dev, const char* library, void* heap);
```

## Finalizing PKCS #11 library

When the device is not longer required then Pkcs11Dev instance can be finalized.

This unloads the shared library.

```
/**
 * Close the Pkcs#11 library.
 *
 * @param  dev  [in]  Device object.
 */
void wc_Pkcs11_Finalize(Pkcs11Dev* dev);
```

## Initializing a token

PKCS #11 defines tokens to be in slots. wolfSSL assumes that the token is in a slot and abstracts slots away.

To initialize a token instance using the API `wc_Pkcs11Token_Init()`.

The slot number of the token need not be supplied if the token name is unique. Pass -1 for the slotId to find token by name on any slot.

The userPin must be supplied to login into a session.

```
/**
 * Set up a token for use.
 *
 * @param  token      [in]  Token object.
 * @param  dev        [in]  PKCS#11 device object.
 * @param  slotId     [in]  Slot number of the token.<br>
 *                          Passing -1 uses the first available slot.
 * @param  tokenName  [in]  Name of token to initialize.
 * @param  userPin    [in]  PIN to use to login as user.
 * @param  userPinSz  [in]  Number of bytes in PIN.
 * @return  BAD_FUNC_ARG when token, dev and/or tokenName is NULL.
 *          WC_INIT_E when initializing token fails.
 *          WC_HW_E when another PKCS#11 library call fails.
 *          -1 when no slot available.
 *          0 on success.
 */
int wc_Pkcs11Token_Init(Pkcs11Token* token, Pkcs11Dev* dev, int slotId,
    const char* tokenName, const unsigned char* userPin, int userPinSz);
```

## Finalize token

Finalizing a token will close all session on the token and zeroize any User PIN.

```
/**
 * Finalize token.
 * Closes all sessions on token.
 *
 * @param  token  [in]  Token object.
 */
void wc_Pkcs11Token_Final(Pkcs11Token* token);
```

## Open Session

A session needs to be opened on a token in order for operations to be performed.

If keys need to persist across operations in a session or you need to manage sessions in the application then opening a session using the API `wc_Pkcs11Token_Open()`.

A session can be opened for reading and writing by setting the flag to 1.

```
/**
 * Open a session on the token to be used for all operations.
 *
 * @param  token      [in]  Token object.
 * @param  readWrite  [in]  Boolean indicating to open session for Read/Write.
 * @return  BAD_FUNC_ARG when token is NULL.
 *          WC_HW_E when opening the session fails.
 *          0 on success.
 */
int wc_Pkcs11Token_Open(Pkcs11Token* token, int readWrite);
```

## Close Session

If you opened a session in your application then you should close it too.

Use the API `wc_Pkcs11Token_Close()` to close the session. wolfSSL will create a session to perform any new cryptographic operations.

Any keys in the session will be lost.

```
/**
 * Close the token's session.
 * All object, like keys, will be destoyed.
 *
 * @param  token    [in]  Token object.
 */
void wc_Pkcs11Token_Close(Pkcs11Token* token);
```

## Registering a PKCS #11 device

Cryptographic operations will be performed on a PKCS #11 device when initialized with a device identifier associated with a token.

An application chooses a number that will be the device identifier and associates the PKCS #11 callback and Pkcs11Token pointer using the API `wc_CryptoDev_RegisterDevice()`.

```
e.g.:
    int         ret;
    int         devId = 1;
    Pkcs11Token token;

    ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb, &token);
    if (ret != 0)
        fprintf(stderr, "Failed to register token");
```

## Initializing ECDSA Cryptographic Operation

To initialize ECC signing or verification operation to use the PKCS #11 token, use the API `wc_ecc_init_ex()`.

```
e.g:
    int     ret;
    ecc_key key;
    int     devId = 1;

    ret = wc_ecc_init_ex(&key, NULL, devId);
```

## Using a Private Key

To use an EC private key, load as normal. (Keys can be generated on the device and the private key will not come off.)

Perform the cryptographic operation as normal and the private key will be loaded onto the token in the session if required.


## Performing other PKCS #11 operations

The function list is available as the field `func` in Pkcs11Dev, Pkcs11Token and Pkcs11Session.

The Slot Id is available in Pkcs11Token and Pkcs11Session as `slotId`.

The session handle is available in Pkcs11Token and Pkcs11Session as `handle`.
