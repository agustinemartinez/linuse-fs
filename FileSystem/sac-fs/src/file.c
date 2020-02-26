//
// Created by utnso on 18/11/19.
//

#include "file.h"
#include "utils.h"
#include "node.h"
#include "disk.h"
#include "bitmap.h"
#include <stdio.h>

// Crea el nodo del archivo y lo referencia a su directorio padre
// NOTA: Esta función es análoga a create_and_save_dir_node, pero el parámetro
// "new_block_num" lo genero dentro de la función, no lo paso desde afuera.
u_int32_t create_and_save_file_node(char* file_name, u_int32_t folder_node_num){

    t_node* folder_node;
    t_block* folder_node_block;
    u_int32_t NodoInicial = NODE_TABLE_INITIAL_BLOCK;

    read_node(folder_node_num,&folder_node,&folder_node_block);

    // Verfico que no existe otro archivo con el mismo nombre
    if ( exists_entry(folder_node, file_name) ) {
        // Si existe la entrada, devuelvo el número de nodo
        t_dir_entry entradaArchivo = get_entry(folder_node,file_name);
        unmap_and_destroy_node(&folder_node,&folder_node_block);
        return (entradaArchivo.node_num); //- NodoInicial);
    }

    // Creo un nodo de archivo
    t_node* new_file_node = create_file_node(file_name, folder_node_num);

    // Tomo un bloque libre
    u_int32_t new_block_num = get_free_node_block();

    // Carga el bloque nuevo en la variable t_block
    t_block* new_file_block = read_block(disk_fd, new_block_num);

    // Carga el contenido del nodo al bloque
    node_to_block(*new_file_node, new_file_block);

    // Creo la entrada de "directorio" en el padre
    add_directory_entry(folder_node, file_name, new_block_num - NodoInicial);

    // Sincroniza el cambio y desconecta
    unmap_and_destroy_node(&new_file_node, &new_file_block);
    unmap_and_destroy_node(&folder_node,&folder_node_block);

    // Retorna el nodo
    return (new_block_num - NodoInicial);
}
t_node* create_file_node(char* new_file_name, u_int32_t  folder_node_num){
    u_int32_t parent_block = get_block_num_from_node_num(folder_node_num);
    u_int32_t ind_ptr_block[1000];
    memset(ind_ptr_block, 0, sizeof(ind_ptr_block));
    u_int64_t date = time(0);
    return create_node(1, new_file_name, parent_block, 0, date, date, ind_ptr_block);
}
