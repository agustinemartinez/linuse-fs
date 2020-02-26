#ifndef AUX_BITMAP_H
#define AUX_BITMAP_H

#include <commons/bitarray.h>
#include <sys/types.h>
#include <stdbool.h>

t_bitarray* bitmap;

void       init_bitmap(char* bitarray);
bool       block_is_free(u_int32_t block_number);
void       free_block(u_int32_t block_number);

u_int32_t  get_free_data_block(void);
u_int32_t  get_free_node_block(void);
u_int32_t  _get_free_block(u_int32_t from_block_num, u_int32_t to_block_num);

#endif //AUX_BITMAP_H
