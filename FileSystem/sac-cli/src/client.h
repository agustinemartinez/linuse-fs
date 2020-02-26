#ifndef FILESYSTEM_CLIENT_H
#define FILESYSTEM_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
//#include "../../mtserver/src/utils.h"
#include "../../mtserver/src/serialization.h"

#define LOCAL_IP "127.0.0.1"
#define DEFAULT_PORT "21000"

const char* ip;
const char* port;

int start_connection(void);
t_package* send_function(u_int8_t function_id, void* arg_data1, void* arg_data2, int64_t arg_int1, int64_t arg_int2, int64_t arg_int3);

#endif //FILESYSTEM_CLIENT_H
