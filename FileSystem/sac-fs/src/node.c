#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"
#include "disk.h"
#include "bitmap.h"
#include "utils.h"
#include "node.h"

// A partir de del numero de nodo retorna el bloque donde se encuentra ese nodo
u_int32_t get_block_num_from_node_num(u_int32_t node_num) {
    return NODE_TABLE_INITIAL_BLOCK + node_num;
}

// Recibe un numero de inodo como parametro y retorna los bloques mapeados y el inodo
void read_node(u_int32_t node_num, t_node** node, t_block** node_block) {
    u_int32_t block_pos = get_block_num_from_node_num(node_num);
    *node_block = read_block(disk_fd, block_pos);
    *node = block_to_node(**node_block);

}

/*
 * Dado un número de nodo, libera el bit del bitmap asociado a él
 */
void delete_node(u_int32_t node_num){
    // Dado el número de nodo, puedo calcular a qué bloque está asociado
    u_int32_t numBloque = get_block_num_from_node_num(node_num);

    // Leo el nodo (para cambiarle el nombre a '\0'
    t_node* unNodo;// = malloc(sizeof(t_node));
    t_block* unBloque;// = malloc(sizeof(t_block));
    read_node(node_num, &unNodo, &unBloque);

    // Le mato el nombre
    unNodo->file_name[0] = '\0';
    // Y libero el estado
    unNodo->state = 0;

    // Sincronizo el cambio en el fs
    unmap_and_destroy_node(&unNodo,&unBloque);
    free_block(numBloque);
}

// Recibe el inodo mapeado y los bloques que ocupa como parametro,
// guarda los cambios en disco y destruye las estructuras
void unmap_and_destroy_node(t_node** node, t_block** node_block) {
    node_to_block(**node, *node_block);
    _unmap_and_destroy_block(*node_block);
    destroy_node(*node);
}

// Reserva un bloque en el bitmap para el nuevo bloque de punteros y
// agrega el nro del bloque al array de bloques de punteros del nodo
u_int32_t create_pointers_block_in_node(t_node* node, u_int16_t pointers_block_array_pos) {
    // Reserva el bloque en el bitmap
    u_int32_t new_pointers_block_num = get_free_data_block();

    // Agrega el nro del bloque reservado al array de bloques de punteros del nodo
    node->indirect_pointer_block[pointers_block_array_pos] = new_pointers_block_num;
    return new_pointers_block_num;
}

// Reserva un bloque en el bitmap para el nuevo bloque de datos y
// agrega el nro del nuevo bloque en la posicion indicada del bloque de punteros del nodo
u_int32_t create_data_block_in_pointer_block(t_node* node, u_int16_t pointers_block_array_pos, u_int16_t position) {

    // Reserva el bloque en el bitmap
    u_int32_t new_data_block_num = get_free_data_block();

    // Busca el bloque de punteros indicado en el nodo
    t_block* pointers_block = read_block(disk_fd, node->indirect_pointer_block[pointers_block_array_pos]);

    // Agrega el nro del bloque en la posicion indicada del bloque de punteros del nodo
    memcpy(&(pointers_block->bytes[position * sizeof(new_data_block_num)]), &new_data_block_num, sizeof(new_data_block_num));
    _unmap_and_destroy_block(pointers_block);
    return new_data_block_num;
}

// Actualiza la estructura del nodo con la informacion del bloque
void _sync_node(t_node** node, t_block node_block) {
    free(*node);
    *node = block_to_node(node_block);
}
void add_data_block_to_node(t_node* unNodo){
    /* Añade un bloque de datos al archivo.
     * Si el último bloque de punteros ya está lleno:
     *   Crea un nuevo bloque de punteros
     *   Asocia el nuevo bloque de punteros al nodo
     *   Busca un bloque libre
     *   En el bloque de punteros, ingresa la referencia al bloque libre
     */

    // Recorro el array de bloques de punteros
    int posicionArray;
    for (posicionArray=0; posicionArray < 1000 && unNodo->indirect_pointer_block[posicionArray] != 0; posicionArray++);
    posicionArray--;
    // En i tengo la cantidad de bloques de punteros que tiene el archivo
    // En indirect_pointer_block[i] tengo el número de bloque del último bloque de punteros
    int cantBloquesDatos = (unNodo->file_size / BLOCK_SIZE) + 1;

    u_int16_t posEnBloquePunteros;
    u_int16_t posEnArrayDeBloquesPunteros;

    if (cantBloquesDatos < (1024 * (posicionArray + 1))){
        /* Si tengo menos bloques de datos que los máximos que puedo indireccionar
         * con i bloques de punteros indirectos (entran 1024 punteros por bloque),
         * entonces puedo agregar un puntero más al bloque [i]*/

        posEnBloquePunteros = cantBloquesDatos % 1024;
        posEnArrayDeBloquesPunteros = posicionArray;

    } else {
        /* Si los bloques de punteros están llenos */
        // Creo un nuevo bloque de punteros en la posición i+1
        //printf("Voy a crear un bloque de punteros en la posicion %d\n", posicionArray+1);
        //printf("En la posicion %d del array, esta el valor %d\n", posicionArray+1, unNodo->indirect_pointer_block[posicionArray+1]);
        u_int32_t nuevoBloquePunteros = create_pointers_block_in_node(unNodo, posicionArray+1);
        //printf("Y ahora en la posicion %d del array, esta el valor %d\n", posicionArray+1, unNodo->indirect_pointer_block[posicionArray+1]);

        posEnBloquePunteros = 0;
        posEnArrayDeBloquesPunteros = posicionArray + 1;
    }

    create_data_block_in_pointer_block(unNodo,posEnArrayDeBloquesPunteros, posEnBloquePunteros);

}
t_block*   get_data_block_from_node(t_node* unNodo, u_int32_t numBloque){
    /*
     * Dado un nodo, devuelve el bloque de datos N° numBloque.
     */
    int posicionArray = numBloque / 1024;
    numBloque = numBloque % 1024;
    u_int32_t* numBloqueDatos = malloc(sizeof(u_int32_t));
    t_block* bloqueDatos;
    t_block* bloquePunteros;
    bloquePunteros = read_block(disk_fd,unNodo->indirect_pointer_block[posicionArray]);

    memcpy(numBloqueDatos,&(bloquePunteros->bytes[numBloque * sizeof(numBloqueDatos)]), sizeof(numBloqueDatos));
    bloqueDatos = read_block(disk_fd,*numBloqueDatos);
    free (numBloqueDatos);
    _unmap_and_destroy_block(bloquePunteros);
    return bloqueDatos;
}
void printNodeInfo (t_node* unNodo){
    int cantBloques = (unNodo->file_size / BLOCK_SIZE)+1;
    printf("Tipo: %d\n",unNodo->state);
    printf("Nombre: %s\n", unNodo->file_name);
    printf("Tamano: %d\n", unNodo->file_size);
    printf("Nodo padre: %d\n",unNodo->parent_block);
    printf("Fecha creacion: %d\n", unNodo->creation_date);
    printf("Fecha modificacion: %d\n", unNodo->modification_date);
    printf("Cant bloques: %d\n", cantBloques);
    printf("Array punteros:\n");
    int i;
    int j;

    char* numBloque = malloc(sizeof(u_int32_t));
    for (i=0; i<1000 && unNodo->indirect_pointer_block[i] != 0; i++) {
        printf("\t[%d] -> [%d]\n", i, unNodo->indirect_pointer_block[i]);
        t_block* bloquePunteros = read_block(disk_fd,unNodo->indirect_pointer_block[i]);
        for (j=0; j<1024 && cantBloques > 0; j++){
            if (j == 0)
                printf("\t\t");
            memcpy(numBloque, &(bloquePunteros->bytes[j * sizeof(u_int32_t)]),sizeof(u_int32_t));
            printf("%d - ",_bytes_to_int32(numBloque));
            cantBloques --;
        }
        free (bloquePunteros);
        if (j>0)
            printf("\n");
    }
    if (i == 0) {
        printf("\tNo tiene punteros\n");
    }
}
void printNodeInfoWithNodeNum(u_int32_t nodeNumber){
    t_block* bloqueNodo;
    t_node* nodo;
    read_node(nodeNumber,&nodo, &bloqueNodo);
    if (nodo->state == 0)
        return;
    printf("********************* NODO %d **********************\n",nodeNumber);
    printNodeInfo(nodo);
    printf("******************************************************\n");
    unmap_and_destroy_node(&nodo,&bloqueNodo);

}
void deleteLastDataBlock(t_node* unNodo){
    if (unNodo->file_size == 0){
        // No tiene ni bloque de punteros. Se va, no hay nada que hacer.
        return;
    }

    int ultimoBloqueDatos = (unNodo->file_size / BLOCK_SIZE);
    int ultimoBloquePunteros = ultimoBloqueDatos / 1024;

    // Pongo como liberado el último bloque de datos del nodo
    bitarray_clean_bit(bitmap,getBlockNumberFromNodeDataBlock(unNodo, ultimoBloqueDatos));
    if (ultimoBloqueDatos % 1024 == 0) {
        // Si estoy borrando el primer bloque de un bloque de punteros, borro tmb el bloque de punteros.
        bitarray_clean_bit(bitmap,unNodo->indirect_pointer_block[ultimoBloquePunteros]);
        unNodo->indirect_pointer_block[ultimoBloquePunteros] = 0;
    }
    unNodo->file_size = ultimoBloqueDatos == 0 ? 0 : (ultimoBloqueDatos - 1) * BLOCK_SIZE;
}
u_int32_t getBlockNumberFromNodeDataBlock(t_node* unNodo, u_int32_t numeroBloqueDatos){
    // Dado un nodo y un número de bloque del mismo, devuelve el numero
    // de bloque de dicho bloque de datos
    int posicionArray = numeroBloqueDatos / 1024;
    int numPunteroIndirecto = numeroBloqueDatos % 1024;
    u_int32_t* numBloque = malloc(sizeof(u_int32_t));

    t_block* bloquePunteros;
    bloquePunteros = read_block(disk_fd,unNodo->indirect_pointer_block[posicionArray]);

    memcpy(numBloque,&(bloquePunteros->bytes[numPunteroIndirecto * sizeof(numBloque)]), sizeof(numBloque));
    u_int32_t bloque = _bytes_to_int32(numBloque);
    _unmap_and_destroy_block(bloquePunteros);
    free(numBloque);
    return (bloque);
}