#ifndef FILESYSTEM_SERIALIZATION_H
#define FILESYSTEM_SERIALIZATION_H

#include <sys/types.h>
#include <sys/stat.h>

typedef enum {
    IDENTIFIER_SAC,
    IDENTIFIER_OPEN,
    IDENTIFIER_CLOSE,
    IDENTIFIER_WRITE,
    IDENTIFIER_READ,
    IDENTIFIER_REMOVE,
    IDENTIFIER_MKDIR,
    IDENTIFIER_RMDIR,
    IDENTIFIER_STAT,
    IDENTIFIER_READDIR,
    IDENTIFIER_RENAME,
    IDENTIFIER_TRUNCATE
} t_identifier_message;

typedef struct {
    int64_t message_id;
    u_int32_t size;
    void * data;
} t_package;

typedef struct {
    u_int8_t function_id;
    void** args;
    u_int8_t count; // Cantidad de argumentos
} t_function;

typedef u_int16_t size_fd;         // file_descriptor: close()
typedef int16_t   size_mode;       // mode_t: open()
typedef u_int32_t size_buffer;     // count:  write(), read()
typedef u_int16_t size_string_len; // length: paths
typedef u_int32_t size_offset;     // offset: write(), read()


void       send_package   (int socket, t_package* package);
t_package* receive_package(int client_socket);
void       send_handshake (int socket);

t_package* serialize_open   (char* path, int mode);
t_package* serialize_close  (int fd);
t_package* serialize_write  (int fd, void *buffer, size_t count, off_t offset);
t_package* serialize_read   (int fd, void *buffer, size_t count, off_t offset);
t_package* serialize_remove (char* path);
t_package* serialize_mkdir  (char *path, mode_t mode);
t_package* serialize_rmdir  (char *path);
t_package* serialize_stat   (char *path, struct stat *stbuf);
t_package* serialize_readdir(char *path);
t_package* serialize_rename (char *old_path, char *new_path);
t_package* serialize_truncate(char* path, off_t size);

t_function* deserialize        (t_package* package);
t_function* deserialize_open   (t_package* package);
t_function* deserialize_close  (t_package* package);
t_function* deserialize_write  (t_package* package);
t_function* deserialize_read   (t_package* package);
t_function* deserialize_remove (t_package* package);
t_function* deserialize_mkdir  (t_package* package);
t_function* deserialize_rmdir  (t_package* package);
t_function* deserialize_stat   (t_package* package);
t_function* deserialize_readdir(t_package* package);
t_function* deserialize_rename (t_package* package);
t_function* deserialize_truncate(t_package* package);

size_string_len  _string_serialized_size(char* string);
void             _serialize_string(char* string, char* buffer);
t_package*       _serialize_path(t_identifier_message message_id, char* path);

void destroy_function(t_function** function);
void destroy_package(t_package** package);

#endif //FILESYSTEM_SERIALIZATION_H
