#ifndef MULTIROM_H
#define MULTIROM_H

#include "roots.h"

#define MULTIROM_BACKUPS_MAX 128

void multirom_deactivate_backup(unsigned char copy);
void multirom_activate_backup(char *path, unsigned char copy);
char *multirom_list_backups();
char multirom_exract_ramdisk();
char multirom_copy_folder(char *folder);
void multirom_change_mountpoints(char apply);
void __multirom_change_mountpoints_helper(RootInfo *info, char *folder, char apply);
char multirom_backup_boot_image(char restore);
int multirom_prepare_zip_file(char *file);
void multirom_flash_zip(char *file, char newRom);

// From mkbootimg/bootimg.h
typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

struct boot_img_hdr
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;  /* size in bytes */
    unsigned kernel_addr;  /* physical load addr */

    unsigned ramdisk_size; /* size in bytes */
    unsigned ramdisk_addr; /* physical load addr */

    unsigned second_size;  /* size in bytes */
    unsigned second_addr;  /* physical load addr */

    unsigned tags_addr;    /* physical addr for kernel tags */
    unsigned page_size;    /* flash page size we assume */
    unsigned unused[2];    /* future expansion: should be 0 */

    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

/*
** +-----------------+
** | boot header     | 1 page
** +-----------------+
** | kernel          | n pages
** +-----------------+
** | ramdisk         | m pages
** +-----------------+
** | second stage    | o pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
*/

#if 0
typedef struct ptentry ptentry;

struct ptentry {
    char name[16];      /* asciiz partition name    */
    unsigned start;     /* starting block number    */
    unsigned length;    /* length in blocks         */
    unsigned flags;     /* set to zero              */
};

/* MSM Partition Table ATAG
**
** length: 2 + 7 * n
** atag:   0x4d534d70
**         <ptentry> x n
*/
#endif

#endif