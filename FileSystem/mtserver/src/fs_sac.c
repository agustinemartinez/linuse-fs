#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../sac-fs/src/sacio.h"
//#include "utils.h"
#include "fs_sac.h"

t_package* _sac_open(const char* path, int mode) {
    int fd = sac_open(path, mode);
    return create_default_package(fd);
}

t_package* _sac_close(int fd) {
    int status = sac_close(fd);
    return create_default_package(status);
}

t_package* _sac_write(int fd, const void *buffer, size_t count, off_t offset) {
    size_t size = sac_write(fd, buffer, count, offset);
    return create_default_package(size);
}

t_package* _sac_read(int fd, void *buffer, size_t count, off_t offset) {
    t_package* package = malloc(sizeof(t_package));
    package->data = malloc(count);
    size_t sizeRead = sac_read(fd, package->data, count, offset);
    package->size = (count > sizeRead) ? sizeRead : count;
    if (package->size < 0) package->message_id = -1;
    else package->message_id = 0;
    return package;
}

t_package* _sac_remove(char* path) {
    int status = sac_remove(path);
    return create_default_package(status);
}

t_package* _sac_mkdir(const char *path, mode_t mode) {
    int status = sac_mkdir(path, mode); //0777
    return create_default_package(status);
}

t_package* _sac_rmdir(const char *path) {
    int status = sac_rmdir(path);
    return create_default_package(status);
}

t_package* _sac_stat(char *path, struct stat *buf) {
    struct stat stbuf;
    t_package* package = create_default_package( sac_stat(path, &stbuf) );
    int idMensaje = package->message_id;
    if (idMensaje >= 0) {
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

    return package;
}

t_package* _sac_readdir(char *path) {
    t_package* package = create_default_package(0);

    char** entries = sac_readdir(path); // Siempre tiene que retornar algo para despues hacer free

    if (entries == NULL) {
        package->message_id = -1;
        return package;
    }

    int prev_size;
    for (int i=0 ; entries[i] != NULL ; i++) {
        char* d_name = entries[i];
        char* buffer;
        // Me guardo lo que tenia en el package->data en un buffer auxiliar
        prev_size = package->size;
        buffer = malloc(prev_size);
        memcpy(buffer, package->data, prev_size);
        free(package->data);
        // Guardo en el package->data lo que tenia + la nueva entrada
        package->size += _string_serialized_size(d_name);
        package->data = malloc(package->size);
        memcpy(package->data, buffer, prev_size);
        free(buffer);
        _serialize_string(d_name, package->data + prev_size);
        free(entries[i]);
    }
    free(entries);
    return package;
}

t_package* _sac_rename(char *old_path, char *new_path) {
    int status = sac_rename(old_path, new_path);
    return create_default_package(status);
}

t_package* _sac_truncate(const char* path, off_t size) {
    int status = sac_truncate(path, size);
    return create_default_package(status);
}
