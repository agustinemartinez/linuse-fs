#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <commons/string.h>
#include "block.h"
#include "disk.h"
#include "bitmap.h"
#include "utils.h"
#include "node.h"
#include "directory.h"

// Retorna la entrada (numero de nodo + nombre) correspondiente al path ingresado
// Ejemplo:
// search_path("/") -> retorna t_dir_entry { .name = "/", .node_num = 0 }
// search_path("/animales/gato.png") -> retorna t_dir_entry { .name = "gato.png", .node_num = X }
t_dir_entry search_path(char * path) {
    // Splitea el path
    char** splitted_path = string_split(path, "/");
    int len = arr_length(splitted_path);

    // Primero recupero al nodo raiz
    t_dir_entry entry = (t_dir_entry) { .node_num = 0, .name = '/' };

    // Recorro el path desde el nodo raiz hasta encontrar el ultimo nodo
    for (int i=0; i < len && is_valid_entry(entry) ; i++) {
        t_node* node;
        t_block* node_block;
        read_node(entry.node_num, &node, &node_block);
        entry = get_entry(node, splitted_path[i]);
        unmap_and_destroy_node(&node, &node_block);
        free(splitted_path[i]);
    }
    free(splitted_path);
    // Retorna una entrada invalida ante un error y el numero de nodo si lo encuentra
    return entry;
}

// Agrega una nueva entrada (numero de nodo + nombre) al inodo
void add_directory_entry(t_node* node, char* name, u_int32_t node_num) {
    // Pongo el nombre en una entrada de 124 bytes + 4 bytes de node_num
    char entry_name[124];
    strcpy(entry_name, name);

    // Cargo la posicion donde agregar la entrada en la estructura t_data_position
    t_data_position position = get_next_write_position(node);
    t_block* data_block = read_next_data_block(node, position);

    // Escribo la nueva entrada en el bloque
    memcpy(&(data_block->bytes[position.byte_num]), &node_num, sizeof(node_num));
    memcpy(&(data_block->bytes[position.byte_num + sizeof(node_num)]), &entry_name, sizeof(entry_name));
    _unmap_and_destroy_block(data_block);

    // Incrementar el file size en el nodo
    node->file_size += ( sizeof(node_num) + sizeof(entry_name) );
}

void delete_directory_entry(t_node* node, char* name){
    // Elimina la entrada que coincide con el nombre de directorio de entre las entradas de un nodo
    t_dir_entry delete_this_entry(t_dir_entry entry) {
        if (!strcmp(entry.name,name)){
            // Si la entrada es la buscada la piso con lo que sigue del bloque, o 0 si no seguía nada
            return entry;
        }
        t_dir_entry invalid_entry = { .name[0] = '\0' };
        return invalid_entry; // Retorna una estructura invalida para que continue iterando las demas entradas
    }
    t_dir_entry entradaEliminar;
    entradaEliminar = iterate_pointers_blocks(node, delete_this_entry);
    if (!is_valid_entry(entradaEliminar)) return;
    //Borro la entrada
    t_block* unBloque = read_block(disk_fd, entradaEliminar.data.block_num);
    char* nombreEntrada = malloc(sizeof(char) * 128);
    memcpy(nombreEntrada,&(unBloque->bytes[entradaEliminar.data.byte_num]), 128);
    memcpy(&(unBloque->bytes[entradaEliminar.data.byte_num]),&(unBloque->bytes[(entradaEliminar.data.byte_num) + DIR_ENTRY_SIZE]),BLOCK_SIZE - ((entradaEliminar.data.byte_num) + DIR_ENTRY_SIZE));
    _unmap_and_destroy_block(unBloque);
    node->file_size -= DIR_ENTRY_SIZE;
    if (node->file_size == 0) {
        // Si le borré todas las entradas, elimino la primera pos del vector de bloques de punteros
        node->indirect_pointer_block[0] = 0;
    }
    free(nombreEntrada);
}

void delete_all_directory_entries(u_int32_t node_num){
    int i;
    t_node* unNodo;// = malloc(sizeof(t_node));
    t_block* unBloque;// = malloc(sizeof(t_block));
    read_node(node_num, &unNodo, &unBloque);
    // Recorro _todo el array de punteros indirectos, y voy liberando los bloques
    for (i=0; i<1000 && unNodo->indirect_pointer_block[i] != 0; i++){
        // Tomar el bloque y marcarlo en el bitmap como libre
        free_block(unNodo->indirect_pointer_block[i]);
    }
    unmap_and_destroy_node(&unNodo, &unBloque);
}

// Retorna la entrada (nombre + nro de nodo) del directorio con el nombre ingresado
// Si no la encuentra retorna una entrada invalida (nombre vacio)
t_dir_entry get_entry(t_node* node, char* entry_name) {
    t_dir_entry find_entry(t_dir_entry entry) {
        t_dir_entry invalid_entry = { .name[0] = '\0' }; // Si no tiene el mismo nombre retorna una entrada invalida
        return !strcmp(entry.name, entry_name) ? entry : invalid_entry;
    }
    return iterate_pointers_blocks(node, find_entry);
}

bool exists_entry(t_node* node, char* entry_name) {
    t_dir_entry entry = get_entry(node, entry_name);
    return is_valid_entry(entry);
}

// Retorna la posicion del ultimo byte del archivo a partir de su tamano
// Nro de bloque de punteros + Nro de bloque dentro del bloque de punteros + Nro de byte dentro del bloque de datos
t_data_position get_next_write_position(t_node* node) {
    // Calcula la cantidad de bloques que tiene el nodo, en función de su tamaño y el tamaño de bloque
    u_int32_t total_block_num  = node->file_size/BLOCK_SIZE;

    t_data_position position;
    position.pointer_block_num = total_block_num/1024;
    position.block_num         = total_block_num%1024;
    position.byte_num          = node->file_size%BLOCK_SIZE;
    return position;
}

bool is_valid_entry(t_dir_entry entry) {
    return entry.name[0] != '\0';
}

bool is_valid_path(char* path) {
    return !string_is_empty(path) && string_starts_with(path, "/") && (strlen(path) == 1 || !string_ends_with(path, "/"));
}

// Imprime por pantalla todas las entradas del nodo
void print_entries(t_node* node) {
    t_dir_entry print_entry(t_dir_entry entry) {
        printf("%d --- %s \n", entry.node_num, entry.name);
        t_dir_entry invalid_entry = { .name[0] = '\0' };
        return invalid_entry; // Retorna una estructura invalida para que continue iterando las demas entradas
    }
    iterate_pointers_blocks(node, print_entry);
}


/*
 *
 * FUNCIONES AUXILIARES
 *
 */

/* Crea un nodo de directorio. Le asigna al array de punteros indirectos todo 0
 * */
t_node* create_dir_node(char* name, u_int32_t parent_node_num) {
    u_int32_t parent_block = get_block_num_from_node_num(parent_node_num);
    u_int32_t ind_ptr_block[1000];
    memset(ind_ptr_block, 0, sizeof(ind_ptr_block));
    u_int64_t date = time(0);
    return create_node(2, name, parent_block, 0, date, date, ind_ptr_block);
}

void create_and_save_dir_node(char* new_dir_name, u_int32_t pre_dir_node_num, u_int32_t new_block_num) {
    t_node* new_dir_node = create_dir_node(new_dir_name, pre_dir_node_num);
    t_block* new_node_block = read_block(disk_fd, new_block_num);
    node_to_block(*new_dir_node, new_node_block);
    unmap_and_destroy_node(&new_dir_node, &new_node_block);
}

void create_dir_node_and_add_entry(t_node* pre_dir_node, u_int32_t pre_dir_node_num, char* new_dir_name) {
    // Reservo el nuevo nodo en el bitmap
    u_int32_t new_block_num = get_free_node_block();
    u_int16_t node_table_initial_block = NODE_TABLE_INITIAL_BLOCK;
    u_int16_t new_node_num = new_block_num - node_table_initial_block;

    // Creo el inodo y lo guardo
    create_and_save_dir_node(new_dir_name, pre_dir_node_num, new_block_num);

    // Agrego la entrada de directorio padre
    add_directory_entry(pre_dir_node, new_dir_name, new_node_num);
}
void delete_dir_node_and_remove_entry (t_node* pre_dir_node, u_int32_t pre_dir_node_num, char* new_dir_name) {
    // HACER. NO ESTA HECHA
    // Reservo el nuevo nodo en el bitmap
    u_int32_t new_block_num = get_free_node_block();
    u_int16_t node_table_initial_block = NODE_TABLE_INITIAL_BLOCK;
    u_int16_t new_node_num = new_block_num - node_table_initial_block;

    // Creo el inodo y lo guardo
    create_and_save_dir_node(new_dir_name, pre_dir_node_num, new_block_num);

    // Agrego la entrada de directorio padre
    add_directory_entry(pre_dir_node, new_dir_name, new_node_num);
}

t_block* read_next_data_block(t_node* node, t_data_position position) {
    u_int32_t data_block_num;
    // Si todavia no hay bloques de punteros asignados en la posicion donde escribir
    if ( node->indirect_pointer_block[position.pointer_block_num] == 0 ) {
        // Crear y asignar al inodo el nuevo bloque de punteros
        create_pointers_block_in_node(node, position.pointer_block_num);
        // Crear y asignar el inodo del nuevo bloque de datos en el bloque de punteros
        data_block_num = create_data_block_in_pointer_block(node, position.pointer_block_num, position.block_num);

    } else {
        // Leo el bloque de punteros
        t_block* pointers_block = read_block(disk_fd, node->indirect_pointer_block[position.pointer_block_num]);
        // Busco el numero del bloque de datos
        char* block_bytes = &(pointers_block->bytes[position.block_num * sizeof(u_int32_t)]);
        data_block_num = _bytes_to_int32(block_bytes);
        _unmap_and_destroy_block(pointers_block);
    }
    // Leo el bloque de datos
    return read_block(disk_fd, data_block_num);
}

// Itera las entradas de un directorio y les aplica la funcion que se pasa por parametro
// Si la funcion retorna una entrada invalida (nombre vacio) continua iterando
// Si la funcion retorna una entrada valida corta el flujo y retorna esa entrada
t_dir_entry iterate_pointers_blocks(t_node* node, t_dir_entry (*process_entry)(t_dir_entry)) {
    t_data_position position = get_next_write_position(node);
    t_dir_entry result = { .name[0] = '\0' }; // Si el resultado es una estructura invalida continua iterando
    //
    int i;
    for (i=0 ; !is_valid_entry(result) && node->indirect_pointer_block[i] != 0  && node->state != 0; i++) {
        // Lee el bloque al que apunta el puntero[i] del nodo
        t_block* pointers_block = read_block(disk_fd, node->indirect_pointer_block[i]);
        // Se fija si es o no el último
        bool is_last_pointers_block = i == position.pointer_block_num;
        // Itera el bloque
        result = iterate_blocks(pointers_block, is_last_pointers_block, position, process_entry);
        _unmap_and_destroy_block(pointers_block);
    }
    // El result.data.byte_num ya viene cargado de las fuciones de más adentro
    // Guardo en la entrada de directorio el número de bloque donde está la entrada de directorio
    // No se para que me sirve el pointer_block_num, seguro para nada acá.
    result.data.pointer_block_num = -1;
    return result;
}
// Itera las entradas de un directorio y les aplica la funcion que se pasa por parametro
// Si la funcion retorna una entrada invalida (nombre vacio) continua iterando
// Si la funcion retorna una entrada valida añade esa entrada a una lista que viene por parámetro
void iterate_pointers_add_list(t_list** unaLista, t_node* node, t_dir_entry (*process_entry)(t_dir_entry)) {
    t_data_position position = get_next_write_position(node);
    t_dir_entry result = { .name[0] = '\0' }; // Si el resultado es una estructura invalida continua iterando

    for (int i=0 ; !is_valid_entry(result) && node->indirect_pointer_block[i] != 0 ; i++) {

        t_block* pointers_block = read_block(disk_fd, node->indirect_pointer_block[i]);
        bool is_last_pointers_block = i == position.pointer_block_num;
        result = iterate_blocks(pointers_block, is_last_pointers_block, position, process_entry);

        // Agrego la entrada a la lista
        add_entry_to_list(unaLista,result);

        // Fuerzo entrada invalida para que siga iterando
        result.name[0] = '\0';
        _unmap_and_destroy_block(pointers_block);
    }
}



// La posicion es la ubicacion del ultimo byte del ultimo bloque de datos del ultimo bloque de punteros
t_dir_entry iterate_blocks(t_block* pointers_block, bool is_last_pointers_block,
                           t_data_position position, t_dir_entry (*process_entry)(t_dir_entry)) {
    u_int32_t nroBloque;
    t_dir_entry result = { .name[0] = '\0' }; // Si el resultado es una estructura invalida continua iterando
    for (int j=0 ; !is_valid_entry(result) && __next_byte(j, position.block_num, 1024, is_last_pointers_block) ; j++) {
        char* bytes = &(pointers_block->bytes[j * sizeof(u_int32_t)]);
        nroBloque =  _bytes_to_int32(bytes);
        t_block* data_block = read_block(disk_fd,nroBloque);
        bool is_last_data_block = (j == position.block_num) && is_last_pointers_block;
        result = iterate_bytes_in_block(data_block, is_last_data_block, position, process_entry);
        _unmap_and_destroy_block(data_block);
    }
    result.data.block_num = nroBloque;
    return result;
}

t_dir_entry iterate_bytes_in_block(t_block* data_block, bool is_last_data_block,
                                   t_data_position position, t_dir_entry (*process_entry)(t_dir_entry)) {
    t_dir_entry result = { .name[0] = '\0' }; // Si el resultado es una estructura invalida continua iterando
    int k;
    for (k=0 ; !is_valid_entry(result)  && __next_byte(k, position.byte_num - 1, BLOCK_SIZE, is_last_data_block) ; k+=128) {
        t_dir_entry entry = _read_directory_entry(&(data_block->bytes[k]));
        result = process_entry(entry);
    }
    result.data.byte_num = (k-=128);
    return result;
}

t_dir_entry _read_directory_entry(char* bytes) {
    t_dir_entry entry;
    entry.node_num = _bytes_to_int32(bytes);
    strcpy(entry.name, bytes + sizeof(entry.node_num));
    return entry;
}

char* _get_new_dir_name_from_path(char* path) {
    char** splitted_path = string_split(path, "/");
    int len = arr_length(splitted_path);
    char* new_dir_name = string_duplicate(splitted_path[len-1]);
    arr_free(splitted_path);
    free(splitted_path);
    return new_dir_name;
}

char* _get_pre_dir_path_from_path(char* path, char* new_dir_name) {
    return string_substring_until(path, strlen(path) - strlen(new_dir_name));
}

bool __next_byte(u_int32_t pos, u_int32_t min_limit, u_int32_t max_limit, bool is_last_block) {
    return pos <= min_limit || (pos < max_limit && !is_last_block);
}

u_int32_t _bytes_to_int32(char* bytes) {
    u_int32_t data;
    memcpy(&data, bytes, sizeof(u_int32_t));
    return data;
}
void add_childs_to_list(t_list** unaLista, t_dir_entry unaDirEntry){
    /* Agrega entries a una lista */
    t_node* unNodo;
    t_block* unBloque;
    read_node(unaDirEntry.node_num, &unNodo, &unBloque);

    t_dir_entry process_entry (t_dir_entry entry) {
        // Añado la entrada a la lista
        add_entry_to_list(unaLista, entry);
        // Devuelvo "invalid entry" para que el algoritmo siga procesando
        t_dir_entry invalid_entry = { .name[0] = '\0' };
        return invalid_entry; // Retorna la entrada invalida
    }

    iterate_pointers_blocks(unNodo, process_entry);
    unmap_and_destroy_node(&unNodo, &unBloque);
}
bool is_directory(t_dir_entry entry){
    int estado;
    t_node* unNodo;
    t_block* unBloque;
    read_node(entry.node_num, &unNodo, &unBloque);
    estado = unNodo->state;
    free(unNodo);
    free(unBloque);
    return estado == 2;
}