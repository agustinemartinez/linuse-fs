#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <commons/string.h>
#include "utils.h"
#include "disk.h"

void format_disk(void) {
    // Crea el haeder, el bitmap y el nodo raiz
    t_header* header = create_default_header();
    bitmap = create_default_bitmap();
    t_node* root_dir = create_root_dir();

    // Mapea los bloques necesarios a memoria y escribe la metadata
    t_block* header_blocks = read_block(disk_fd, 0);
    header_to_block(*header, header_blocks);
    _unmap_and_destroy_block(header_blocks);

    t_block** bitmap_blocks = read_blocks(disk_fd, BITMAP_INITIAL_BLOCK, BITMAP_BLOCKS);
    bitmap_to_block(*bitmap, bitmap_blocks);
    _unmap_and_destroy_blocks(bitmap_blocks);

    t_block* root_node_blocks = read_block(disk_fd, NODE_TABLE_INITIAL_BLOCK);
    node_to_block(*root_dir, root_node_blocks);
    _unmap_and_destroy_block(root_node_blocks);

    disk_close(disk_fd);

    print_disk_info();
    print_bytes_as_binary(bitmap->bitarray, bitmap->size);

    destroy_header(header);
    destroy_node(root_dir);
    free(bitmap->bitarray);
    bitarray_destroy(bitmap);

    exit(0);
}

void disk_create(void) {
    if (!file_exists(DISK_PATH) || DISK_SIZE > file_size(DISK_PATH)) {
        mkdir(SAC_ROOT_PATH, 0700);
        char* command = string_from_format("dd if=/dev/urandom of=%s bs=%lu count=1 iflag=fullblock", DISK_PATH, DISK_SIZE);
        system(command);
        free(command);
    }
}

int disk_open(void) {
    // Si el archivo no existe lo crea en SAC_ROOT_PATH
    disk_create();

    int fd = open(DISK_PATH, O_RDWR);
    if (fd < 0) {
        printf("Hubo un problema al acceder al disco.\n");
        exit(-2);
    }
    return fd;
}

void disk_close(int file_descriptor) {
    close(file_descriptor);
}
