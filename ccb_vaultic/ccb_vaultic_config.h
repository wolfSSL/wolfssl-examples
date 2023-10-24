/*
 * ccb_vaultic_config.h
 *
 * Copyright (C) 2023 wolfSSL Inc.
 *
  * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef CCB_VAULTIC_CONFIG_H
#define CCB_VAULTIC_CONFIG_H

#include "ccb_vaultic.h"

#ifdef HAVE_CCBVAULTIC

/*
 * Example configuration structures */

/* Configuration choices */
enum {
    _KDF_ID  = 0,
    _USER_ID = 1,
    _MANU_ID = 7,
};

#define _KDF_KEY \
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F" \
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
#define _KDF_LABEL \
    "KDF Label"

#define _USER_MAC \
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
#define _USER_ENC \
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"

#define _MANU_PIN \
    "\x00\x01\x02\x03\x04\x05\x06\x07"

#define _KEY_NAME "/key.pem"
#define _CRT_NAME "/crt.pem"
#define _CA_NAME "/ca.pem"

/* Storage for loaded files */
char gLoadKey_data[CCBVAULTIC_FILE_DATA_LEN_MAX];
char gLoadCrt_data[CCBVAULTIC_FILE_DATA_LEN_MAX];
char gLoadCa_data[CCBVAULTIC_FILE_DATA_LEN_MAX];

/* Filthy preprocessor trick... */
#define QUOTE(...) #__VA_ARGS__

/* From wolfssl/certs/client-key.pem */
char gProvKey_data[] = QUOTE(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAwwPRK/45pDJFO1PIhCsqfHSavaoqUgdH1qY2sgcyjtC6aXvG
w0Se1IFI/S1oootnu6F1yDYsStIb94u6zw357+zxgR57mwNHmr9lzH9lJGmm6BSJ
W+Q098WwFJP1Z3s6enjhAVZWkaYTQo3SPECcTO/Rht83URsMoTv18aNKNeThzpbf
G36/TpfQEOioCDCBryALQxTFdGe0MoJvjYbCiECZNoO6HkByIhfXUmUkc7DO7xnN
rv94bHvAEgPUTnINUG07ozujmV6dyNkMhbPZitlUJttt+qy7/yVMxNF59HHThkAY
E7BjtXJOMMSXhIYtVi/XFfd/wK71/Fvl+6G60wIDAQABAoIBAQCi5thfEHFkCJ4u
bdFtHoXSCrGMR84sUWqgEp5T3pFMHW3qWXvyd6rZxtmKq9jhFuRjJv+1bBNZuOOl
yHIXLgyfb+VZP3ZvSbERwlouFikN3reO3EDVou7gHqH0vpfbhmOWFM2YCWAtMHac
PM3miO5HknkLWgDiXl8RfH35CLcgBokqXf0AqyLh8LO8JKleJg4fAC3+IZpTW23T
K6uUgmhDNtj2L8Yi/LVBXQ0zYOqkfX7oS1WRVtNcV48flBcvqt7pnqj0z4pMjqDk
VnOyz0+GxWk88yQgi1yWDPprEjuaZ8HfxpaypdWSDZsJQmgkEEXUUOQXOUjQNYuU
bRHej8pZAoGBAOokp/lpM+lx3FJ9iCEoL0neunIW6cxHeogNlFeEWBY6gbA/os+m
bB6wBikAj+d3dqzbysfZXps/JpBSrvw4kAAUu7QPWJTnL2p+HE9BIdQxWR9OihqN
p1dsItjl9H4yphDLZKVVA4emJwWMw9e2J7JNujDaR49U0z2LhI2UmFilAoGBANU4
G8OPxZMMRwtvNZLFsI1GyJIYj/WACvfvof6AubUqusoYsF2lB9CTjdicBBzUYo6m
JoEB/86KKmM0NUCqbYDeiSNqV02ebq2TTlaQC22dc4sMric93k7wqsVseGdslFKc
N2dsLe+7r9+mkDzER8+Nlp6YqbSfxaZQ3LPw+3QXAoGAXoMJYr26fKK/QnT1fBzS
ackEDYV+Pj0kEsMYe/Mp818OdmxZdeRBhGmdMvPNIquwNbpKsjzl2Vi2Yk9d3uWe
CspTsiz3nrNrClt5ZexukU6SIPb8/Bbt03YM4ux/smkTa3gOWkZktF63JaBadTpL
78c8Pvf9JrggxJkKmnO+wxkCgYEAukSTFKw0GTtfkWCs97TWgQU2UVM96GXcry7c
YT7Jfbh/h/A7mwOCKTfOck4R1bHBDAegmZFKjX/sec/xObXphexi99p9vGRNIjwO
8tZR9YfYmcARIF0PKf1b4q7ZHNkhVm38hNBf7RAVHBgh58Q9S9fQnmqVzyLJA3ue
42AB/C8CgYAR0EvPG2e5nxB1R4ZlrjHCxjCsWQZQ2Q+1cAb38NPIYnyo2m72IT/T
f1/qiqs/2Spe81HSwjA34y2jdQ0eTSE01VdwXIm/cuxKbmjVzRh0M06MOkWP5pZA
62P5GYY6Ud2JS7Dz+Z9dKJU4vjWrylznk1M0oUVdEzllQkahn831vw==
-----END RSA PRIVATE KEY-----
);

/* From wolfssl/certs/client-cert.pem */
char gProvCrt_data[] = QUOTE(
-----BEGIN CERTIFICATE-----
MIIFHTCCBAWgAwIBAgIUc/tU1gN9TAeE4gARjN2Q3EiN6lMwDQYJKoZIhvcNAQEL
BQAwgZ4xCzAJBgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdC
b3plbWFuMRUwEwYDVQQKDAx3b2xmU1NMXzIwNDgxGTAXBgNVBAsMEFByb2dyYW1t
aW5nLTIwNDgxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJ
ARYQaW5mb0B3b2xmc3NsLmNvbTAeFw0yMjEyMTYyMTE3NDlaFw0yNTA5MTEyMTE3
NDlaMIGeMQswCQYDVQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwH
Qm96ZW1hbjEVMBMGA1UECgwMd29sZlNTTF8yMDQ4MRkwFwYDVQQLDBBQcm9ncmFt
bWluZy0yMDQ4MRgwFgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0B
CQEWEGluZm9Ad29sZnNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQDDA9Er/jmkMkU7U8iEKyp8dJq9qipSB0fWpjayBzKO0Lppe8bDRJ7UgUj9
LWiii2e7oXXINixK0hv3i7rPDfnv7PGBHnubA0eav2XMf2UkaaboFIlb5DT3xbAU
k/Vnezp6eOEBVlaRphNCjdI8QJxM79GG3zdRGwyhO/Xxo0o15OHOlt8bfr9Ol9AQ
6KgIMIGvIAtDFMV0Z7Qygm+NhsKIQJk2g7oeQHIiF9dSZSRzsM7vGc2u/3hse8AS
A9ROcg1QbTujO6OZXp3I2QyFs9mK2VQm2236rLv/JUzE0Xn0cdOGQBgTsGO1ck4w
xJeEhi1WL9cV93/ArvX8W+X7obrTAgMBAAGjggFPMIIBSzAdBgNVHQ4EFgQUM9hF
Ztdohxh+VA1wJ5HHJteFZcAwgd4GA1UdIwSB1jCB04AUM9hFZtdohxh+VA1wJ5HH
JteFZcChgaSkgaEwgZ4xCzAJBgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAw
DgYDVQQHDAdCb3plbWFuMRUwEwYDVQQKDAx3b2xmU1NMXzIwNDgxGTAXBgNVBAsM
EFByb2dyYW1taW5nLTIwNDgxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0G
CSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbYIUc/tU1gN9TAeE4gARjN2Q3EiN
6lMwDAYDVR0TBAUwAwEB/zAcBgNVHREEFTATggtleGFtcGxlLmNvbYcEfwAAATAd
BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDQYJKoZIhvcNAQELBQADggEB
ADbLvMVSmmbNkU2PJ5+zZIAOZLTLGs11noJ8VWfYn5CjNJaZQ/dJU6JYhaCzg0+v
uBWKiB7zYPR8lLVYaPEqE4A0wm+l+H52FoFPNovDWb1R3WCH1x2WRGkHPI8oVrER
XE6BP1cl/WXdB88XCgF+Tj+Oc9v+9PLF/6N2qHRGLkcNsO0KwMUKZdPcYrLgHo69
872vr2aENpLiO4DQV6ZBo2LRpm0UbM2CscHBNVWuWUmoJlK97xssH505BNKCoGs5
cVkzgrpVbJfyG1vgTeLPiecmuCxsn4PW7U4vdakwTgGVDU+DXsivf2fqU7/Kmx/U
/zaXAnGOM97iWCeqcAxb3g4=
-----END CERTIFICATE-----
);

/* From wolfssl/certs/ca-cert.pem */
char gProvCa_data[] = QUOTE(
-----BEGIN CERTIFICATE-----
MIIE/zCCA+egAwIBAgIULIDO20edB2aSPWjXyqyQT8ppQUswDQYJKoZIhvcNAQEL
BQAwgZQxCzAJBgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdC
b3plbWFuMREwDwYDVQQKDAhTYXd0b290aDETMBEGA1UECwwKQ29uc3VsdGluZzEY
MBYGA1UEAwwPd3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdv
bGZzc2wuY29tMB4XDTIyMTIxNjIxMTc0OVoXDTI1MDkxMTIxMTc0OVowgZQxCzAJ
BgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdCb3plbWFuMREw
DwYDVQQKDAhTYXd0b290aDETMBEGA1UECwwKQ29uc3VsdGluZzEYMBYGA1UEAwwP
d3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wuY29t
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvwzKLRSyHoRCW804H0ry
TXUQ8bY1n9/KfQOY06zeA2buKvHYsH1uB1QLEJghTYDLEiDnzE/eRX3Jcncy6sqQ
u2lSEAMvqPOVxfGLYlYb72dvpBBBla0Km+OlwLDScHZQMFuo6AgsfO2nonqNOCkc
rMft8nyVsJWCfUlcOM13Je+9gHVTlDw9ymNbnxW10x0TLxnRPNt2Osy4fcnlwtfa
QG/YIdxzG0ItU5z+Gvx9q3o2P5jehHwFZ85qFDiHqfGMtWjLaH9xICv1oGP1Vi+j
JtK3b7FaF9c4mQj+k1hv/sMTSQgWC6dNZwBSMWcjTpjtUUUduQTZC+zYKLNLve02
eQIDAQABo4IBRTCCAUEwHQYDVR0OBBYEFCeOZxF0wyYdP+0zY7Ok2B0w5ejVMIHU
BgNVHSMEgcwwgcmAFCeOZxF0wyYdP+0zY7Ok2B0w5ejVoYGapIGXMIGUMQswCQYD
VQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwHQm96ZW1hbjERMA8G
A1UECgwIU2F3dG9vdGgxEzARBgNVBAsMCkNvbnN1bHRpbmcxGDAWBgNVBAMMD3d3
dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbYIU
LIDO20edB2aSPWjXyqyQT8ppQUswDAYDVR0TBAUwAwEB/zAcBgNVHREEFTATggtl
eGFtcGxlLmNvbYcEfwAAATAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw
DQYJKoZIhvcNAQELBQADggEBAK6wpDWOihum67OiV886H9xuvNLQpkqPiApudNXR
fNFEsdQ7FwMJWkbtCAjP8f0gB2fAl+w183XKIGGYPvVNvuaddR7kA62Mph497OQa
klv5o62Dyk/Nqji7bq6t+qdG8Ytz7AkjvPIY5beShj6kdWDHPQ8/gwDDBgic0VTW
um2VPTShviSRzCADEVtyHNRl0BGIdSYEJu9mcOY7OIecU3EbCVFwUJlMMQxiRFcw
YAT8EiyjJLT3EdUOtSEL7YYRZ002+legWVUhs23kd17sfvAJE46ZmLLhgrZLPg9B
pgzNSZl+5IrLN+1Tz4ZdqSao5QElWrS8JTXx+lpcztS4miw=
-----END CERTIFICATE-----
);

/*************** Generated structures *******************/

/* Forward declarations, for readability */
const ccbVaultIc_Config gKdfConfig;
const ccbVaultIc_Config gUserConfig;
const ccbVaultIc_Config gManuConfig;

const ccbVaultIc_Provision gProvision;
ccbVaultIc_Load gLoad;

/* KDF Auth */
#define _KDF                                        \
    {                                               \
        .id = _KDF_ID,                              \
        .role = CCBVAULTIC_AUTH_ROLE_APPROVED,      \
        .kind = CCBVAULTIC_AUTH_KIND_KDF,           \
        .auth.kdf =                                 \
        {                                           \
            .key_len = sizeof(_KDF_KEY) - 1,        \
            .key     = _KDF_KEY,                    \
            .label_len = sizeof(_KDF_LABEL) - 1,    \
            .label     = _KDF_LABEL,                \
        }                                           \
    }

/* User Auth */
#define _USER                                       \
    {                                               \
        .id = _USER_ID,                             \
        .role = CCBVAULTIC_AUTH_ROLE_UNAPPROVED,    \
        .kind = CCBVAULTIC_AUTH_KIND_SCP03,         \
        .auth.scp03 =                               \
        {                                           \
            .mac_len = sizeof(_USER_MAC) - 1,       \
            .mac     = _USER_MAC,                   \
            .enc_len = sizeof(_USER_ENC) - 1,       \
            .enc     = _USER_ENC,                   \
        }                                           \
    }

/* Manu Auth */
#define _MANU                                       \
    {                                               \
        .id = _MANU_ID,                             \
        .role = CCBVAULTIC_AUTH_ROLE_MANUFACTURER,  \
        .kind = CCBVAULTIC_AUTH_KIND_PIN,           \
        .auth.pin =                                 \
        {                                           \
            .pin_len = sizeof(_MANU_PIN) - 1,       \
            .pin     = _MANU_PIN,                   \
        }                                           \
    }

const ccbVaultIc_Config gKdfConfig =
    {
        .startup_delay_ms = CCBVAULTIC_FAST_START_MS,
        .timeout_ms       = CCBVAULTIC_APDU_TIMEOUT_MS,
        .spi_rate_khz     = CCBVAULTIC_SPI_RATE_KHZ,
        .auth             = _KDF,
    };

const ccbVaultIc_Config gUserConfig =
    {
        .startup_delay_ms = CCBVAULTIC_FAST_START_MS,
        .timeout_ms       = CCBVAULTIC_APDU_TIMEOUT_MS,
        .spi_rate_khz     = CCBVAULTIC_SPI_RATE_KHZ,
        .auth             = _USER,
    };

const ccbVaultIc_Config gManuConfig =
    {
        .startup_delay_ms = CCBVAULTIC_FAST_START_MS,
        .timeout_ms       = CCBVAULTIC_APDU_TIMEOUT_MS,
        .spi_rate_khz     = CCBVAULTIC_SPI_RATE_KHZ,
        .auth             = _MANU,
    };

ccbVaultIc_File gLoadFiles[] =
    {
        {
            .name_len = sizeof(_KEY_NAME),
            .name     = _KEY_NAME,
            .data_len = sizeof(gLoadKey_data),
            .data     = gLoadKey_data,
        },
        {
            .name_len = sizeof(_CRT_NAME),
            .name     = _CRT_NAME,
            .data_len = sizeof(gLoadCrt_data),
            .data     = gLoadCrt_data,
        },
        {
            .name_len = sizeof(_CA_NAME),
            .name     = _CA_NAME,
            .data_len = sizeof(gLoadCa_data),
            .data     = gLoadCa_data,
        },
    };

ccbVaultIc_Load gLoad =
    {
        .file_count = sizeof(gLoadFiles) / sizeof(gLoadFiles[0]),
        .file = gLoadFiles,
    };

const ccbVaultIc_File gProvFiles[] =
    {
        {
            .name_len = sizeof(_KEY_NAME),
            .name     = _KEY_NAME,
            .data_len = sizeof(gProvKey_data),
            .data     = gProvKey_data,
        },
        {
            .name_len = sizeof(_CRT_NAME),
            .name     = _CRT_NAME,
            .data_len = sizeof(gProvCrt_data),
            .data     = gProvCrt_data,
        },
        {
            .name_len = sizeof(_CA_NAME),
            .name     = _CA_NAME,
            .data_len = sizeof(gProvCa_data),
            .data     = gProvCa_data,
        },
    };

const ccbVaultIc_Provision gProvision =
    {
        .self_test = CCBVAULTIC_PROV_SELFTEST_OPTIONAL,
        .create = _USER,
        .file_count = sizeof(gProvFiles) / sizeof(gProvFiles[0]),
        .file = gProvFiles,
    };

#endif  /* HAVE_CCBVAULTIC */

#endif /* CCB_VAULTIC_CONFIG_H_ */
