#ifndef FILESYSTEM_LOG_H_
#define FILESYSTEM_LOG_H_

#include <commons/log.h>

t_log* g_logger;

void init_logger(void);
void finish_logger(void);
void log_init_file_system(void);
void log_connection_info(char* ip, char* port);

#endif /* FILESYSTEM_LOG_H_ */
