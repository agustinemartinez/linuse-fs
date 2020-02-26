#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <commons/string.h>
#include "fs_local.h"


t_package* _local_open(char* path, mode_t mode) {
    char* local_path = _real_file_path(path);
    int64_t fd = open(local_path, mode, 0777);//S_IRWXU|S_IRWXO|S_IRWXG);
    free(local_path);
    return create_default_package(fd);
}

t_package* _local_close(int fd) {
    return create_default_package( close(fd) );
}

t_package* _local_write(int fd, void *buffer, size_t bytes_to_write, off_t offset) {
    lseek(fd, offset, SEEK_SET);
    return create_default_package( write(fd, buffer, bytes_to_write) );
}

t_package* _local_read(int fd, void *buffer, size_t bytes_to_read, off_t offset) {
    t_package* package = malloc(sizeof(t_package));
    package->data = malloc(bytes_to_read);
    lseek(fd, offset, SEEK_SET);
    package->size = read(fd, package->data, bytes_to_read);
    if (package->size < 0) package->message_id = -1;
    else package->message_id = 0;
    return package;
}

t_package* _local_remove(char* path) {
    char* local_path = _real_file_path(path);
    int64_t status = remove(local_path);
    free(local_path);
    return create_default_package(status);
}

t_package* _local_mkdir(char *path, mode_t mode) {
    char* local_path = _real_file_path(path);
    int64_t status = mkdir(local_path, 0777);
    free(local_path);
    return create_default_package(status);
}

t_package* _local_rmdir(char *path) {
    char* local_path = _real_file_path(path);
    int64_t status = rmdir(local_path);
    free(local_path);
    return create_default_package(status);
}

t_package* _local_stat(char *path, struct stat *buf) {
    char* local_path = _real_file_path(path);
    struct stat stbuf;
    t_package* package = create_default_package( stat(local_path, &stbuf) );

    if (package->message_id >= 0) {
        // TODO: Revisar por que rompe y delegar en la funcion
        //__fill_buffer_from_stat(&stbuf, package->data, &(package->size));
        int16_t is_dir_or_file = S_ISDIR(stbuf.st_mode) ? 2 : 1;
        u_int16_t node_num = stbuf.st_ino;
        u_int32_t fsize    = stbuf.st_size;
        u_int64_t mtime    = stbuf.st_mtim.tv_sec;
        u_int64_t ctime    = stbuf.st_ctim.tv_sec;
        size_buffer size   = sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(fsize) + sizeof(mtime) + sizeof(ctime);
        free(package->data);
        package->data = malloc(size);

        // Directorio = 2, Archivo = 1
        memcpy(package->data, &is_dir_or_file, sizeof(is_dir_or_file));
        memcpy(package->data + sizeof(is_dir_or_file), &node_num, sizeof(node_num));
        memcpy(package->data + sizeof(is_dir_or_file) + sizeof(node_num), &fsize, sizeof(fsize));
        memcpy(package->data + sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(fsize), &mtime, sizeof(mtime));
        memcpy(package->data + sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(fsize) + sizeof(mtime), &ctime, sizeof(ctime));
        package->size = size;
    }

    free(local_path);
    return package;
}

t_package* _local_readdir(char *path) {
    char* local_path = _real_file_path(path);
    t_package* package = create_default_package(0);
    //t_package* package = create_default_package(-1);

    DIR *dir = opendir(local_path);
	struct dirent *dp;

	while ( (dp = readdir(dir)) != NULL ) {
	    //package->message_id = 0; // Si no retorno NULL a la primera, no fallo
	    char* d_name = dp->d_name, *buffer;
        int prev_size = package->size;
        // Me guardo lo que tenia en el package->data en un buffer auxiliar
        buffer = malloc(prev_size);
        memcpy(buffer, package->data, prev_size);
        free(package->data);
        // Guardo en el package->data lo que tenia + la nueva entrada
        package->size += _string_serialized_size(d_name);
        package->data = malloc(package->size);
        memcpy(package->data, buffer, prev_size);
        free(buffer);
	    _serialize_string(d_name, package->data + prev_size);
	}

	closedir(dir);
    free(local_path);
    return package;
}

t_package* _local_rename(char *old_path, char *new_path) {
    char* local_old_path = _real_file_path(old_path);
    char* local_new_path = _real_file_path(new_path);
    int64_t status = rename(local_old_path, local_new_path);
    free(local_old_path);
    free(local_new_path);
    return create_default_package(status);
}

t_package* _local_truncate(const char* path, off_t size) {
    char* local_path = _real_file_path((char*)path);
    int status = truncate(local_path, size);
    free(local_path);
    return create_default_package(status);
}

char* _real_file_path(char* path_to_add) {
    return string_from_format("%s%s", BASE_PATH, path_to_add);
}

void __fill_buffer_from_stat(struct stat *stbuf, void* buffer, size_string_len* buf_size) {
    // is_dir_or_file + nodo + size + date + date
    int16_t is_dir_or_file = S_ISDIR(stbuf->st_mode) ? 2 : 1;
    u_int16_t node_num = stbuf->st_ino;
    u_int32_t fsize    = stbuf->st_size;
    u_int64_t mtime    = stbuf->st_mtim.tv_sec;
    u_int64_t ctime    = stbuf->st_ctim.tv_sec;
    *buf_size          = sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(fsize) + sizeof(mtime) + sizeof(ctime);
    buffer = malloc(*buf_size);

    // Directorio = 2, Archivo = 1
    memcpy(buffer, &is_dir_or_file, sizeof(is_dir_or_file));
    memcpy(buffer + sizeof(is_dir_or_file), &node_num, sizeof(node_num));
    memcpy(buffer + sizeof(is_dir_or_file) + sizeof(node_num), &fsize, sizeof(fsize));
    memcpy(buffer + sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(fsize), &mtime, sizeof(mtime));
    memcpy(buffer + sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(fsize) + sizeof(mtime), &ctime, sizeof(ctime));
}

t_package* create_default_package(int64_t message_id) {
    t_package* package = malloc(sizeof(t_package));
    package->message_id = message_id;
    package->size = 0;
    package->data = malloc(package->size);
    return package;
}
