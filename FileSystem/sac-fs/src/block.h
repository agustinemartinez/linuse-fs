#ifndef AUX_BLOCK_H
#define AUX_BLOCK_H

#include <sys/types.h>

typedef struct {
    char* bytes;
} t_block;

t_block* read_block(int file_descriptor, u_int32_t block_num);
t_block** read_blocks(int file_descriptor, u_int32_t from_block, u_int32_t to_block);
void* _map_block(int file_descriptor, int block_num);
void _sync_block(t_block* block);
void _unmap_and_destroy_block(t_block* block);
void _unmap_and_destroy_blocks(t_block **blocks);

#endif //AUX_BLOCK_H
