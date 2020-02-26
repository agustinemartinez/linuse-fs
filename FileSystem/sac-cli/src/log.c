#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <commons/string.h>
#include "log.h"

#define PATH_LOG "logs/fuse.log"
#define PROGRAM_NAME "FUSE"

void init_logger(void) {
	struct stat st = {0};
	if (stat("logs", &st) == -1) mkdir("logs", 0700);
	g_logger = log_create(PATH_LOG, PROGRAM_NAME, true, LOG_LEVEL_TRACE);
}

void finish_logger(void) {
	log_destroy(g_logger);
}

void log_init_file_system(void) {
	log_trace(g_logger, "Se inicio el proceso FS"); 
}

void log_connection_info(char* ip, char* port) {
    log_trace(g_logger, "Se conectara a %s : %s", ip, port);
}
