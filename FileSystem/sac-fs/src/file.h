//
// Created by utnso on 18/11/19.
//

#ifndef FILESYSTEM_FILE_H
#define FILESYSTEM_FILE_H

#include "disk.h"

u_int32_t create_and_save_file_node(char* file_name, u_int32_t folder_node_num);
t_node* create_file_node(char* new_file_name, u_int32_t  folder_node_num);

#endif //FILESYSTEM_FILE_H
