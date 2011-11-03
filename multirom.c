#include <time.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <sys/wait.h>

#include "common.h"
#include "multirom.h"
#include "extracommands.h"
#include "minui/minui.h"

void multirom_deactivate_backup(unsigned char copy)
{
    // Just to make sure it exists
    __system("mkdir /sd-ext/multirom/backup");

    char cmd[512];
    struct tm *loctm = getLocTime();
    sprintf(cmd, "%s /sd-ext/multirom/rom /sd-ext/multirom/backup/rom_%u%02u%02u-%02u%02u && sync", copy ? "cp -r -p" : "mv",
            loctm->tm_year+1900, loctm->tm_mon+1, loctm->tm_mday, loctm->tm_hour, loctm->tm_min);

    if(copy)
    {
        run_script("\nCreate backup?",
                   "\nCreating backup: ",
                   cmd,
                   "\nUnable to create backup\n(%s)\n",
                   "\nOops... something went wrong!\nPlease check the recovery log!\n\n",
                   "\nBackup succesfuly created!\n\n",
                   "\nAborted!\n\n");
    }
    else
    {
        run_script("\nDeactivate rom?",
                   "\nDeactivating : ",
                   cmd,
                   "\nUnable to deactivate\n(%s)\n",
                   "\nOops... something went wrong!\nPlease check the recovery log!\n\n",
                   "\nDeactivation succesfuly\ncompleted!\n\n",
                   "\nAborted!\n\n");
    }
}

void multirom_activate_backup(char *path, unsigned char copy)
{
    char cmd[256];
    sprintf(cmd, "%s %s /sd-ext/multirom/rom && sync", copy ? "cp -r -p" : "mv", path);
    run_script("\nActivate backup?",
               "\nActivating backup: ",
               cmd,
               "\nUnable to activate\n(%s)\n",
               "\nOops... something went wrong!\nPlease check the recovery log!\n\n",
               "\nActivation succesfuly\ncompleted!\n\n",
               "\nAborted!\n\n");
}

void multirom_print_rom_id(char *path)
{
    char str[200];
    sprintf(str, "/sd-ext/multirom/backup/%s/system/build.prop", path);
    FILE *props = fopen(str, "r");
    if(!props)
        return;

    int tmp = 0;
    char *find = NULL;
    int itr;
    strcpy(str,"");
    while(!feof(props))
    {
        itr = 0;
        while(1)
        {
            tmp = fgetc(props);

            if(tmp == EOF || (char)tmp == '\n')
                break;
            if((char)tmp == ' ')
                continue;

            str[itr++] = (char)tmp;
        }
        str[itr] = 0; // null-terminated string
        find = strstr(str, "ro.build.display.id=");
        if(!find)
            continue;
        find += 20; // lenght of "ro.build.display.id="
        fclose(props);
        ui_print("%s\n", find);
        return;
    }
    fclose(props);
}

char *multirom_list_backups()
{
    DIR *dir = opendir("/sd-ext/multirom/backup");
    if(!dir)
    {
        ui_print("Could not open backup dir, no backups present?\n");
        return NULL;
    }

    struct dirent * de = NULL;
    unsigned char total = 0;
    unsigned char *backups[MULTIROM_BACKUPS_MAX];

    while ((de = readdir(dir)) != NULL)
    {
        if (de->d_name[0] == '.')
            continue;
        backups[total] = (char*)malloc(128);
        strcpy(backups[total++], de->d_name);
        if(total >= MULTIROM_BACKUPS_MAX-1)
            break;
    }
    backups[total] = NULL;

    if(total == 0)
    {
        ui_print("No backups found");
        return NULL;
    }
    closedir(dir);

    static char* headers[] = { "Choose backup",
                               "or press BACK to return",
                               "",
                               NULL };
    ui_start_menu(headers, backups);
    int selected = 0;
    int chosen_item = -1;

    ui_reset_progress();
    multirom_print_rom_id(backups[selected]);

    for (;;) {
        int key = ui_wait_key();
        int visible = ui_text_visible();

        if (key == KEY_BACK) {
            break;
        } else if ((key == KEY_VOLUMEDOWN) && visible) {
            ++selected;
            selected = ui_menu_select(selected);
            multirom_print_rom_id(backups[selected]);
        } else if ((key == KEY_VOLUMEUP) && visible) {
            --selected;
            selected = ui_menu_select(selected);
            multirom_print_rom_id(backups[selected]);
        } else if ((key == KEY_MENU) && visible ) {
            chosen_item = selected;
        }

        if (chosen_item >= 0)
        {
            ui_end_menu();
            char *path = (char*)malloc(128);
            ui_print("Selected backup: %s\n", backups[chosen_item]);
            sprintf(path, "/sd-ext/multirom/backup/%s", backups[chosen_item]);

            // Free backup names from memory
            unsigned char i = 0;
            for(; i < total; ++i)
                free(backups[i]);
            return path;
        }
    }
    return NULL;
}

char multirom_exract_ramdisk()
{
    ui_print("Dumping boot img...\n");
    if(__system("dump_image boot /tmp/boot.img") != 0)
    {
        ui_print("Could not dump boot.img!\n");
        return -1;
    }

    FILE *boot_img = fopen("/tmp/boot.img", "r");
    FILE *ramdisk = fopen("/tmp/rd.cpio.gz", "w");
    if(!boot_img || !ramdisk)
    {
        ui_print("Could not open boot.img or ramdisk!\n");
        return -1;
    }

    // load needed ints
    struct boot_img_hdr header;
    unsigned *start = &header.kernel_size;
    fseek(boot_img, BOOT_MAGIC_SIZE, SEEK_SET); 
    fread(start, 4, 8, boot_img);

    // get ramdisk offset
    unsigned int ramdisk_pos = (1 + ((header.kernel_size + header.page_size - 1) / header.page_size))*2048;
    ui_print("Ramdisk addr %u\nRamdisk size %u\n", ramdisk_pos, header.ramdisk_size);

    // get ramdisk!
    char *buffer = (char*) malloc(header.ramdisk_size);
    fseek(boot_img, ramdisk_pos, SEEK_SET);
    fread(buffer, 1, header.ramdisk_size, boot_img);
    fwrite(buffer, 1, header.ramdisk_size, ramdisk);
    fflush(ramdisk);
    fclose(boot_img);
    fclose(ramdisk);
    free(buffer);

    // extact it...
    ui_print("Extracting init files...\n");
    if(__system("mkdir -p /tmp/boot && cd /tmp/boot && gzip -d -c /tmp/rd.cpio.gz | busybox_cpio cpio -i") != 0)
    {
        __system("rm -r /tmp/boot");
        ui_print("Failed to extract boot image!\n");
        return -1;
    }

    // copy our files
    __system("mkdir /sd-ext/multirom/rom/boot");
    __system("cp /tmp/boot/*.rc /sd-ext/multirom/rom/boot/");
    __system("cp /tmp/boot/sbin/adbd /sd-ext/multirom/rom/boot/");
    __system("cp /tmp/boot/default.prop /sd-ext/multirom/rom/boot/");

    FILE *init_f = fopen("/tmp/boot/main_init", "r");
    if(init_f)
    {
        fclose(init_f);
        __system("cp /tmp/boot/main_init /sd-ext/multirom/rom/boot/init");
    }
    else __system("cp /tmp/boot/init /sd-ext/multirom/rom/boot/init");

    __system("rm /sd-ext/multirom/rom/boot/preinit.rc");

    sync();

    // and delete temp files
    __system("rm -r /tmp/boot");
    __system("rm /tmp/boot.img");
    __system("rm /tmp/rd.cpio.gz");
    return 0;
}

char multirom_copy_folder(char *folder)
{
    ui_print("Copying folder /%s... ", folder);

    char cmd[100];
    sprintf(cmd, "mkdir /sd-ext/multirom/rom/%s", folder);
    __system(cmd);

    sprintf(cmd, "cp -r -p /%s/* /sd-ext/multirom/rom/%s/ && sync", folder, folder);
    pid_t pid = fork();
    if (pid == 0)
    {
        char *args[] = { "/sbin/sh", "-c", cmd, "1>&2", NULL };
        execv("/sbin/sh", args);
        _exit(-1);
    }

    int status;
    char state = 0;
    ui_print("-");
    while (waitpid(pid, &status, WNOHANG) == 0)
    {
        switch(state)
        {
            case 0: ui_print("\b-"); break;
            case 1: ui_print("\b\\"); break;
            case 2: ui_print("\b|"); break;
            case 3: ui_print("\b/"); break;
        }
        ++state;
        if(state > 3) state = 0;
        sleep(1);
    }
    ui_print("\b\n");

    if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
    {
        ui_print("Failed to copy /%s!\n", folder);
        return -1;
    }

    return 0;
}

void multirom_change_mountpoints(char apply)
{
    if(apply)
        __system("cp /etc/fstab /etc/fstab.back");
    else
        __system("cp /etc/fstab.back /etc/fstab");

    RootInfo *info = get_root_info_for_path("SYSTEM:");
    if(info)
        __multirom_change_mountpoints_helper(info, "system", apply);

    info = get_root_info_for_path("DATA:");
    if(info)
        __multirom_change_mountpoints_helper(info, "data", apply);
}

void __multirom_change_mountpoints_helper(RootInfo *info, char *folder, char apply)
{
    if(apply)
    {
        char *device = (char*)malloc(100);
        sprintf(device, "/sd-ext/multirom/rom/%s", folder);
        info->device = device;
        info->filesystem = "auto";
        info->filesystem_options = "bind";
        char fstab[100];
         sprintf(fstab, "echo \"/sd-ext/multirom/rom/%s    /%s auto  bind\" >> /etc/fstab", folder, folder);
        __system(fstab);
    }
    else
    {
        free(info->device);
        info->device = g_mtd_device;
        info->filesystem = "yaffs2";
        info->filesystem_options = NULL;
    }
}

char multirom_backup_boot_image(char restore)
{
    if(restore)
    {
        ui_print("Restoring boot.img...\n");
        if(__system("flash_image boot /tmp/boot-backup.img") != 0)
        {
            ui_print("Could not restore boot.img!\n");
            return -1;
        }
    }
    else
    {
        ui_print("Creating backup of boot.img...\n");
        __system("rm /tmp/boot-backup.img");
        if(__system("dump_image boot /tmp/boot-backup.img") != 0)
        {
            ui_print("Could not dump boot.img!\n");
            return -1;
        }
    }
    return 0;
}
