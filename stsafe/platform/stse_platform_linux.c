/******************************************************************************
 * \file    stse_platform_linux.c
 * \brief   STSecureElement platform implementation for Linux/Raspberry Pi
 * \author  wolfSSL Inc.
 *
 ******************************************************************************
 * \attention
 *
 * Copyright 2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 ******************************************************************************
 */

#include "core/stse_platform.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

/* I2C bus file descriptor */
static int g_i2c_fd = -1;
static uint8_t g_current_addr = 0;

/* I2C buffer for frame assembly */
#define I2C_BUFFER_SIZE 755U
static PLAT_UI8 I2c_tx_buffer[I2C_BUFFER_SIZE];
static PLAT_UI8 I2c_rx_buffer[I2C_BUFFER_SIZE];
static PLAT_UI16 i2c_tx_frame_size;
static volatile PLAT_UI16 i2c_tx_frame_offset;
static PLAT_UI16 i2c_rx_frame_size;
static volatile PLAT_UI16 i2c_rx_frame_offset;

/* Timeout tracking */
static struct timespec g_timeout_start;
static PLAT_UI16 g_timeout_ms = 0;

/*--------------------- I2C Functions --------------------------- */

stse_ReturnCode_t stse_platform_i2c_init(PLAT_UI8 busID)
{
    char i2c_path[20];

    if (g_i2c_fd >= 0) {
        return STSE_OK; /* Already initialized */
    }

    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", busID);
    g_i2c_fd = open(i2c_path, O_RDWR);

    if (g_i2c_fd < 0) {
        perror("Failed to open I2C bus");
        return STSE_PLATFORM_BUS_ERR;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_wake(PLAT_UI8 busID,
                                         PLAT_UI8 devAddr,
                                         PLAT_UI16 speed)
{
    (void)speed;

    if (g_i2c_fd < 0) {
        stse_ReturnCode_t ret = stse_platform_i2c_init(busID);
        if (ret != STSE_OK) {
            return ret;
        }
    }

    /* Set I2C slave address */
    if (ioctl(g_i2c_fd, I2C_SLAVE, devAddr) < 0) {
        return STSE_PLATFORM_BUS_ERR;
    }
    g_current_addr = devAddr;

    /* Send wake-up byte (0x00) */
    uint8_t wake_byte = 0x00;
    write(g_i2c_fd, &wake_byte, 1);

    /* Wait for device to wake up */
    usleep(5000); /* 5ms */

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_send_start(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI16 FrameLength)
{
    (void)busID;
    (void)devAddr;
    (void)speed;

    if (FrameLength > I2C_BUFFER_SIZE) {
        return STSE_PLATFORM_BUFFER_ERR;
    }

    i2c_tx_frame_size = FrameLength;
    i2c_tx_frame_offset = 0;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_send_continue(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pData,
    PLAT_UI16 data_size)
{
    (void)busID;
    (void)devAddr;
    (void)speed;

    if (data_size != 0) {
        if (pData == NULL) {
            memset((I2c_tx_buffer + i2c_tx_frame_offset), 0x00, data_size);
        } else {
            memcpy((I2c_tx_buffer + i2c_tx_frame_offset), pData, data_size);
        }
        i2c_tx_frame_offset += data_size;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_send_stop(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pElement,
    PLAT_UI16 element_size)
{
    (void)speed;

    if (g_i2c_fd < 0) {
        stse_ReturnCode_t ret = stse_platform_i2c_init(busID);
        if (ret != STSE_OK) {
            return ret;
        }
    }

    /* Add final element if provided */
    if (pElement != NULL && element_size > 0) {
        memcpy((I2c_tx_buffer + i2c_tx_frame_offset), pElement, element_size);
        i2c_tx_frame_offset += element_size;
    }

    /* Set I2C slave address if changed */
    if (g_current_addr != devAddr) {
        if (ioctl(g_i2c_fd, I2C_SLAVE, devAddr) < 0) {
            return STSE_PLATFORM_BUS_ERR;
        }
        g_current_addr = devAddr;
    }

    /* Write the assembled frame */
    ssize_t written = write(g_i2c_fd, I2c_tx_buffer, i2c_tx_frame_offset);
    if (written != (ssize_t)i2c_tx_frame_offset) {
        return STSE_PLATFORM_BUS_ERR;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_send(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pData,
    PLAT_UI16 data_size)
{
    (void)speed;

    if (g_i2c_fd < 0) {
        stse_ReturnCode_t ret = stse_platform_i2c_init(busID);
        if (ret != STSE_OK) {
            return ret;
        }
    }

    /* Set I2C slave address */
    if (g_current_addr != devAddr) {
        if (ioctl(g_i2c_fd, I2C_SLAVE, devAddr) < 0) {
            return STSE_PLATFORM_BUS_ERR;
        }
        g_current_addr = devAddr;
    }

    ssize_t written = write(g_i2c_fd, pData, data_size);
    if (written != (ssize_t)data_size) {
        return STSE_PLATFORM_BUS_ERR;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_receive(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pFrame_header,
    PLAT_UI8 *pFrame_payload,
    PLAT_UI16 *pFrame_payload_Length)
{
    (void)speed;

    if (g_i2c_fd < 0) {
        stse_ReturnCode_t ret = stse_platform_i2c_init(busID);
        if (ret != STSE_OK) {
            return ret;
        }
    }

    /* Set I2C slave address */
    if (g_current_addr != devAddr) {
        if (ioctl(g_i2c_fd, I2C_SLAVE, devAddr) < 0) {
            return STSE_PLATFORM_BUS_ERR;
        }
        g_current_addr = devAddr;
    }

    /* Read header byte first */
    ssize_t rd = read(g_i2c_fd, pFrame_header, 1);
    if (rd != 1) {
        return STSE_PLATFORM_BUS_ERR;
    }

    /* Read length (2 bytes) */
    PLAT_UI8 len_bytes[2];
    rd = read(g_i2c_fd, len_bytes, 2);
    if (rd != 2) {
        return STSE_PLATFORM_BUS_ERR;
    }

    PLAT_UI16 payload_len = ((PLAT_UI16)len_bytes[0] << 8) | len_bytes[1];
    *pFrame_payload_Length = payload_len;

    if (payload_len > 0 && pFrame_payload != NULL) {
        rd = read(g_i2c_fd, pFrame_payload, payload_len);
        if (rd != (ssize_t)payload_len) {
            return STSE_PLATFORM_BUS_ERR;
        }
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_receive_start(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI16 frame_Length)
{
    (void)speed;
    ssize_t rd;

    if (frame_Length > I2C_BUFFER_SIZE) {
        return STSE_PLATFORM_BUFFER_ERR;
    }

    if (g_i2c_fd < 0) {
        stse_ReturnCode_t ret = stse_platform_i2c_init(busID);
        if (ret != STSE_OK) {
            return ret;
        }
    }

    if (g_current_addr != devAddr) {
        if (ioctl(g_i2c_fd, I2C_SLAVE, devAddr) < 0) {
            return STSE_PLATFORM_BUS_ERR;
        }
        g_current_addr = devAddr;
    }

    /* Read the full frame into buffer */
    rd = read(g_i2c_fd, I2c_rx_buffer, frame_Length);
    if (rd != (ssize_t)frame_Length) {
        return STSE_PLATFORM_BUS_ACK_ERROR;
    }

    i2c_rx_frame_size = frame_Length;
    i2c_rx_frame_offset = 0;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_receive_continue(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pElement,
    PLAT_UI16 element_size)
{
    (void)busID;
    (void)devAddr;
    (void)speed;

    if (pElement != NULL && element_size > 0) {
        /* Check for buffer overflow */
        if ((i2c_rx_frame_size - i2c_rx_frame_offset) < element_size) {
            return STSE_PLATFORM_BUFFER_ERR;
        }

        /* Copy from receive buffer */
        memcpy(pElement, (I2c_rx_buffer + i2c_rx_frame_offset), element_size);
    }

    i2c_rx_frame_offset += element_size;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_i2c_receive_stop(
    PLAT_UI8 busID,
    PLAT_UI8 devAddr,
    PLAT_UI16 speed,
    PLAT_UI8 *pElement,
    PLAT_UI16 element_size)
{
    stse_ReturnCode_t ret;

    /* Copy last element */
    ret = stse_platform_i2c_receive_continue(busID, devAddr, speed, pElement, element_size);

    i2c_rx_frame_offset = 0;

    return ret;
}

/*--------------------- Delay/Timeout Functions --------------------------- */

stse_ReturnCode_t stse_platform_delay_init(void)
{
    return STSE_OK;
}

void stse_platform_Delay_ms(PLAT_UI32 delay_val)
{
    usleep(delay_val * 1000);
}

void stse_platform_timeout_ms_start(PLAT_UI16 timeout_val)
{
    clock_gettime(CLOCK_MONOTONIC, &g_timeout_start);
    g_timeout_ms = timeout_val;
}

PLAT_UI8 stse_platform_timeout_ms_get_status(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long elapsed_ms = (now.tv_sec - g_timeout_start.tv_sec) * 1000 +
                      (now.tv_nsec - g_timeout_start.tv_nsec) / 1000000;

    return (elapsed_ms >= g_timeout_ms) ? 1 : 0;
}

/*--------------------- CRC Functions --------------------------- */

/* CRC-16-CCITT table (reflected/LSB-first) - matches STSAFE protocol */
static const PLAT_UI16 crc16_tab[] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static PLAT_UI16 g_crc_val = 0xFFFF;

stse_ReturnCode_t stse_platform_crc16_init(void)
{
    return STSE_OK;
}

PLAT_UI16 stse_platform_Crc16_Calculate(PLAT_UI8 *pbuffer, PLAT_UI16 length)
{
    PLAT_UI16 i;
    g_crc_val = 0xFFFF;

    for (i = 0; i < length; i++) {
        g_crc_val = ((g_crc_val >> 8) ^ crc16_tab[(g_crc_val ^ pbuffer[i]) & 0x00FF]);
    }

    return ~g_crc_val;
}

PLAT_UI16 stse_platform_Crc16_Accumulate(PLAT_UI8 *pbuffer, PLAT_UI16 length)
{
    PLAT_UI16 i;

    for (i = 0; i < length; i++) {
        g_crc_val = ((g_crc_val >> 8) ^ crc16_tab[(g_crc_val ^ pbuffer[i]) & 0x00FF]);
    }

    return ~g_crc_val;
}

/*--------------------- Power Functions --------------------------- */

stse_ReturnCode_t stse_platform_power_init(void)
{
    return STSE_OK;
}

stse_ReturnCode_t stse_platform_power_on(PLAT_UI8 busID, PLAT_UI8 devAddr)
{
    (void)busID;
    (void)devAddr;
    return STSE_OK;
}

stse_ReturnCode_t stse_platform_power_off(PLAT_UI8 busID, PLAT_UI8 devAddr)
{
    (void)busID;
    (void)devAddr;
    return STSE_OK;
}

/*--------------------- Random Functions --------------------------- */

stse_ReturnCode_t stse_platform_generate_random_init(void)
{
    srand((unsigned int)time(NULL));
    return STSE_OK;
}

PLAT_UI32 stse_platform_generate_random(void)
{
    return (PLAT_UI32)rand();
}
