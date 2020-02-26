#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "directory.h"
#include "node.h"
#include "sacio.h"
#include "utils.h"
#include <commons/collections/list.h>
#include "file.h"
#include <errno.h>

int sac_mkdir(const char *path, mode_t mode) {
    // Ignoro mode porque el FS no maneja permisos sobre los archivos
    // Verifico que el path sea valido
    if ( !is_valid_path((char*)path) ) return -1;

    char* new_dir_name = _get_new_dir_name_from_path((char*)path);
    char* pre_dir_path = _get_pre_dir_path_from_path((char*)path, new_dir_name);

    // Obtener el inodo del directorio anterior
    t_dir_entry entry = search_path(pre_dir_path);

    // Verifico que encontro el directorio anterior
    if ( is_valid_entry(entry) ) {
        t_node* pre_dir_node;
        u_int32_t pre_dir_node_num = entry.node_num;

        // Me traigo el nodo del directorio anterior
        t_block* pre_dir_node_block;
        read_node(pre_dir_node_num, &pre_dir_node, &pre_dir_node_block);

        // Verfico que no existe otro directorio con el mismo nombre
        if ( exists_entry(pre_dir_node, new_dir_name) ) {
            unmap_and_destroy_node(&pre_dir_node, &pre_dir_node_block);
            return -1;
        }

        // Crea el nodo del nuevo directorio y agrega la entrada en el directorio padre
        create_dir_node_and_add_entry(pre_dir_node, pre_dir_node_num, new_dir_name);
        unmap_and_destroy_node(&pre_dir_node, &pre_dir_node_block);
    }
    else return -1;

    // Liberar memoria
    free(new_dir_name);
    free(pre_dir_path);

    return 0;
}

int sac_rmdir(const char* path) {
    // Verifico que el path sea válido
    if (!is_valid_path((char *) path)) return -1;

    // Obtengo el nombre del directorio a eliminar
    char *dir_name = _get_new_dir_name_from_path((char *) path);

    // Obtener el inodo del directorio
    t_dir_entry entry = search_path((char*)path);

    // Verifico si el directorio es válido. Si no es válido se las toma
    if ( !is_valid_entry(entry) ) return -1;

    // Verifico si es un directorio
    if (!is_directory(entry)) return (-1);

    t_node* nodoDirectorioEliminar;
    t_block* bloqueDirectorioEliminar;

    read_node(entry.node_num,&nodoDirectorioEliminar,&bloqueDirectorioEliminar);
    if (nodoDirectorioEliminar->file_size > 0) {
        // El directorio a eliminar no está vacío
        printf("Pillin, estabas tratando de borrar la carpeta %s que tiene cosas (pesa %d bytes). Retornare ENOTEMPTY (%d).\n", nodoDirectorioEliminar->file_name, nodoDirectorioEliminar->file_size, ENOTEMPTY);
        unmap_and_destroy_node(&nodoDirectorioEliminar,&bloqueDirectorioEliminar);
        return -2;//ENOTEMPTY;
    }



    delete_parent_entry(entry);

    // Generar Lista del arbol de directorios
    t_list* listaEliminacion = list_create();

    // Añade el archivo/carpeta a la lista
    add_entry_to_list(&listaEliminacion, entry);

    // Si es una carpeta, leer todas las entradas de directorio que tenga y agregarlas a la lista
    add_childs_to_list(&listaEliminacion, entry);

    // Ya metí el nodo del archivo/carpeta a eliminar.
    // Ahora reviso si al meter ese nodo, aparecieron otros directorios en los que tenga que buscar adentro tambien
    int i;
    // En la posicion 0 de la lista, está la carpeta que quise eliminar originalmente. Empiezo a leer desde la
    // posición siguiente, buscando que aparezcan más cosas.
    t_dir_entry* unaEntrada;// = malloc(sizeof(t_dir_entry));
    for (i=1; i<list_size(listaEliminacion); i++) {
        // Si es una carpeta, leer todas las entradas de directorio que tenga y agregarlas a la lista
        unaEntrada = list_get(listaEliminacion,i);
        strcpy(entry.name, unaEntrada->name);
        entry.node_num = unaEntrada->node_num;
        add_childs_to_list(&listaEliminacion, entry);
    }

    for (i=0; i<list_size(listaEliminacion); i++) {
        unaEntrada = list_get(listaEliminacion,i);
        strcpy(entry.name, unaEntrada->name);
        entry.node_num = unaEntrada->node_num;
        printf("Nodo: %d - Nombre: %s\n", entry.node_num, entry.name);
        delete_all_directory_entries(entry.node_num);
        delete_node(entry.node_num);
    }
    free (dir_name);

    void destroy_entry(t_dir_entry* e) { free(e->name); }
    list_destroy_and_destroy_elements(listaEliminacion, destroy_entry);
    return 0;
}

int sac_open(const char* path, int mode){
    // Verifico si existe el archivo.
    //   Si no existe, lo creo
    //   Si existe, lo creo y devuelvo el número de nodo

    // Verifico que el path sea válido
    if (!is_valid_path((char *) path)) return -1;

    // Verifico si existe el directorio

    // Ruta de la carpeta que contendrá al archivo
    char* pathCarpeta;
    pathCarpeta = getFolderPathFromFilePath(path);

    // Obtener el inodo del directorio y verifico si existe
    t_dir_entry entryCarpeta = search_path(pathCarpeta);

    // Verifico si el directorio es válido. Si no es válido se las toma
    if ( !is_valid_entry(entryCarpeta) ) return -1;

    char* nombreArchivo = getFileNameFromFilePath(path);

    u_int32_t nodoArchivo;
    // Verifica adentro si existe o no el archivo
    nodoArchivo = create_and_save_file_node(nombreArchivo,entryCarpeta.node_num);

    free(pathCarpeta);
    free(nombreArchivo);
    return nodoArchivo;
}

size_t sac_write(int fd, const void *buffer, size_t count, off_t offset){
    t_node* file_node;
    t_block* file_node_block;

    int espacioDisponible;
    int cantBytesEscribir;
    int cantBytesEscritos = 0;
    int cantBytesEscritosEnBloqueActual = 0;

    read_node(fd,&file_node,&file_node_block);

    /*
     * Primero veo si el archivo tiene al menos un bloque de datos asociado
     * Si no tiene, le asocio uno.
     */

    if (file_node->indirect_pointer_block[0] == 0){
        add_data_block_to_node(file_node);
    }
    int bloqueInicio = offset / BLOCK_SIZE;
    int byteInicio = offset - (BLOCK_SIZE * (bloqueInicio));

    char* auxBuffer;
    t_block* bloqueDatos;
    int offsetRestante = offset;
    // Primero completo el offset con '\0' si corresponde, así puede escribir con el offset que se le cante
    t_data_position posUltimoByte;
    int cantNullsEscribir = 0;
    while (file_node->file_size < offsetRestante) {
        posUltimoByte = get_next_write_position(file_node);
        int cantNullsEscribir = min(offsetRestante, BLOCK_SIZE) - posUltimoByte.byte_num;

        // Asigno la cant de bytes necesarios al buffer, y lo lleno de /0
        auxBuffer = malloc(sizeof(char) * cantNullsEscribir);
        memset(auxBuffer, '\0', cantNullsEscribir);

        // Agarro el bloque para empezar a escribir y escribo los datos
        bloqueDatos = get_data_block_from_node(file_node, posUltimoByte.block_num);
        memcpy(&(bloqueDatos->bytes[posUltimoByte.byte_num]), auxBuffer, cantNullsEscribir);

        // Incremento el tamaño de archivo, descuento del offsetRestante y libero buffer
        file_node->file_size += cantNullsEscribir;
        offsetRestante -= cantNullsEscribir;
        free(auxBuffer);

        if ((posUltimoByte.byte_num + cantNullsEscribir) == BLOCK_SIZE && offsetRestante > 0) {
            // Si con la cantidad de nulls que escribí llegué a completar un bloque, y aún resta por escribir...
            // agrego un bloque de datos
            add_data_block_to_node(file_node);
        }
    }

    int fileSizeOriginal = file_node->file_size;

    while (cantBytesEscritos < count){
        // Escribe al final del archivo

        // Leo el bloque de datos entero
        t_block* data_block = get_data_block_from_node(file_node, bloqueInicio);

        // Se pueden escribir (BLOCK_SIZE - position.byte_num) bytes en el bloque (position.block_num);
        //espacioDisponible = BLOCK_SIZE - position.byte_num;
        espacioDisponible = BLOCK_SIZE - byteInicio;

        // Me quedo con el menor: lo pasado por count, o el espacio disponible
        cantBytesEscribir = ((count - cantBytesEscritos) > espacioDisponible) ? espacioDisponible : (count - cantBytesEscritos);

        // Escribo la nueva entrada en el bloque
        memcpy(&(data_block->bytes[byteInicio + cantBytesEscritosEnBloqueActual]), (buffer+cantBytesEscritos), cantBytesEscribir);

        _unmap_and_destroy_block(data_block);

        cantBytesEscritos += cantBytesEscribir;
        cantBytesEscritosEnBloqueActual += cantBytesEscribir;

        if ((cantBytesEscribir == espacioDisponible) && (cantBytesEscritos <= count)) {
            // Si el bloque se llenó  (bytes a escribir = espacio disponible) y
            // si aún falta escribir info (bytes escritos < count)
            add_data_block_to_node(file_node);
            bloqueInicio += 1;
            byteInicio = 0;
            cantBytesEscritosEnBloqueActual=0;
        }
        if ((offset + cantBytesEscritos) > fileSizeOriginal) {
            // Solo si con los bytes escritos escribí al menos 1 byte más que el file size tal y como era
            // antes de entrar al write, inremento el file size en el nodo
            //file_node->file_size += cantBytesEscribir;
            file_node->file_size = offset + cantBytesEscritos;
        }
    }
    unmap_and_destroy_node(&file_node, &file_node_block);

    return count;

}

size_t sac_read(int fd, void *buffer, size_t count, off_t offset){

    t_node* file_node;
    t_block* file_node_block;
    t_block* bloqueDatos;

    read_node(fd,&file_node,&file_node_block);

    if (file_node->state != 1) {
        // Si no es un archivo,se va
        unmap_and_destroy_node(&file_node, &file_node_block);
        return -1;
    }

    if (file_node->indirect_pointer_block[0] == 0){
        // Si no hay que leer, se va
        unmap_and_destroy_node(&file_node, &file_node_block);
        return -1;
    }

    int off = offset;
    int tam = file_node->file_size;
    if (off >= tam) {
        unmap_and_destroy_node(&file_node, &file_node_block);
        return 0;
    }

    int bloqueInicio = offset / BLOCK_SIZE;
    int byteInicio = offset - (BLOCK_SIZE * (bloqueInicio));
    int datosALeer = 0;

    // Leo el bloque de datos entero
    bloqueDatos = get_data_block_from_node(file_node, bloqueInicio);

    int datosLeidos=0;
    int bloquesLeidos = 0;

    if (count + offset > file_node->file_size) {
        // Si la cantidad de bytes a leer, sumado al offset pasado, es mayor que el tamaño de archivo
        count = file_node->file_size - offset;
    }

    while (datosLeidos < count){
        int MinimoEntre_BlockSize_FileSize = BLOCK_SIZE > file_node->file_size ? file_node->file_size : BLOCK_SIZE;
        datosALeer = ( (count - datosLeidos) > (MinimoEntre_BlockSize_FileSize - byteInicio) ) ? (MinimoEntre_BlockSize_FileSize - byteInicio) : (count - datosLeidos);
        // Luego copio el pedazo que necesito
        memcpy((buffer + datosLeidos),&(bloqueDatos->bytes[byteInicio]),datosALeer);
        datosLeidos += datosALeer;

        if (datosLeidos >= file_node->file_size){
            //memset(buffer + datosLeidos - 1,'\0',1);
            break;
        }

        _unmap_and_destroy_block(bloqueDatos);
        if (datosLeidos < count) {
            // Hay que leer otro bloque
            //free (bloqueDatos);
            bloquesLeidos += 1;
            bloqueDatos = get_data_block_from_node(file_node, bloqueInicio + bloquesLeidos);
            byteInicio = 0;
        }
    }
    unmap_and_destroy_node(&file_node, &file_node_block);
    return datosLeidos;
}

int sac_stat(char* path, struct stat *stbuf) {
    // Recupero el inodo del path, si no existe, retorno -1 (-ENOENT)
    if ( !is_valid_path(path) ) return -1;

    t_dir_entry entry = search_path(path);
    if ( !is_valid_entry(entry) ) return -1;

    t_node*  file_node;
    t_block* file_node_block;
    u_int32_t NodoInicial = NODE_TABLE_INITIAL_BLOCK;
    int numNodo = entry.node_num;// - NodoInicial;
    read_node(numNodo, &file_node, &file_node_block);

    // Lleno el stat con los datos del inodo
    switch (file_node->state) {
        case 1: // Es un archivo regular
            stbuf->st_mode = S_IFREG | 0777;
            break;
        case 2: // Es un directorio
            stbuf->st_mode = S_IFDIR | 0777;
            break;
        default: // El nodo o la entrada son inconsistentes
            unmap_and_destroy_node(&file_node, &file_node_block);
            return -1;
    }

    stbuf->st_ino         = numNodo;                      // node_num
    stbuf->st_size        = file_node->file_size;         // file_size
    stbuf->st_mtim.tv_sec = file_node->modification_date; // modification time
    stbuf->st_ctim.tv_sec = file_node->creation_date;     // creation

    unmap_and_destroy_node(&file_node, &file_node_block);
    return 0;
}

char** sac_readdir(char* path) {
    // Recupero el directorio del path, si no existe o no es un directorio, retorno NULL (-ENOENT)
    if ( !is_valid_path(path) ) return NULL;

    t_dir_entry dir_entry = search_path(path);
    t_node*  dir_node;
    t_block* dir_node_block;
    read_node(dir_entry.node_num, &dir_node, &dir_node_block);

    if ( !is_valid_entry(dir_entry) || dir_node->state != 2 ) {
        unmap_and_destroy_node(&dir_node, &dir_node_block);
        return NULL;
    }

    int number_of_entries = dir_node->file_size / DIR_ENTRY_SIZE; // Cantidad de entradas del directorio
    char** entry_names = malloc( sizeof(char*) * (number_of_entries + 1) );

    int i = 0;
    t_dir_entry load_entry(t_dir_entry entry) {
        t_dir_entry invalid_entry = { .name[0] = '\0' };
        entry_names[i] = malloc(strlen(entry.name) + 1);
        memcpy(entry_names[i], entry.name, strlen(entry.name) + 1);
        i++;
        return invalid_entry; // Retorna una estructura invalida para que continue iterando las demas entradas
    }
    iterate_pointers_blocks(dir_node, load_entry);

    entry_names[number_of_entries] = NULL;

    unmap_and_destroy_node(&dir_node, &dir_node_block);
    return entry_names;
}

int sac_rename(char* old_path, char* new_path) {
    // Verifico que el path sea valido
    if (!is_valid_path(old_path) || !is_valid_path(new_path)) return -1;

    // Obtengo el path de los directorios (old y new) que tienen las entradas
    char *old_file_name    = _get_new_dir_name_from_path(old_path);
    char *new_file_name    = _get_new_dir_name_from_path(new_path);
    char *old_pre_dir_path = _get_pre_dir_path_from_path(old_path, old_file_name);
    char *new_pre_dir_path = _get_pre_dir_path_from_path(new_path, new_file_name);

    // Obtener el inodo del directorio anterior y del nuevo
    t_dir_entry old_dir_entry = search_path(old_pre_dir_path);
    t_dir_entry new_dir_entry = search_path(new_pre_dir_path);
    free(old_pre_dir_path);
    free(new_pre_dir_path);

    // Verifico que encontro el directorio anterior y el nuevo
    if ( is_valid_entry(old_dir_entry) && is_valid_entry(new_dir_entry) ) {
        // Me traigo el nodo del directorio anterior
        t_node*  pre_dir_node;
        t_block* pre_dir_node_block;
        read_node(old_dir_entry.node_num, &pre_dir_node, &pre_dir_node_block);

        // Recupero la entrada del archivo a renombrar para guardar el nro de inodo y despues la borro
        t_dir_entry old_file_entry = get_entry(pre_dir_node, old_file_name);
        delete_directory_entry(pre_dir_node, old_file_entry.name);
        unmap_and_destroy_node(&pre_dir_node, &pre_dir_node_block);

        // Me traigo el nodo del nuevo directorio y le agrego la nueva entrada
        read_node(new_dir_entry.node_num, &pre_dir_node, &pre_dir_node_block);
        add_directory_entry(pre_dir_node, new_file_name, old_file_entry.node_num);
        unmap_and_destroy_node(&pre_dir_node, &pre_dir_node_block);

        // Me traigo el nodo del archivo y le actualizo el padre
        t_node*  file_node;
        t_block* file_node_block;
        read_node(old_file_entry.node_num, &file_node, &file_node_block);
        file_node->parent_block = new_dir_entry.node_num + NODE_TABLE_INITIAL_BLOCK;
        unmap_and_destroy_node(&file_node, &file_node_block);

        free(old_file_name);
        free(new_file_name);
    } else {
        free(old_file_name);
        free(new_file_name);
        return -1;
    }

    return 0;
}

int sac_remove(char* path){
    if ( !is_valid_path(path) ) return -1;

    t_dir_entry entry = search_path(path);
    if ( !is_valid_entry(entry) ) return -1;

    t_node*  file_node;
    t_block* file_node_block;
    read_node(entry.node_num, &file_node, &file_node_block);
    if (file_node->state != 1) {
        // Si no es un archivo,se va
        unmap_and_destroy_node(&file_node, &file_node_block);
        return -1;
    }

    // Es un archivo, y está vigente. Lo puedo eliminar.
    // Tengo que liberar en el bitmap los bloques de datos asociados al archivo
    // Liberar los bloques de punteros
    // Liberar el nodo

    int cantBloques = (file_node->file_size / BLOCK_SIZE)+1;
    int i;
    int j;

    char* numBloque = malloc(sizeof(u_int32_t));
    for (i=0; i<1000 && file_node->indirect_pointer_block[i] != 0; i++) {
        t_block *bloquePunteros = read_block(disk_fd, file_node->indirect_pointer_block[i]);
        for (j = 0; j < 1024 && cantBloques > 0; j++) {
            memcpy(numBloque, &(bloquePunteros->bytes[j * sizeof(u_int32_t)]), sizeof(u_int32_t));
            bitarray_clean_bit(bitmap, _bytes_to_int32(numBloque));
            cantBloques--;
        }
        _unmap_and_destroy_block(bloquePunteros);
        bitarray_clean_bit(bitmap, file_node->indirect_pointer_block[i]);
        //free(bloquePunteros);
    }
    free (numBloque);

    // Leo el nodo padre
    t_node* nodoPadre;
    t_block* bloquePadre = read_block(disk_fd,file_node->parent_block);
    nodoPadre = block_to_node(*bloquePadre);

    // Elimino el directory entry del nodo padre
    char* nombreArchivo = getFileNameFromFilePath(path);
    delete_directory_entry(nodoPadre, nombreArchivo);
    unmap_and_destroy_node(&nodoPadre,&bloquePadre);

    file_node->state = 0;
    u_int32_t numBloqueNodo = get_block_num_from_node_num(entry.node_num);
    unmap_and_destroy_node(&file_node, &file_node_block);
    bitarray_clean_bit(bitmap, numBloqueNodo);
    free(nombreArchivo);
    return 0;
}

int sac_close(int fd){
    return (fd >= NODE_TABLE_INITIAL_BLOCK && fd <= NODE_TABLE_INITIAL_BLOCK + NODE_TABLE_BLOCKS);
}
int sac_truncate(const char* path, off_t lenght) {
    printf("Entra a truncate. Path: %s, lenght: %d\n", path, lenght);

    if (lenght < 0) {
        printf("Esta truncando a menos de 0 bytes. Para probar, le paso a manopla un 0.\n");
        lenght=0;
        //return 0;
    }

    if (!is_valid_path(path)) return -1;

    t_dir_entry entry = search_path(path);
    if (!is_valid_entry(entry)) return -1;

    t_node *file_node;
    t_block *file_node_block;
    read_node(entry.node_num, &file_node, &file_node_block);
    if (file_node->state != 1) {
        // Si no es un archivo,se va
        unmap_and_destroy_node(&file_node, &file_node_block);
        return -1;
    }

    int cantBytesATruncar = 0;
    int tamanoArchivo = file_node->file_size;
    int largoTruncado = lenght;
    int tamanoBloque = BLOCK_SIZE;
    char* buffer;
    t_data_position posUltimoByte = get_next_write_position(file_node);

    //******************************** SI EL TRUNCATE ES PARA ALARGAR EL ARCHIVO ********************************
    if (largoTruncado > tamanoArchivo) {
        printf("TRUNCATE para alargar. El archivo pesa %d y quiero truncarlo a %d\n", tamanoArchivo,largoTruncado);
        // Si el truncate quiere hacer crecer al archivo
        buffer = malloc(sizeof(char) * (largoTruncado - tamanoArchivo));
        memset(buffer,'A',(largoTruncado - tamanoArchivo));
        // Completa el bloque con /0. Arranca con offset=filesize
        sac_write(sac_open(path,0),buffer,(largoTruncado - tamanoArchivo), file_node->file_size);
        free(buffer);
        return (0);
    }

    //******************************** SI EL TRUNCATE ERA PARA ACHICAR EL ARCHIVO ********************************
    printf("TRUNCATE para acortar. El archivo pesa %d y quiero truncarlo a %d\n", tamanoArchivo,largoTruncado);
    while ((tamanoArchivo > largoTruncado) && tamanoArchivo > 0) {
	//Calculo bytes a truncar
        cantBytesATruncar = tamanoArchivo - largoTruncado;

        printf("Vuelta de truncate. CantBytesTruncar: %d (Tamanio archivo: %d, bytes a truncar: %d)\n", cantBytesATruncar, tamanoArchivo, cantBytesATruncar);

        if (cantBytesATruncar >= tamanoBloque || largoTruncado == 0) {
            // Si la diferencia entre el tamaño actual del archivo y el valor final
            // a truncar es mayor que el tamaño de bloque, puedo sacar el último bloque del archivo.
            deleteLastDataBlock(file_node);

        } else if (largoTruncado > 0) {
            // Si tiene que truncar Menos_que_un_bloque bytes, entonces solo modifica el file size
            file_node->file_size = file_node->file_size - cantBytesATruncar;
        }
        tamanoArchivo = file_node->file_size;
    }
    unmap_and_destroy_node(&file_node,&file_node_block);
    return (0);
}