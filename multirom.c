#include <time.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>

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
    for (;;) {
        int key = ui_wait_key();
        int visible = ui_text_visible();

        if (key == KEY_BACK) {
            break;
        } else if ((key == KEY_VOLUMEDOWN) && visible) {
            ++selected;
            selected = ui_menu_select(selected);
        } else if ((key == KEY_VOLUMEUP) && visible) {
            --selected;
            selected = ui_menu_select(selected);
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