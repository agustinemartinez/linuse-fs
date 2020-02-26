#ifndef AUX_DISK_H
#define AUX_DISK_H

#include <sys/types.h>
#include <stdbool.h>
#include <commons/bitarray.h>
#include "block.h"

#define SAC_ROOT_PATH "/home/utnso/sac-root"
#define DISK_PATH "/home/utnso/sac-root/disk"

#define IDENTIFIER "SAC"
#define VERSION 1
#define BITMAP_INITIAL_BLOCK 1
#define BLOCK_SIZE 4096

// Estos se pueden modificar porque varian segun el tamano del disco
#define TOTAL_BLOCKS 3000
#define NODE_TABLE_BLOCKS 32
//

#define DISK_SIZE BLOCK_SIZE*TOTAL_BLOCKS // 256KB
#define BITMAP_SIZE DISK_SIZE/(BLOCK_SIZE*8)
#define NODE_TABLE_SIZE NODE_TABLE_BLOCKS*BLOCK_SIZE
#define NODE_TABLE_INITIAL_BLOCK (BITMAP_INITIAL_BLOCK + BITMAP_BLOCKS)

#define BITMAP_BLOCKS (BITMAP_SIZE % BLOCK_SIZE == 0 ? BITMAP_SIZE / BLOCK_SIZE : BITMAP_SIZE / BLOCK_SIZE + 1)
#define DATA_BLOCKS TOTAL_BLOCKS - 1 - BITMAP_BLOCKS - NODE_TABLE_BLOCKS
#define DIR_ENTRY_SIZE 128

typedef u_int32_t ptrGBloque;

typedef struct {
    char identifier[3];
    u_int32_t version;
    u_int32_t bitmap_initial_block;
    u_int32_t bitmap_size;
    char padding[4081];
} t_header;

typedef struct {
    u_int32_t data_block[1024];
} t_indirect_pointer_block; // ptrGBloque

typedef struct {
    u_int8_t state; // 0: Borrado | 1: Ocupado | 2: Directorio
    char file_name[71];
    u_int32_t parent_block;
    u_int32_t file_size;
    u_int64_t creation_date;
    u_int64_t modification_date;
    ptrGBloque indirect_pointer_block[1000];
} t_node;

// File descriptor del archivo del disco
int disk_fd;

// Bitmap del FS
t_bitarray* bitmap;

void        format_disk(void);
void        disk_create(void);
int         disk_open(void);
void        disk_close(int file_descriptor);

#endif //AUX_DISK_H
