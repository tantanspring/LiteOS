/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

/* Includes -----------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#include <sys/unistd.h>
#include <sys/stat.h>
#endif

#if defined (__GNUC__) || defined (__CC_ARM)
#include <sys/fcntl.h>
#include <los_printf.h>
#endif

#include <los_vfs.h>
#include <los_fatfs.h>

#include "hal_sram.h"
/* Defines ------------------------------------------------------------------*/
/* Block Size in Bytes */
#define BLOCK_SIZE                512

/* Typedefs -----------------------------------------------------------------*/
/* Macros -------------------------------------------------------------------*/
/* Local variables ----------------------------------------------------------*/
static volatile DSTATUS state = STA_NOINIT;

/* Extern variables ---------------------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Public functions ---------------------------------------------------------*/
static DSTATUS stm32f1xx_fatfs_status(BYTE lun)
{
    return state;
}

static DSTATUS stm32f1xx_fatfs_initialize(BYTE lun)
{
    state = STA_NOINIT;

    if(hal_sram_init() == SRAM_OK)
    {
        state &= ~STA_NOINIT;
    }
    return state;
}

static DRESULT stm32f1xx_fatfs_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    uint32_t BufferSize = (BLOCK_SIZE * count); 
    uint8_t *pSramAddress = (uint8_t *) (SRAM_DEVICE_ADDR + (sector * BLOCK_SIZE)); 
    
    for(; BufferSize != 0; BufferSize--)
    {
      *buff++ = *(__IO uint8_t *)pSramAddress++;  
    } 
    
    return RES_OK;
}

static DRESULT stm32f1xx_fatfs_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    uint32_t BufferSize = (BLOCK_SIZE * count) + count; 
    uint8_t *pSramAddress = (uint8_t *) (SRAM_DEVICE_ADDR + (sector * BLOCK_SIZE)); 
    
    for(; BufferSize != 0; BufferSize--)
    {
      *(__IO uint8_t *)pSramAddress++ = *buff++;    
    } 
    
    return RES_OK;
}

static DRESULT stm32f1xx_fatfs_ioctl(BYTE lun, BYTE cmd, void *buff)
{
    DRESULT res = RES_OK;

    switch (cmd)
    {
    case CTRL_SYNC:
        break;
    case GET_SECTOR_COUNT:
        *(DWORD *)buff = SRAM_DEVICE_SIZE / BLOCK_SIZE;
        break;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = BLOCK_SIZE;
        break;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = BLOCK_SIZE;
        break;
    default:
        res = RES_PARERR;
        break;
    }
    return res;
}

static struct diskio_drv sram_drv =
{
    stm32f1xx_fatfs_initialize,
    stm32f1xx_fatfs_status,
    stm32f1xx_fatfs_read,
    stm32f1xx_fatfs_write,
    stm32f1xx_fatfs_ioctl
};

int stm32f1xx_fatfs_init(void)
{
    int8_t drive = -1;
    
    (void)fatfs_init();

    if(fatfs_mount("/fatfs/", &sram_drv, (uint8_t *)&drive) < 0)
    {
        PRINT_ERR ("failed to mount fatfs!\n");
    }
    
    return drive;
}

DWORD get_fattime (void)
{
    return 0;
}

/* Private functions --------------------------------------------------------*/

