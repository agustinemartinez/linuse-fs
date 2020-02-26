#include "client.h"

int start_connection(void) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;		
	hints.ai_socktype = SOCK_STREAM;	

	getaddrinfo(ip, port, &hints, &server_info);

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    printf("Conectado al servidor.\n");
    int connection_ok = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen) == 0;
	freeaddrinfo(server_info);

    return connection_ok ? server_socket : -1;
}

t_package* send_function(u_int8_t function_id, void* arg_data1, void* arg_data2, int64_t arg_int1, int64_t arg_int2, int64_t arg_int3) {
    int server_socket = start_connection();
    if (server_socket < 0) {
        printf("Fallo la conexion.\n");
        exit(-1);
    }

    t_package* package;
    send_handshake(server_socket);

    switch (function_id) {
        case IDENTIFIER_OPEN: 
            package = serialize_open(arg_data1, arg_int1); // path, mode
            break;
        case IDENTIFIER_CLOSE:
            package = serialize_close(arg_int1); // fd
            break;
        case IDENTIFIER_WRITE:
            package = serialize_write(arg_int1, arg_data1, arg_int2, arg_int3); // fd, data-para-escribir, size(data-para-escribir), offset
            break;
        case IDENTIFIER_READ:
            package = serialize_read(arg_int1, NULL, arg_int2, arg_int3); // fd, size(data-para-leer), offset
            break;
        case IDENTIFIER_REMOVE:
            package = serialize_remove(arg_data1); // path
            break;
        case IDENTIFIER_MKDIR: 
            package = serialize_mkdir(arg_data1, arg_int1); // path, mode
            break;
        case IDENTIFIER_RMDIR:
            package = serialize_rmdir(arg_data1); // path
            break;
        case IDENTIFIER_STAT:
            package = serialize_stat(arg_data1, NULL); // path
            break;
        case IDENTIFIER_READDIR:
            package = serialize_readdir(arg_data1); // path
            break;
        case IDENTIFIER_RENAME:
            package = serialize_rename(arg_data1, arg_data2); // path
            break;
        case IDENTIFIER_TRUNCATE:
            package = serialize_truncate(arg_data1, arg_int1); // path, length
            break;
        default:
            exit(-1);
    }

    send_package(server_socket, package);
    destroy_package(&package);
    printf("Paquete enviado.\n");
    printf("Esperando respuesta......\n");
    package = receive_package(server_socket);
    close(server_socket);
    return package;
}

/****  MAIN-TEST  ****
 * Para testear el server sin FUSE, descomentar este main y compilar con: make sac-cli

int main(void) {
    ip = LOCAL_IP;
    char* test_file = "/test.txt";
    int open_mode = O_RDWR|O_CREAT;
    char* test_dir = "/test_dir";
    int dir_mode = 777;
    char* esto_se_escribe_en_el_archivo = "Esto-se-escribe-en-el-archivo";
    int size_esto_se_escribe_en_el_archivo = strlen(esto_se_escribe_en_el_archivo) + 1;

    t_package* response = send_function(IDENTIFIER_OPEN, test_file, open_mode, 0);
    int fd = response->message_id;
    response = send_function(IDENTIFIER_WRITE, esto_se_escribe_en_el_archivo, fd, size_esto_se_escribe_en_el_archivo);
    response = send_function(IDENTIFIER_CLOSE, NULL, fd, 0);

    response = send_function(IDENTIFIER_OPEN, test_file, open_mode, 0);
    fd = response->message_id;
    response = send_function(IDENTIFIER_READ, NULL, fd, size_esto_se_escribe_en_el_archivo);
    printf("Info leida: %lli, %s\n", response->message_id, (char*)response->data);
    response = send_function(IDENTIFIER_CLOSE, NULL, fd, 0);
    response = send_function(IDENTIFIER_REMOVE, test_file, 0, 0);

    response = send_function(IDENTIFIER_MKDIR, test_dir, dir_mode, 0);
    response = send_function(IDENTIFIER_RMDIR, test_dir, 0, 0);

    return 0;
}

*/
