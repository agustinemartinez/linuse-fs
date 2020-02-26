#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <commons/string.h>
#include "log.h"

#define PATH_LOG "logs/sac.log"
#define PROGRAM_NAME "SAC-SERVER"

void init_logger(void) {
    struct stat st = {0};
    if (stat("logs", &st) == -1) mkdir("logs", 0700);
    g_logger = log_create(PATH_LOG, PROGRAM_NAME, true, LOG_LEVEL_TRACE);
}

void finish_logger(void) {
    log_destroy(g_logger);
}

void log_socket_connected(int socket) {
    log_trace(g_logger, "Se conecto el socket %d", socket);
}

void log_socket_disconnected(int socket) {
    log_trace(g_logger, "Se desconecto el socket %d", socket);
}

void log_server_listenning(char* listen_port, int use_linux_fs) {
    if (use_linux_fs)
        log_trace(g_logger, "Se escucha en el puerto %s y se utiliza LINUX FS", listen_port);
    else
        log_trace(g_logger, "Se escucha en el puerto %s y se utiliza SAC FS"  , listen_port);
}

void log_comunication_error(int socket) {
    log_trace(g_logger, "Error en la comunicacion con el socket %d", socket);
}

void log_function_call(t_function function) {
    switch (function.function_id) {
        case IDENTIFIER_OPEN:
            log_trace(g_logger, "Llamada a open(%s, %d)" , function.args[0], *((size_mode*)function.args[1]));
            break;
        case IDENTIFIER_CLOSE:
            log_trace(g_logger, "Llamada a close(%u)", *((size_fd *)function.args[0]));
            break;
        case IDENTIFIER_WRITE:
            log_trace(g_logger, "Llamada a write(%u, %u, %u)", *((size_fd*)function.args[0]),
                      *((size_buffer*)function.args[2]), *((size_offset*)function.args[3]));
            break;
        case IDENTIFIER_READ:
            log_trace(g_logger, "Llamada a read(%u, %u, %u)", *((size_fd*)function.args[0]),
                      *((size_buffer*)function.args[1]), *((size_offset*)function.args[2]));
            break;
        case IDENTIFIER_REMOVE:
            log_trace(g_logger, "Llamada a remove(%s)"    , function.args[0]);
            break;
        case IDENTIFIER_MKDIR:
            log_trace(g_logger, "Llamada a mkdir(%s)"     , function.args[0]);
            break;
        case IDENTIFIER_RMDIR:
            log_trace(g_logger, "Llamada a rmdir(%s)"     , function.args[0]);
            break;
        case IDENTIFIER_STAT:
            log_trace(g_logger, "Llamada a stat(%s)"      , function.args[0]);
            break;
        case IDENTIFIER_READDIR:
            log_trace(g_logger, "Llamada a readdir(%s)"   , function.args[0]);
            break;
        case IDENTIFIER_RENAME:
            log_trace(g_logger, "Llamada a rename(%s, %s)", function.args[0], function.args[1]);
            break;
        case IDENTIFIER_TRUNCATE:
            log_trace(g_logger, "Llamada a truncate(%s, %u)", function.args[0], *((size_buffer*)function.args[1]));
            break;
        default:
            log_trace(g_logger, "No se reconocio la funcion recibida");
            break;
    }
}
