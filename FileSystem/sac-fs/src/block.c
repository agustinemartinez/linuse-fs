#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "disk.h"
#include "block.h"

t_block* read_block(int file_descriptor, u_int32_t block_num) {
    t_block** blocks = read_blocks(file_descriptor, block_num, block_num);
    t_block* block = *blocks;
    free(blocks);
    return block;
}

t_block **read_blocks(int file_descriptor, u_int32_t from_block, u_int32_t to_block) {
    u_int32_t diff = to_block - from_block + 1;
    t_block** blocks = (t_block**) malloc(sizeof(t_block)*diff + 1);
    for ( int i=0 ; i < diff ; i++ ) {
        int block_num = from_block + i;
        blocks[i] = (t_block*) malloc(sizeof(t_block));
        blocks[i]->bytes = (char*) _map_block(file_descriptor, block_num);
    }
    blocks[diff] = NULL;
    return blocks;
}

void* _map_block(int file_descriptor, int block_num) {
    u_int32_t offset = block_num * BLOCK_SIZE;
    return mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, file_descriptor, offset);
}

void _sync_block(t_block *block) {
    msync((void*)block->bytes, BLOCK_SIZE, MS_SYNC);
}

void _unmap_and_destroy_block(t_block *block) {
    munmap((void*)block->bytes, BLOCK_SIZE);
    free(block);
}

void _unmap_and_destroy_blocks(t_block **blocks) {
    for (int i = 0 ; blocks[i] != NULL ; i++)
        _unmap_and_destroy_block(blocks[i]);
    free(blocks);
}
