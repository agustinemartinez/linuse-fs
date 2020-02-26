#ifndef FILESYSTEM_FS_SAC_H
#define FILESYSTEM_FS_SAC_H

#include <stddef.h>
#include "serialization.h"

t_package* _sac_open   (const char* path, int mode);
t_package* _sac_close  (int fd);
t_package* _sac_write  (int fd, const void *buffer, size_t count, off_t offset);
t_package* _sac_read   (int fd, void *buffer, size_t count, off_t offset);
t_package* _sac_remove (char* path);
t_package* _sac_mkdir  (const char *path, mode_t mode);
t_package* _sac_rmdir  (const char *path);
t_package* _sac_stat   (char *path, struct stat *buf);
t_package* _sac_readdir(char *path);
t_package* _sac_rename (char *old_path, char *new_path);
t_package* _sac_truncate(const char* path, off_t size);


#endif //FILESYSTEM_FS_SAC_H
