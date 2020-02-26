#ifndef FILESYSTEM_DIRECTORY_H
#define FILESYSTEM_DIRECTORY_H

#include "disk.h"
#include <commons/collections/list.h>

typedef struct {
    u_int32_t pointer_block_num; // Indice del vector ptrGBloque. Numero de bloque de punteros dentro del array ptrGBloque del t_node
    u_int32_t block_num; // Numero de bloque dentro del bloque de punteros (pointer_block_num) que apunta al bloque de datos
    u_int32_t byte_num; // Numero de byte dentro del bloque de datos apuntado dentro del bloque de punteros
} t_data_position;

typedef struct {
    char name[71];
    u_int32_t node_num;
    t_data_position data;
} t_dir_entry; // Cada entrada ocupa 128 bytes para que entren exactamente 32 entradas por bloque

t_dir_entry search_path(char * path);
void add_directory_entry(t_node* node, char* name, u_int32_t node_num);

// Eliminia una entrada de directorio
void delete_directory_entry(t_node*, char*);

// Elimina todas las entradas de directorio asociadas a un directorio
void delete_all_directory_entries(u_int32_t node_num);

t_dir_entry get_entry(t_node* node, char* entry_name);
bool exists_entry(t_node* node, char* entry_name);
t_data_position get_next_write_position(t_node* node);
bool is_valid_entry(t_dir_entry entry);
bool is_valid_path(char* path);
void print_entries(t_node* node);


/*** FUNCIONES AUXILIARES ***/


t_node* create_dir_node(char* name, u_int32_t parent_node_num);
void create_and_save_dir_node(char* new_dir_name, u_int32_t pre_dir_node_num, u_int32_t new_block_num);
void create_dir_node_and_add_entry(t_node* pre_dir_node, u_int32_t pre_dir_node_num, char* new_dir_name);

// Borra el nodo de un directorio, y borra la entrada de directorio del nodo padre
void delete_dir_node_and_remove_entry (t_node* pre_dir_node, u_int32_t pre_dir_node_num, char* new_dir_name);
t_block* read_next_data_block(t_node* node, t_data_position position);
t_dir_entry iterate_pointers_blocks(t_node* node, t_dir_entry (*process_entry)(t_dir_entry));

// Recibe una lista, un nodo, y una func. Aplica la func a las entradas del nodo. Caso exito, agrega el nodo a la lista.
void iterate_pointers_add_list(t_list**, t_node*, t_dir_entry (*process_entry)(t_dir_entry));
t_dir_entry iterate_blocks(t_block* pointers_block, bool is_last_pointers_block, t_data_position position, t_dir_entry (*process_entry)(t_dir_entry));
t_dir_entry iterate_bytes_in_block(t_block* data_block, bool is_last_data_block, t_data_position position, t_dir_entry (*process_entry)(t_dir_entry));
t_dir_entry _read_directory_entry(char* bytes);
char* _get_new_dir_name_from_path(char* path);
char* _get_pre_dir_path_from_path(char* path, char* new_dir_name);
bool __next_byte(u_int32_t pos, u_int32_t min_limit, u_int32_t max_limit, bool is_last_block);
u_int32_t _bytes_to_int32(char* bytes);

/*
 * Dada una entrada de directorio, recorre el nodo y agrega los archivos y carpetas que
 * están contenidos en él a una lista
 */
void add_childs_to_list(t_list**, t_dir_entry);
bool is_directory(t_dir_entry entry);
#endif //FILESYSTEM_DIRECTORY_H
