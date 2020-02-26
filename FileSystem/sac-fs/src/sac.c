#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>
#include "utils.h"
#include "sacio.h"
#include "directory.h"
#include "node.h"
#include "../../mtserver/src/server.h"
#include "sac.h"

int main(int argc, char* argv[]) {
    //validate_input(argc, argv);
    //server_init();

    // Abro el archivo del disco
    disk_fd = disk_open();

    // Valido que los parametros ingresados sean correctos
    validate_input(argc, argv);

    // Para formatear el disco descomentar esta linea
    //format_disk();

    // Si no se formatea el disco inicializo las estructuras del FS (bitmap, etc.) con la info del disco
    t_block** bitmap_blocks;
    load_bitmap(&bitmap_blocks);

    // Luego el server empieza a escuchar pedidos
    if (true) {
        server_init();
    }

    // Recupero el nodo indicado en el path
    t_node* node;
    t_block* node_block;
    t_dir_entry entrada = search_path("/");
    read_node(entrada.node_num, &node, &node_block);

    // Actualizo la estructura del nodo para imprimirla por pantalla
    _sync_node(&node, *node_block);
    print_entries(node);

    sac_open("/archivo1.txt",0);

    if (false){
        // Escribir
        char* buf = malloc(sizeof(char)*101);
        sprintf(buf,"a123456789b123456789c123456789d123456789e123456789f123456789g123456789h123456789i123456789j123456789");
        for(int i=0; i<30000; i++) {
            sac_write(sac_open("/archivo2.txt",0), buf, 100, 100*i);
        }
    }
    if (false) {
        // Leer
        char* otroBuf = malloc(sizeof(char) * 1001);
        for (int i=0; i < 5000; i++){
            sac_read(1,otroBuf,1000, 1000 * i);
            otroBuf[1000] =  '\0';
            printf("%s\n", otroBuf);
        }
    }
    printf("sac_truncate devuelve %d\n",sac_truncate("/archivo1.txt",4000));
    printNodeInfoWithNodeNum(sac_open("/archivo1.txt",0));

    _sync_node(&node, *node_block);
    print_entries(node);

    // unmapeo el nodo y lo destruyo
    unmap_and_destroy_node(&node, &node_block);
    // Guardo el bitmap en el disco y lo imprimo por pantalla
    unmap_bitmap(bitmap_blocks);

    // Cierro el archivo del disco
    disk_close(disk_fd);

    return 0;
}

void set_config(char* path_config) {
    if (access(path_config, F_OK) == -1) {
        printf("No se encontro el archivo de configuracion.\n");
        exit(-1);
    }
    t_config* cfg = config_create( path_config );
    LISTEN_PORT   = string_itoa( config_get_int_value(cfg, "LISTEN_PORT") );
    USE_LINUX_FS  = config_get_int_value(cfg, "USE_LINUX_FS");
    config_destroy(cfg);
}

void validate_input(int argc, char * argv[]) {
    if ( argc == 1 ) { // Momentaneo para seguir ejecutando sin argumentos
        USE_LINUX_FS = DEFAULT_USE_LINUX_FS;
        LISTEN_PORT  = DEFAULT_PORT;
        return;
    }
    if ( argc == 2 ) {
        if ( string_ends_with(argv[1], ".cfg") ) {
            printf("Se recibio un archivo de configuracion.\n");
            set_config(argv[1]); // Cargo la IP recibida por archivo de configuracion
            return;
        }
        else if ( string_ends_with(argv[1], "-f") || string_ends_with(argv[1], "-format") ) {
            printf("Se realizara un formateo del disco.\n");
            format_disk();
            exit(0);
        }
    }
    printf("Los parametros ingresados son incorrectos.\n");
    exit(-1);
}

void load_bitmap(t_block*** bitmap_blocks) {
    *bitmap_blocks = read_blocks(disk_fd, BITMAP_INITIAL_BLOCK, BITMAP_BLOCKS);
    bitmap = block_to_bitmap(*bitmap_blocks);
    printf("Bitmap:\n");
    print_bytes_as_binary(bitmap->bitarray, bitmap->size);
}

void unmap_bitmap(t_block** bitmap_blocks) {
    //printf("Bitmap:\n");
    //print_bytes_as_binary(bitmap->bitarray, bitmap->size);
    bitmap_to_block(*bitmap, bitmap_blocks);
    _unmap_and_destroy_blocks(bitmap_blocks);
}
