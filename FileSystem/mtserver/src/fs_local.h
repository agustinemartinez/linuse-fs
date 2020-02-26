#ifndef FILESYSTEM_FS_LOCAL_H
#define FILESYSTEM_FS_LOCAL_H

#include <sys/types.h>
#include "serialization.h"

#define BASE_PATH "/home/utnso/sac-root"

t_package* _local_open(char* path, mode_t mode);
t_package* _local_close(int fd);
t_package* _local_remove(char* path);

t_package* _local_write(int fd, void *buffer, size_t bytes_to_write, off_t offset);
t_package* _local_read(int fd, void *buffer, size_t bytes_to_read, off_t offset);

t_package* _local_mkdir(char *path, mode_t mode);
t_package* _local_rmdir(char *path);

t_package* _local_stat(char *path, struct stat *buf);
t_package* _local_readdir(char *path);
t_package* _local_rename(char *old_path, char *new_path);
t_package* _local_truncate(const char* path, off_t size);

char* _real_file_path(char* path_to_add);

void __fill_buffer_from_stat(struct stat *stbuf, void* buffer, size_string_len* buf_size);
t_package* create_default_package(int64_t message_id);

#endif //FILESYSTEM_FS_LOCAL_H
