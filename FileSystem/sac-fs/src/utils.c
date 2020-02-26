#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include "utils.h"
#include "disk.h"
#include "node.h"

void print_disk_info(void) {
    printf("HEADER BLOCKS: %d\nBITMAP BLOCKS: %d\nNODE TABLE BLOCKS: %d\nDATA BLOCKS: %d\nTOTAL BLOCKS: %d\n",
            1, BITMAP_BLOCKS, NODE_TABLE_BLOCKS, DATA_BLOCKS, TOTAL_BLOCKS);
}

void print_bytes_as_binary(char* bytes, int size) {
    for (int j = 0 ; j < size ; j++) {
        for (int i = 7; i >= 0; --i) {
            putchar( (bytes[j] & (1 << i)) ? '1' : '0' );
        }
        if ( (j+1) % 5 == 0 ) putchar('\n');
        else putchar(' ');
    }
    putchar('\n');
}

/* Devuevlve el tamaño de un array */
int arr_length(char** arr) {
    int length;
    for(length=0 ; arr[length] != NULL ; length++);
    return length;
}

void arr_free(char** arr) {
    if (arr == NULL) return;
    for(int i=0; i < arr_length(arr); i++)
        free(arr[i]);
}

off_t file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0)
        return st.st_size;
    return -1;
}

bool file_exists(const char* path) {
    return access(path, F_OK) != -1;
}

t_header *create_default_header(void) {
    return create_header(IDENTIFIER, VERSION, BITMAP_INITIAL_BLOCK, BITMAP_SIZE);
}

t_node *create_root_dir(void) {
    u_int32_t ind_ptr_block[1000];
    memset(ind_ptr_block, 0, sizeof(ind_ptr_block));
    u_int64_t date = time(0);
    return create_node(2, "/", 0, 0, date, date, ind_ptr_block);
}

t_bitarray* create_default_bitmap(void) {
    char* bitarray = string_repeat(0b00000000, BITMAP_SIZE);
    t_bitarray* bitmap = bitarray_create_with_mode(bitarray, BITMAP_SIZE, MSB_FIRST);
    bitarray_set_bit(bitmap, 0); // Bloque para el header
    for (int i = 0 ; i < BITMAP_BLOCKS ; i++) // Bloques para el bitmap
        bitarray_set_bit(bitmap, i + 1);
    bitarray_set_bit(bitmap, BITMAP_BLOCKS + 1);
    /*
    for (int i = 0 ; i < NODE_TABLE_BLOCKS ; i++) // Bloques para los inodos
        bitarray_set_bit(bitmap, i + 1 + BITMAP_BLOCKS);
    */
    return bitmap;
}

t_header *create_header(char *identifier, u_int8_t version, u_int32_t bitmap_initial_block, u_int32_t bitmap_size) {
    t_header* header = (t_header*) malloc(sizeof(t_header));
    memcpy(header->identifier, identifier, 3);
    header->version = version;
    header->bitmap_initial_block = bitmap_initial_block;
    header->bitmap_size = bitmap_size;
    return header;
}

t_node *create_node(u_int8_t state, char *file_name, u_int32_t parent_block, u_int32_t file_size, u_int64_t creation_date,
                    u_int64_t modification_date, u_int32_t indirect_pointer_block[]) {
    t_node* node = (t_node*) malloc(sizeof(t_node));
    node->state  = state;
    strcpy(node->file_name, file_name);
    node->parent_block = parent_block;
    node->file_size = file_size;
    node->creation_date = creation_date;
    node->modification_date = modification_date;
    memcpy(node->indirect_pointer_block, indirect_pointer_block, sizeof(node->indirect_pointer_block));
    return node;
}

void header_to_block(t_header header, t_block *block) {
    int pos = 0;
    memcpy(block->bytes + pos, header.identifier           , sizeof(header.identifier));
    pos += sizeof(header.identifier);
    memcpy(block->bytes + pos, &header.version             , sizeof(header.version));
    pos += sizeof(header.version);
    memcpy(block->bytes + pos, &header.bitmap_initial_block, sizeof(header.bitmap_initial_block));
    pos += sizeof(header.bitmap_initial_block);
    memcpy(block->bytes + pos, &header.bitmap_size         , sizeof(header.bitmap_size));
}

t_header *block_to_header(t_block block) {
    char identifier[3];
    u_int32_t version, bitmap_initial_block, bitmap_size;

    int pos = 0;
    memcpy(identifier           , block.bytes + pos, sizeof(identifier));
    pos += sizeof(identifier);
    memcpy(&version             , block.bytes + pos, sizeof(version));
    pos += sizeof(version);
    memcpy(&bitmap_initial_block, block.bytes + pos, sizeof(bitmap_initial_block));
    pos += sizeof(bitmap_initial_block);
    memcpy(&bitmap_size         , block.bytes + pos, sizeof(bitmap_size));

    return create_header(identifier, version, bitmap_initial_block, bitmap_size);
}

void bitmap_to_block(t_bitarray bitmap, t_block** blocks) {
    int block_num = 0;
    for (int i = 0; i < bitmap.size ; i++) {
        blocks[block_num]->bytes[i] = bitmap.bitarray[i];
        if (i % BLOCK_SIZE == 1 && (i+1) % BLOCK_SIZE == 0) block_num++;
    }
}

t_bitarray* block_to_bitmap(t_block** blocks) {
    char* bitarray = malloc(sizeof(char)*BITMAP_SIZE);
    int block_num = 0;
    for (int i = 0; i < BITMAP_SIZE ; i++) {
        bitarray[i] = blocks[block_num]->bytes[i] ;
        if (i % BLOCK_SIZE == 1 && (i+1) % BLOCK_SIZE == 0) block_num++;
    }
    return bitarray_create_with_mode(bitarray, BITMAP_SIZE, MSB_FIRST);
}

void node_to_block(t_node node, t_block *block) {
    int pos = 0;
    memcpy(block->bytes + pos, &node.state                 , sizeof(node.state));
    pos += sizeof(node.state);
    memcpy(block->bytes + pos, node.file_name              , sizeof(node.file_name));
    pos += sizeof(node.file_name);
    memcpy(block->bytes + pos, &node.parent_block          , sizeof(node.parent_block));
    pos += sizeof(node.parent_block);
    memcpy(block->bytes + pos, &node.file_size             , sizeof(node.file_size));
    pos += sizeof(node.file_size);
    memcpy(block->bytes + pos, &node.creation_date         , sizeof(node.creation_date));
    pos += sizeof(node.creation_date);
    memcpy(block->bytes + pos, &node.modification_date     , sizeof(node.modification_date));
    pos += sizeof(node.modification_date);
    memcpy(block->bytes + pos, &node.indirect_pointer_block, sizeof(node.indirect_pointer_block));
}

t_node *block_to_node(t_block block) {
    u_int8_t state;
    char file_name[71];
    u_int32_t parent_block, file_size;
    u_int64_t creation_date, modification_date;
    u_int32_t indirect_pointer_block[1000];

    int pos = 0;
    memcpy(&state, block.bytes + pos, sizeof(state));
    pos += sizeof(state);
    memcpy(&file_name, block.bytes + pos, sizeof(file_name));
    pos += sizeof(file_name);
    memcpy(&parent_block, block.bytes + pos, sizeof(parent_block));
    pos += sizeof(parent_block);
    memcpy(&file_size, block.bytes + pos, sizeof(file_size));
    pos += sizeof(file_size);
    memcpy(&creation_date, block.bytes + pos, sizeof(creation_date));
    pos += sizeof(creation_date);
    memcpy(&modification_date, block.bytes + pos, sizeof(modification_date));
    pos += sizeof(modification_date);
    memcpy(indirect_pointer_block, block.bytes + pos, sizeof(indirect_pointer_block));

    return create_node(state, file_name, parent_block, file_size, creation_date, modification_date, indirect_pointer_block);
}

void destroy_header(t_header *header) {
    free(header);
}

void destroy_node(t_node* node) {
    free(node);
}

void add_entry_to_list (t_list** unaLista, t_dir_entry unaEntrada){
    t_dir_entry* ent = malloc(sizeof(t_dir_entry));
    ent->node_num = unaEntrada.node_num;
    strcpy(ent->name, unaEntrada.name);
    list_add(*unaLista, ent);
}
void delete_parent_entry(t_dir_entry unaEntrada){
    // Abro el nodo, busco al padre, abro al padre, busco la entrada y la elimino
    t_node* unNodo;
    t_block* unBloque;
    t_node* nodoPadre;// = malloc(sizeof(t_node));
    t_block* bloquePadre;

    //  Leo el nodo en cuestión
    read_node(unaEntrada.node_num, &unNodo, &unBloque);
    // Leo el padre
    bloquePadre = read_block(disk_fd, unNodo->parent_block);
    nodoPadre = block_to_node(*bloquePadre);
    delete_directory_entry(nodoPadre, unaEntrada.name);

    unmap_and_destroy_node(&unNodo, &unBloque);
    unmap_and_destroy_node(&nodoPadre, &bloquePadre);
}
char* getFolderPathFromFilePath(char* filePath){
    // Dado un path de archivo, devuelve la ruta de la carpeta

    // Parto el path por la barra
    char** splitted_path = string_split(filePath, "/");
    int len = arr_length(splitted_path);

    // Devuelvo _todo, menos el ultimo pedacito.
    // (Ultimo pedacito = (lenght(rutaArchivo) - nombreArchivo)
    char* folder_path = string_substring_until(filePath, string_length(filePath) - string_length(splitted_path[len-1]) - 1);
    arr_free(splitted_path);
    free(splitted_path);
    return folder_path;
}
char* getFileNameFromFilePath(char* filePath){
    char** splitted_path = string_split(filePath, "/");
    int len = arr_length(splitted_path);
    char* new_file_name = string_duplicate(splitted_path[len-1]);
    arr_free(splitted_path);
    free(splitted_path);
    return new_file_name;
}
int min(int a, int b){
    return (a > b) ? b : a;
}