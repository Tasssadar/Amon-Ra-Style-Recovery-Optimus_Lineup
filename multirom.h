#ifndef MULTIROM_H
#define MULTIROM_H

#define MULTIROM_BACKUPS_MAX 128

void multirom_deactivate_backup(unsigned char copy);
void multirom_activate_backup(char *path, unsigned char copy);
char *multirom_list_backups();

#endif