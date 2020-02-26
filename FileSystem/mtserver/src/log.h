#ifndef FILESYSTEM_LOG_H
#define FILESYSTEM_LOG_H

#include <commons/log.h>
#include "serialization.h"

t_log* g_logger;

void init_logger(void);
void finish_logger(void);
void log_server_listenning(char* listen_port, int use_linux_fs);
void log_socket_connected(int socket);
void log_socket_disconnected(int socket);
void log_comunication_error(int socket);
void log_function_call(t_function function);

#endif //FILESYSTEM_LOG_H
