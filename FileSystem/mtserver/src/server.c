#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include "log.h"
#include "fs_local.h"
#include "fs_sac.h"
//#include "utils.h"
#include "server.h"

void server_init(void) {
    init_logger();
    connection_threads = list_create();
    pthread_mutex_init(&mutex_connection_threads, NULL);

    int listenning_socket = init_listenning_socket();
    log_server_listenning(LISTEN_PORT, USE_LINUX_FS);
    listen(listenning_socket, BACKLOG);
    // listen() es una syscall BLOQUEANTE. El sistema esperara hasta que reciba una conexion entrante.

    int socket_cliente;

    //for (int i = 0; i < 100 ; i++) {
    while(1) {
        if ( list_size(connection_threads) < MAX_THREADS && (socket_cliente = accept_connection(listenning_socket)) >= 0 ) {
            pthread_t thread_id;
            if( pthread_create(&thread_id, NULL, (void*)attend_connection, (void*)socket_cliente) < 0 ) {
                close(socket_cliente);
                perror("Could not create thread.\n");
                break;
            }
            pthread_mutex_lock(&mutex_connection_threads);
            list_add(connection_threads, (void*)thread_id);
            pthread_mutex_unlock(&mutex_connection_threads);
        }
    }

    finish_server(listenning_socket);
    finish_logger();
}

int init_listenning_socket(void) {
    struct addrinfo hints;
    struct addrinfo *serverInfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family	  = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
    hints.ai_flags    = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
    hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

    getaddrinfo(NULL, LISTEN_PORT, &hints, &serverInfo);

    int listenning_socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

    bind(listenning_socket, serverInfo->ai_addr, serverInfo->ai_addrlen);
    freeaddrinfo(serverInfo);

    return listenning_socket;
}

int accept_connection(int listenning_socket) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    // accept() es una syscall BLOQUEANTE. El sistema espera si no hay conexiones pendientes.
    return accept(listenning_socket, (struct sockaddr *) &addr, &addrlen);
}

void attend_connection(int socket_cliente) {
    t_package* package = receive_package(socket_cliente);

    if (package->message_id == IDENTIFIER_SAC) { // HANDSHAKE
        destroy_package(&package);
        log_socket_connected(socket_cliente);

        // Recibo el paquete con el mensaje y lo deserializo
        package = receive_package(socket_cliente);
        t_function* funcion = deserialize(package);
        destroy_package(&package);
        log_function_call(*funcion);

        // Ejecuto la funcion con los paramtros especificados y envio la respuesta
        package = execute(funcion);
        send_package(socket_cliente, package);
        destroy_package(&package);
        destroy_function(&funcion);
    }
    else {
        log_comunication_error(socket_cliente);
    }

    close(socket_cliente);
    log_socket_disconnected(socket_cliente);
    remove_thread_from_actual_threads( pthread_self() );
}

t_package* execute(t_function* function) {
    t_package* response;
    switch (function->function_id) {
        case IDENTIFIER_OPEN:
            if (USE_LINUX_FS) response = _local_open(function->args[0], *((size_mode*)function->args[1]));
            else response = _sac_open(function->args[0], *((size_mode*)function->args[1]));
            break;
        case IDENTIFIER_CLOSE:
            if (USE_LINUX_FS) response = _local_close(*((size_fd *)function->args[0]));
            else response = _sac_close(*((size_fd *)function->args[0]));
            break;
        case IDENTIFIER_WRITE:
            if (USE_LINUX_FS) response = _local_write(*((size_fd*)function->args[0]), function->args[1],
                    *((size_buffer*)function->args[2]), *((size_offset*)function->args[3]));
            else response = _sac_write(*((size_fd*)function->args[0]), function->args[1],
                    *((size_buffer*)function->args[2]), *((size_offset*)function->args[3]));
            break;
        case IDENTIFIER_READ:
            if (USE_LINUX_FS) response = _local_read(*((size_fd*)function->args[0]), NULL,
                    *((size_buffer*)function->args[1]), *((size_offset*)function->args[2]));
            else response = _sac_read(*((size_fd*)function->args[0]), NULL,
                    *((size_buffer*)function->args[1]), *((size_offset*)function->args[2]));
            break;
        case IDENTIFIER_REMOVE:
            if (USE_LINUX_FS) response = _local_remove(function->args[0]);
            else response = _sac_remove(function->args[0]);
            break;
        case IDENTIFIER_MKDIR:
            if (USE_LINUX_FS) response = _local_mkdir(function->args[0], 0);
            else response = _sac_mkdir(function->args[0], 0);
            break;
        case IDENTIFIER_RMDIR:
            if (USE_LINUX_FS) response = _local_rmdir(function->args[0]);
            else {
                printf("Entro a _sac_rmdir\n");
                response = _sac_rmdir(function->args[0]);
            }

            break;
        case IDENTIFIER_STAT:
            if (USE_LINUX_FS) response = _local_stat(function->args[0], NULL);
            else response = _sac_stat(function->args[0], NULL);
            break;
        case IDENTIFIER_READDIR:
            if (USE_LINUX_FS) response = _local_readdir(function->args[0]);
            else response = _sac_readdir(function->args[0]);
            break;
        case IDENTIFIER_RENAME:
            if (USE_LINUX_FS) response = _local_rename(function->args[0], function->args[1]);
            else response = _sac_rename(function->args[0], function->args[1]);
            break;
        case IDENTIFIER_TRUNCATE:
            if (USE_LINUX_FS) response = _local_truncate(function->args[0], *((off_t*)function->args[1]));
            else response = _sac_truncate(function->args[0], *((off_t*)function->args[1]));
            break;
        default:
            response = create_default_package(-1);
            break;
    }
    return response;
}

void finish_server(int listenning_socket) {
    wait_connection_threads(); // Espero a que terminen todos los threads
    close(listenning_socket);
    list_destroy(connection_threads);
    pthread_mutex_destroy(&mutex_connection_threads);
}

void wait_connection_threads(void) {
    pthread_t tid;
    for (int threadNum = 0; threadNum < MAX_THREADS; threadNum++) {
        tid = (pthread_t)list_get(connection_threads, threadNum);
        pthread_join(tid, NULL);
    }
}

void remove_thread_from_actual_threads(pthread_t tidToRemove) {
    pthread_mutex_lock(&mutex_connection_threads);
    for (int threadNum = 0; threadNum < MAX_THREADS; threadNum++) {
        if ((pthread_t)list_get(connection_threads, threadNum) == tidToRemove )
            list_remove(connection_threads, threadNum);
    }
    pthread_mutex_unlock(&mutex_connection_threads);
    pthread_detach(tidToRemove);
}

