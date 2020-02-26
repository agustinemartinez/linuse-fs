#ifndef FILESYSTEM_SERVER_H
#define FILESYSTEM_SERVER_H

#include <commons/collections/list.h>
#include "serialization.h"

#define BACKLOG 6			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define MAX_THREADS 6		// Define la cantidad maxima de hilos que se pueden crear simultaneas

#define DEFAULT_USE_LINUX_FS false
#define DEFAULT_PORT "21000"

bool  USE_LINUX_FS;
char* LISTEN_PORT;

t_list* connection_threads;
pthread_mutex_t mutex_connection_threads;

void server_init(void);

void finish_server(int listenning_socket);
int init_listenning_socket(void);
int accept_connection(int listenning_socket);
void attend_connection(int socket_cliente);
void remove_thread_from_actual_threads(pthread_t tidToRemove);
void wait_connection_threads(void);

t_package* execute(t_function* function);

#endif //FILESYSTEM_SERVER_H
