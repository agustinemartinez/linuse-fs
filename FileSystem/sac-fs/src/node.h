#ifndef FILESYSTEM_NODE_H
#define FILESYSTEM_NODE_H

#include <sys/types.h>
#include <stdbool.h>
#include "disk.h"

u_int32_t get_block_num_from_node_num(u_int32_t node_num);
void      read_node(u_int32_t node_num, t_node** node, t_block** node_block);
void      delete_node(u_int32_t node_num);
void      unmap_and_destroy_node(t_node** node, t_block** node_block);
u_int32_t create_pointers_block_in_node(t_node* node, u_int16_t pointers_block_array_pos);
u_int32_t create_data_block_in_pointer_block(t_node* node, u_int16_t pointers_block_array_pos, u_int16_t position);
void      _sync_node(t_node** node, t_block node_block);
void      add_data_block_to_node(t_node*);
t_block*  get_data_block_from_node(t_node*, u_int32_t);
void      printNodeInfo (t_node*);
void printNodeInfoWithNodeNum(u_int32_t nodeNumber);
void deleteLastDataBlock(t_node* unNodo);
u_int32_t getBlockNumberFromNodeDataBlock(t_node* unNodo, u_int32_t numeroBloqueDatos);


#endif //FILESYSTEM_NODE_H
