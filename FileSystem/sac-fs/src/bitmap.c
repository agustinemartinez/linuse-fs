#include <stdlib.h>
#include <stdio.h>
#include "disk.h"
#include "bitmap.h"

void init_bitmap(char* bitarray) {
    bitmap = bitarray_create_with_mode(bitarray, TOTAL_BLOCKS/8, MSB_FIRST);
}

// Reserva el siguiente nro de bloque para nodos libre (dentro del total de bloques designados para nodos)
u_int32_t get_free_node_block(void) {
    return _get_free_block(NODE_TABLE_INITIAL_BLOCK, NODE_TABLE_INITIAL_BLOCK + NODE_TABLE_BLOCKS);
}

// Reserva el siguiente nro de bloque de datos libre (dentro del total de bloques de datos disponibles)
u_int32_t get_free_data_block(void) {
    return _get_free_block(NODE_TABLE_INITIAL_BLOCK + NODE_TABLE_BLOCKS, TOTAL_BLOCKS);
}

bool block_is_free(u_int32_t block_number) {
    return !bitarray_test_bit(bitmap, block_number);
}

// Marca como libre el nro de bloque indicado
void free_block(u_int32_t block_number) {
    bitarray_clean_bit(bitmap, block_number);
}

u_int32_t _get_free_block(u_int32_t from_block_num, u_int32_t to_block_num) {
    u_int32_t bit_index;
    for ( bit_index = from_block_num ; !block_is_free(bit_index) ; bit_index++ );
    if ( bit_index >= to_block_num ) {
        perror("No hay bloques disponibles");
        exit(-1);
    }
    bitarray_set_bit(bitmap, bit_index);
    return bit_index;
}
