#ifndef AUX_UTILS_H
#define AUX_UTILS_H

#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include "directory.h"
#include "disk.h"

t_header*   create_default_header(void);
t_bitarray* create_default_bitmap(void);
t_node*     create_root_dir(void);
t_header*   create_header(char* identifier, u_int8_t version, u_int32_t bitmap_initial_block, u_int32_t bitmap_size);
t_node*     create_node(u_int8_t state, char *file_name, u_int32_t parent_block, u_int32_t file_size, u_int64_t creation_date,
                        u_int64_t modification_date, u_int32_t indirect_pointer_block[]);

void        header_to_block(t_header header, t_block* block);
t_header*   block_to_header(t_block block);
void        bitmap_to_block(t_bitarray bitmap, t_block **blocks);
t_bitarray* block_to_bitmap(t_block** blocks);
void        node_to_block(t_node node, t_block *block);
t_node*     block_to_node(t_block block);

int         arr_length(char** arr);
void        arr_free(char** arr);
void        print_disk_info(void);
void        print_bytes_as_binary(char* bytes, int size);
off_t       file_size(const char* path);
bool        file_exists(const char* path);

void        destroy_header(t_header* header);
void        destroy_node(t_node* node);

void        add_entry_to_list(t_list**, t_dir_entry);
void        delete_parent_entry(t_dir_entry);
char*       getFolderPathFromFilePath(char* filePath);
char*       getFileNameFromFilePath(char* filePath);
int         min(int a, int b);

#endif //AUX_UTILS_H
