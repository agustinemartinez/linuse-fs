#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/string.h>
#include "server.h"
#include "serialization.h"

void send_package(int socket, t_package* package) {
    send(socket, &(package->message_id), sizeof(package->message_id), MSG_NOSIGNAL);
    send(socket, &(package->size)      , sizeof(package->size)      , MSG_NOSIGNAL);
    if (package->size > 0) {
        send(socket, package->data, package->size, MSG_NOSIGNAL);
    }
}

t_package* receive_package(int client_socket) {
    t_package* package = malloc(sizeof(t_package));
    recv(client_socket, &(package->message_id), sizeof(package->message_id), MSG_WAITALL);
    recv(client_socket, &(package->size)      , sizeof(package->size)      , MSG_WAITALL);
    package->data = malloc(package->size);
    if (package->size > 0) {
        recv(client_socket, package->data     , package->size              , MSG_WAITALL);
    }
    return package;
}

void send_handshake(int socket) {
    t_package handshake = {
            .message_id = IDENTIFIER_SAC,
            .size = 0,
            .data = malloc(handshake.size)
    };
    send_package(socket, &handshake);
}

t_package* serialize_open(char* path, int mode) {
    size_mode smode = mode;
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_OPEN;
    package->size = _string_serialized_size(path) + sizeof(smode);
    package->data = malloc(package->size);
    _serialize_string(path, package->data);
    memcpy(package->data + _string_serialized_size(path), &smode, sizeof(smode));
    return package;
}

t_package* serialize_close(int fd) {
    size_fd sfd = fd;
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_CLOSE;
    package->size = sizeof(sfd);
    package->data = malloc(package->size);
    memcpy(package->data, &sfd, sizeof(sfd));
    return package;
}

t_package* serialize_write(int fd, void *buffer, size_t count, off_t offset) {
    size_fd sfd = fd;
    size_buffer scount = count;
    size_offset soffset = offset;
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_WRITE;
    package->size = sizeof(sfd) + sizeof(scount) + scount + sizeof(soffset);
    package->data = malloc(package->size);
    memcpy(package->data, &sfd, sizeof(sfd));
    memcpy(package->data + sizeof(sfd), &count, sizeof(scount));
    memcpy(package->data + sizeof(sfd) + sizeof(count), buffer, scount);
    memcpy(package->data + sizeof(sfd) + sizeof(count) + count, &soffset, sizeof(soffset));
    return package;
}

t_package* serialize_read(int fd, void *buffer, size_t count, off_t offset) {
    // Ignoro el buffer porque los datos viajan x socket
    size_fd sfd = fd;
    size_buffer scount = count;
    size_offset soffset = offset;
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_READ;
    package->size = sizeof(sfd) + sizeof(scount) + sizeof(soffset);
    package->data = malloc(package->size);
    memcpy(package->data, &sfd, sizeof(sfd));
    memcpy(package->data + sizeof(sfd), &scount, sizeof(scount));
    memcpy(package->data + sizeof(sfd) + sizeof(scount), &soffset, sizeof(soffset));
    return package;
}

t_package* serialize_remove(char* path) {
    return _serialize_path(IDENTIFIER_REMOVE, path);
}

t_package* serialize_mkdir(char *path, mode_t mode) {
    size_mode smode = mode;
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_MKDIR;
    package->size = _string_serialized_size(path) + sizeof(smode);
    package->data = malloc(package->size);
    _serialize_string(path, package->data);
    memcpy(package->data + _string_serialized_size(path), &smode, sizeof(smode));
    return package;
}

t_package* serialize_rmdir(char *path) {
    return _serialize_path(IDENTIFIER_RMDIR, path);
}

t_package* serialize_stat(char *path, struct stat *stbuf) {
    return _serialize_path(IDENTIFIER_STAT, path);
}

t_package* serialize_readdir(char *path) {
    return _serialize_path(IDENTIFIER_READDIR, path);
}

t_package* serialize_rename(char *old_path, char *new_path) {
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_RENAME;
    package->size = _string_serialized_size(old_path) + _string_serialized_size(new_path);
    package->data = malloc(package->size);
    _serialize_string(old_path, package->data);
    _serialize_string(new_path, package->data + _string_serialized_size(old_path));
    return package;
}

t_package* serialize_truncate(char* path, off_t size) {
    t_package* package = malloc(sizeof(t_package));
    package->message_id = IDENTIFIER_TRUNCATE;
    package->size = _string_serialized_size(path) + sizeof(size);
    package->data = malloc(package->size);
    _serialize_string(path, package->data);
    memcpy(package->data + _string_serialized_size(path), &size, sizeof(size));
    return package;
}

t_function* deserialize(t_package* package) {
    switch (package->message_id) {
        case IDENTIFIER_OPEN:
            return deserialize_open(package);
        case IDENTIFIER_CLOSE:
            return deserialize_close(package);
        case IDENTIFIER_WRITE:
            return deserialize_write(package);
        case IDENTIFIER_READ:
            return deserialize_read(package);
        case IDENTIFIER_REMOVE:
            return deserialize_remove(package);
        case IDENTIFIER_MKDIR:
            return deserialize_mkdir(package);
        case IDENTIFIER_RMDIR:
            return deserialize_rmdir(package);
        case IDENTIFIER_STAT:
            return deserialize_stat(package);
        case IDENTIFIER_READDIR:
            return deserialize_readdir(package);
        case IDENTIFIER_RENAME:
            return deserialize_rename(package);
        case IDENTIFIER_TRUNCATE:
            return deserialize_truncate(package);
        default:
            return NULL;
    }
}

t_function* deserialize_open(t_package* package) {
    // Reservo memoria y cargo el function id y la cantidad de parametros
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 2;
    function->args = malloc( function->count * sizeof(char*) );
    // Leo el tamano del path
    size_string_len length ;//= *((u_int16_t*)package->data);
    memcpy(&length, package->data, sizeof(length));
    // Reservo memoria y cargo el path en el primer argumento
    function->args[0] = malloc(length);
    memcpy(function->args[0], package->data + sizeof(length), length);
    // Cargo el mode (int16_t) en el segundo parametro
    function->args[1] = malloc(sizeof(size_mode));
    memcpy(function->args[1], package->data + sizeof(length) + length, sizeof(size_mode));
    return function;
}

t_function* deserialize_close(t_package* package) {
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 1;
    function->args = malloc( function->count * sizeof(char*) );
    function->args[0] = malloc(package->size);
    memcpy(function->args[0], package->data, package->size);
    return function;
}

t_function* deserialize_write(t_package* package) {
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 4;
    function->args = malloc( function->count * sizeof(char*) );
    function->args[0] = malloc(sizeof(size_fd));
    function->args[2] = malloc(sizeof(size_buffer));
    memcpy(function->args[0], package->data, sizeof(size_fd));
    memcpy(function->args[2], package->data + sizeof(size_fd), sizeof(size_buffer));
    size_buffer scount = *((size_buffer*)function->args[2]);
    function->args[1] = malloc(scount);
    memcpy(function->args[1], package->data + sizeof(size_fd) + sizeof(size_buffer), scount);
    function->args[3] = malloc(sizeof(size_offset));
    memcpy(function->args[3], package->data + sizeof(size_fd) + sizeof(size_buffer) + scount, sizeof(size_offset));
    return function;
}

t_function* deserialize_read(t_package* package) {
    // Ignoro el buffer porque los datos viajan x socket
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 3;
    function->args = malloc( function->count * sizeof(char*) );
    function->args[0] = malloc(sizeof(size_fd));
    function->args[1] = malloc(sizeof(size_buffer));
    function->args[2] = malloc(sizeof(size_offset));
    memcpy(function->args[0], package->data, sizeof(size_fd));
    memcpy(function->args[1], package->data + sizeof(size_fd), sizeof(size_buffer));
    memcpy(function->args[2], package->data + sizeof(size_fd) + sizeof(size_buffer), sizeof(size_offset));
    return function;
}

t_function* deserialize_remove(t_package* package) {
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 1;
    function->args = malloc( function->count * sizeof(char*) );
    size_string_len length;// = *((u_int16_t*)package->data);
    memcpy(&length, package->data, sizeof(length));
    function->args[0] = malloc(length);
    memcpy(function->args[0], package->data + sizeof(length), length);
    return function;
}

t_function* deserialize_mkdir(t_package* package) {
    return deserialize_open(package);
}

t_function* deserialize_rmdir(t_package* package) {
    return deserialize_remove(package);
}

t_function* deserialize_stat(t_package* package) {
    return deserialize_open(package);
}

t_function* deserialize_readdir(t_package* package) {
    return deserialize_remove(package);
}

t_function* deserialize_rename(t_package* package) {
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 2;
    function->args = malloc( function->count * sizeof(char*) );
    size_string_len length1, length2;
    memcpy(&length1, package->data, sizeof(length1));
    function->args[0] = malloc(length1);
    memcpy(function->args[0], package->data + sizeof(length1), length1);
    memcpy(&length2, package->data + sizeof(length1) + length1, sizeof(length2));
    function->args[1] = malloc(length2);
    memcpy(function->args[1], package->data + sizeof(length1) + length1 + sizeof(length2), length2);
    return function;
}

t_function* deserialize_truncate(t_package* package) {
    // Reservo memoria y cargo el function id y la cantidad de parametros
    t_function* function = malloc(sizeof(t_function));
    function->function_id = package->message_id;
    function->count = 2;
    function->args = malloc( function->count * sizeof(char*) );
    // Leo el tamano del path
    size_string_len length ;//= *((u_int16_t*)package->data);
    memcpy(&length, package->data, sizeof(length));
    // Reservo memoria y cargo el path en el primer argumento
    function->args[0] = malloc(length);
    memcpy(function->args[0], package->data + sizeof(length), length);
    // Cargo el mode (int16_t) en el segundo parametro
    function->args[1] = malloc(sizeof(off_t));
    memcpy(function->args[1], package->data + sizeof(length) + length, sizeof(off_t));
    return function;
}

size_string_len _string_serialized_size(char* string) {
    return sizeof(size_string_len) + strlen(string) + 1;
}

void _serialize_string(char* string, char* buffer) {
    size_string_len length = strlen(string) + 1;
    memcpy(buffer, &(length), sizeof(length));
    memcpy(buffer + sizeof(length), string, length);
}

t_package* _serialize_path(t_identifier_message message_id, char* path) {
    t_package* package = malloc(sizeof(t_package));
    package->message_id = message_id;
    package->size = _string_serialized_size(path);
    package->data = malloc(package->size);
    _serialize_string(path, package->data);
    return package;
}

void destroy_function(t_function** function) {
    for (int i=0; i < (*function)->count ; i++)
        free( (*function)->args[i] );
    free((*function)->args);
    free(*function);
}

void destroy_package(t_package** package) {
    free( (*package)->data );
    free(*package);
}
