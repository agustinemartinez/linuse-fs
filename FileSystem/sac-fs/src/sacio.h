#ifndef AUX_SACIO_H
#define AUX_SACIO_H

#include <sys/stat.h>
#include <unistd.h>

/** OPERACIONES */

//  Crear, escribir, leer y borrar archivos.
int sac_open(const char* path, int mode);
int sac_close(int fd);
size_t sac_write(int fd, const void *buffer, size_t count, off_t offset);
size_t sac_read(int fd, void *buffer, size_t count, off_t offset);
int sac_remove(char* path);
int sac_truncate(const char* path, off_t lenght);

//  Crear y listar directorios y sus archivos.
int sac_mkdir(const char *path, mode_t mode);

//  Eliminar directorios.
int sac_rmdir(const char *path);

//  Describir directorios y archivos.
int sac_stat(char* path, struct stat *stbuf);
char** sac_readdir(char* path);
int sac_rename(char* old_path, char* new_path);

#endif //AUX_SACIO_H
