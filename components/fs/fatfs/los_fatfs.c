/*----------------------------------------------------------------------------
 * Copyright (c) <2013-2018>, <Huawei Technologies Co., Ltd>
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
#include "los_fatfs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <los_vfs.h>

#if defined (__GNUC__) || defined (__CC_ARM)
#include <sys/fcntl.h>
#endif

#ifdef __GNUC__
#include <sys/unistd.h>
#endif

#include <los_printf.h>
/* Defines ------------------------------------------------------------------*/
/* Typedefs -----------------------------------------------------------------*/

/* Macros -------------------------------------------------------------------*/
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef POINTER_ASSERT
#define POINTER_ASSERT(p) \
    if(p == NULL) \
    { \
        return -1; \
    }
#endif
/* Local variables ----------------------------------------------------------*/
struct disk_mnt disk;

/* Extern variables ---------------------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Public functions ---------------------------------------------------------*/

/**
  * @brief  Links a compatible diskio driver/lun id and increments the number of active
  *         linked drivers.
  * @note   The number of linked drivers (volumes) is up to 10 due to FatFs limits.
  * @param  drv: pointer to the disk IO Driver structure
  * @param  lun : only used for USB Key Disk to add multi-lun management
            else the parameter must be equal to 0
  * @retval Returns -1 in case of failure, otherwise return the drive (0 to volumes).
  */
static int fatfs_link_driver(const struct diskio_drv *drv, uint8_t lun)
{
    int ret = -1;
    int i;

    if(disk.num >= FF_VOLUMES)
        return ret;

    for(i = 0; i < FF_VOLUMES; i++)
    {
        if(disk.dev[i].drv != 0)
            continue;
        
        disk.dev[disk.num].state = 0;
        disk.dev[disk.num].drv = drv;
        disk.dev[disk.num].lun = lun;
        disk.num++;
        return i;
    }
    return ret;
}

/**
  * @brief  Unlinks a diskio driver and decrements the number of active linked
  *         drivers.
  * @param  drive: the disk drive (0 to volumes)
  * @param  lun : not used
  * @retval Returns -1 in case of failure, otherwise return the drive (0 to volumes).
  */
static int fatfs_unlink_driver(uint8_t drive, uint8_t lun)
{
    int ret = -1;

    if(disk.num >= 1 && drive < FF_VOLUMES)
    {
        if(disk.dev[drive].drv != 0)
        {
            disk.dev[drive].state= 0;
            disk.dev[drive].drv = 0;
            disk.dev[drive].lun = 0;
            disk.num--;
            return drive;
        }
    }

    return ret;
}

int fatfs_register(const struct diskio_drv *drv)
{
    return fatfs_link_driver(drv, 0);
}

int fatfs_unregister(uint8_t drive)
{
    return fatfs_unlink_driver(drive, 0);
}

static int fatfs_flags_get (int oflags)
{
    int flags = 0;

    switch (oflags & O_ACCMODE)
    {
    case O_RDONLY:
        flags |= FA_READ;
        break;
    case O_WRONLY:
        flags |= FA_WRITE;
        break;
    case O_RDWR:
        flags |= FA_READ | FA_WRITE;
        break;
    default:
        break;
    }

    if (oflags & O_CREAT)
    {
        flags |= FA_CREATE_ALWAYS;
    }

    if (oflags & O_EXCL)
    {
        flags |= FA_OPEN_ALWAYS;
    }

    if (oflags & O_TRUNC)
    {
        flags |= FA_OPEN_ALWAYS;
    }

    if (oflags & O_APPEND)
    {
        flags |= FA_READ | FA_WRITE | FA_OPEN_APPEND;
    }

    return flags;
}

static int fatfs_op_open (struct file *file, const char *path_in_mp, int flags)
{
    FRESULT res;
    FIL     *fp;

    fp = (FIL *) malloc (sizeof(FIL));
    if (fp == NULL)
    {
        PRINT_ERR ("fail to malloc memory in FATFS, <malloc.c> is needed,"
                   "make sure it is added\n");
        return -1;
    }

    res = f_open (fp, path_in_mp, fatfs_flags_get (flags));
    if(res == FR_OK)
    {
        file->f_data = (void *) fp;
    }
    else
    {
        free(fp);
    }

    return res;
}

static int fatfs_op_close (struct file *file)
{
    FRESULT res;
    FIL     *fp = (FIL *)file->f_data;

    POINTER_ASSERT(fp);
    
    res = f_close(fp);
    if(res == FR_OK)
    {
        free(fp);
        file->f_data = NULL;
    }
    
    return res;
}

static ssize_t fatfs_op_read (struct file *file, char *buff, size_t bytes)
{
    ssize_t size = 0;
    FRESULT res;
    FIL     *fp = (FIL *)file->f_data;

    POINTER_ASSERT(fp);
    res = f_read (fp, buff, bytes, (UINT *)&size);
    if(res != FR_OK || size == 0)
    {
        PRINT_ERR ("failed to read, res=%d\n", res);
        return -1;
    }
    return size;
}

static ssize_t fatfs_op_write (struct file *file, const char *buff, size_t bytes)
{
    ssize_t  size = 0;
    FRESULT  res;
    FIL     *fp = (FIL *)file->f_data;
    
    POINTER_ASSERT(fp);
    res = f_write (fp, buff, bytes, (UINT *)&size);
    if(res != FR_OK || size == 0)
    {
        PRINT_ERR ("failed to write, res=%d\n", res);
        return -1;
    }
    return size;
}

static off_t fatfs_op_lseek (struct file *file, off_t off, int whence)
{
    FIL *fp = (FIL *)file->f_data;
    
    POINTER_ASSERT(fp);
    return f_lseek(fp, off);
}

static int fatfs_op_unlink (struct mount_point *mp, const char *path_in_mp)
{
    return f_unlink(path_in_mp);
}

static int fatfs_op_rename (struct mount_point *mp, const char *path_in_mp_old,
                             const char *path_in_mp_new)
{
    return f_rename(path_in_mp_old, path_in_mp_new);
}

static int fatfs_op_sync (struct file *file)
{
    FIL *fp = (FIL *)file->f_data;
    
    POINTER_ASSERT(fp);
    return f_sync(fp);
}

static int fatfs_op_opendir (struct dir *dir, const char *path)
{
    FRESULT  res;
    DIR     *dp;

    dp = (DIR *) malloc (sizeof (DIR));

    if (dp == NULL)
    {
        PRINT_ERR ("fail to malloc memory in SPIFFS, <malloc.c> is needed,"
                   "make sure it is added\n");
        return -1;
    }
    
    res = f_opendir(dp, path);
    if(res != FR_OK)
    {
        free(dp);
        return res;
    }

    dir->d_data   = dp;
    dir->d_offset = 0;

    return res;
}

static int fatfs_op_readdir (struct dir *dir, struct dirent *dent)
{
    FRESULT  res;
    DIR     *dp = (DIR *) dir->d_data;
    FILINFO  e;
    int     len;

    POINTER_ASSERT(dp);

    res = f_readdir(dp, &e);
    if (res != FR_OK)
    {
        return -1;
    }
    
    len = MIN(sizeof(e.fname), LOS_MAX_FILE_NAME_LEN) - 1;
    strncpy ((char *)dent->name, (const char *) e.fname, len);
    dent->name [len] = '\0';
    dent->size = e.fsize;

    if (e.fattrib == AM_DIR)
    {
        dent->type = VFS_TYPE_DIR;
    }
    else
    {
        dent->type = VFS_TYPE_FILE;
    }

    return FR_OK;

}

static int fatfs_op_closedir (struct dir *dir)
{
    FRESULT  res;
    DIR     *dp = (DIR *) dir->d_data;

    POINTER_ASSERT(dp);
    
    res = f_closedir (dp);
    if(res == FR_OK)
    {
        free (dp);
        dir->d_data = NULL;
    }

    return res;
}

static int fatfs_op_mkdir(struct mount_point *mp, const char *path)
{
    return f_mkdir(path);
}

static struct file_ops fatfs_ops =
{
    fatfs_op_open,
    fatfs_op_close,
    fatfs_op_read,
    fatfs_op_write,
    fatfs_op_lseek,
    NULL,               /* stat not supported for now */
    fatfs_op_unlink,
    fatfs_op_rename,
    NULL,               /* ioctl not supported for now */
    fatfs_op_sync,
    fatfs_op_opendir,
    fatfs_op_readdir,
    fatfs_op_closedir,
    fatfs_op_mkdir      /* fatfs do not support mkdir */
};

static struct file_system fatfs_fs =
{
    "fatfs",
    &fatfs_ops,
    NULL,
    0
};

int fatfs_init (void)
{
    static int fatfs_inited = FALSE;
    
    if (fatfs_inited)
    {
        return LOS_OK;
    }

    if (los_vfs_init () != LOS_OK)
    {
        return LOS_NOK;
    }

    if (los_fs_register (&fatfs_fs) != LOS_OK)
    {
        PRINT_ERR ("failed to register fs!\n");
        return LOS_NOK;
    }

    fatfs_inited = TRUE;

    PRINT_INFO ("register fatfs done!\n");

    return LOS_OK;
}


int fatfs_mount(const char *path, struct diskio_drv *drv, uint8_t *drive)
{
    int s_drive;
    char dpath[10] = {0};
    int ret = -1;
    BYTE *work_buff = NULL;
    FRESULT res;
    FATFS   *fs = NULL;

    s_drive = fatfs_register(drv);
    if(s_drive < 0)
    {
        PRINT_ERR("failed to register diskio!\n");
        return s_drive;
    }
    fs = (FATFS *) malloc (sizeof (FATFS));
    if (fs == NULL)
    {
        PRINT_ERR ("fail to malloc memory in FATFS, <malloc.c> is needed,"
                   "make sure it is added\n");
        goto err;
    }
    memset(fs, 0, sizeof(FATFS));
    sprintf(dpath, "%d:/", s_drive);
    res = f_mount(fs, (const TCHAR *)dpath, 1);
    if(res == FR_NO_FILESYSTEM)
    {
        work_buff = (BYTE *)malloc(FF_MAX_SS);
        if(work_buff == NULL)
            goto err;
        memset(work_buff, 0, FF_MAX_SS);
        res = f_mkfs((const TCHAR *)dpath, FM_ANY, 0, work_buff, FF_MAX_SS);
        if(res == FR_OK)
        {
            res = f_mount(NULL, (const TCHAR *)dpath, 1);
            res = f_mount(fs, (const TCHAR *)dpath, 1);
        }
        free(work_buff);
    }
    if(res != FR_OK)
    {
        PRINT_ERR("failed to mount fatfs, res=%d!\n", res);
        goto err_free;
    }

    ret = los_fs_mount ("fatfs", path, fs);

    if (ret == LOS_OK)
    {
        PRINT_INFO ("fatfs mount at %s done!\n", path);
        *drive = s_drive;
        return LOS_OK;
    }

    PRINT_ERR ("failed to mount!\n");

err_free:
    if(fs != NULL)
        free(fs);
err:
    fatfs_unregister(s_drive);
    return ret; 
}

int fatfs_unmount(const char *path, uint8_t drive)
{
    char dpath[10] = {0};
    
    sprintf(dpath, "%d:/", drive);
    fatfs_unregister(drive);
    f_mount(NULL, (const TCHAR *)dpath, 1);
    los_fs_unmount(path); // need free fs(in fatfs_mount) 
    
    return 0;
}

/* Private functions --------------------------------------------------------*/

