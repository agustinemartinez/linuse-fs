#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <commons/string.h>
#include "client.h"
#include "log.h"
#include "fuse.h"

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los interpreta
	if ( fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1 ) {
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Si se paso el parametro --ip:port el campo ip deberia tener el valor pasado
	//ip = (runtime_options.optional_ip != NULL) ? runtime_options.optional_ip : LOCAL_IP;
	//port = DEFAULT_PORT;

    char** ip_and_port = get_ip_and_port_or_default(runtime_options.optional_ip);
	ip   = ip_and_port[0];
	port = ip_and_port[1];

	// Inicio el archivo de log	
	init_logger();
	log_init_file_system();
	log_connection_info(ip, port);

	// Esta es la funcion principal de FUSE, es la que se encarga de realizar el montaje, 
	// comuniscarse con el kernel, delegar todo en varios threads
	return fuse_main(args.argc, args.argv, &do_oper, NULL);
}


/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para tratar de abrir un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		fi - es una estructura que contiene la metadata del archivo indicado en el path
 *
 * 	@RETURN
 * 		O archivo fue encontrado. -EACCES archivo no es accesible
 */
static int do_open(const char *path, struct fuse_file_info *fi) {

    log_trace(g_logger, "[ OPEN ] :: path=%s", path);

	if ( !_correct_path(path) ) return 0;

	// TODO: Sacar flags por defecto
	int flags = O_RDWR|O_CREAT;//fi->flags & 3; // fi->flags;
    t_package* response = send_function(IDENTIFIER_OPEN, (char*)path, NULL, flags, 0, 0);
    int fd = response->message_id;
	destroy_package(&response);
	
	log_trace(g_logger, "[ OPEN ] :: path=%s, result=%li", path, fd);

	if (fd < 0) return -EACCES;
	else fi->fh = fd;

	return 0;
}

/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener el contenido de un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es el buffer donde se va a guardar el contenido solicitado
 * 		size - Nos indica cuanto tenemos que leer
 * 		offset - A partir de que posicion del archivo tenemos que leer
 *
 * 	@RETURN
 * 		Si se usa el parametro direct_io los valores de retorno son 0 si  elarchivo fue encontrado
 * 		o -ENOENT si ocurrio un error. Si el parametro direct_io no esta presente se retorna
 * 		la cantidad de bytes leidos o -ENOENT si ocurrio un error. ( Este comportamiento es igual
 * 		para la funcion write )
 */
static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

    log_trace(g_logger, "[ READ ] :: path=%s, size=%u, offset=%u", path, size, offset);

    if ( !_correct_path(path) ) return 0;

	int fd = fi->fh;
	t_package* response = send_function(IDENTIFIER_READ, NULL, NULL, fd, size, offset);
	memcpy(buffer, response->data, response->size);
	
	int read_size;
	if (response->message_id == 0) read_size = response->size;
	else read_size = response->message_id;

	log_trace(g_logger, "[ READ ] :: path=%s, size=%u, offset=%u, result=%li", path, size, offset, read_size);

	destroy_package(&response);

	return read_size;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

    log_trace(g_logger, "[ WRITE ] :: path=%s, size=%u, offset=%u", path, size, offset);

    int fd = fi->fh;
    t_package* response = send_function(IDENTIFIER_WRITE, (char*)buffer, NULL, fd, size, offset);
	int res = response->message_id;
	destroy_package(&response);
	
	log_trace(g_logger, "[ WRITE ] :: path=%s, size=%u, offset=%u, result=%li", path, size, offset, res);

	return res;
}

/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la metadata de un archivo/directorio. Esto puede ser tamaño, tipo,
 * permisos, dueño, etc ...
 * Esta funcion siempre llega antes de un open(), read(), mkdir(), rmdir()
 * Si su retorno no es el esperado, FUSE no llamara a la siguiente funcion (open, read, etc.)
 * 
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		stbuf - Esta esta estructura es la que debemos completar
 *
 * 	@RETURN
 * 		O archivo/directorio fue encontrado. -ENOENT archivo/directorio no encontrado
 */
static int do_getattr(const char *path, struct stat *stbuf) {

    log_trace(g_logger, "[ GETATTR ] :: path=%s", path);

    if ( !_correct_path(path) ) return 0;

    t_package* response = send_function(IDENTIFIER_STAT, (char*)path, NULL, 0, 0, 0);
	int status = response->message_id;

	log_trace(g_logger, "[ GETATTR ] :: path=%s, result=%li", path, status);

	// Si no existe el directorio (status = -1), se debe retornar ENOENT para que despues se pueda hacer mkdir()
	if (status == 0) {
		__fill_stat_from_buffer(response->data, stbuf);
		destroy_package(&response);
		return 0;
	} else { 
		destroy_package(&response);
		return -ENOENT;
	}
}

/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la lista de archivos o directorios que se encuentra dentro de un directorio
 * Con esta funcion se carga la lista de entradas del directorio cuando se hace ls
 * 
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es un buffer donde se colocaran los nombres de los archivos y directorios
 * 		      que esten dentro del directorio indicado por el path
 * 		filler - Este es un puntero a una función, la cual sabe como guardar una cadena dentro
 * 		         del campo buf
 *
 * 	@RETURN
 * 		O directorio fue encontrado. -ENOENT directorio no encontrado
 */
static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

    log_trace(g_logger, "[ READDIR ] :: path=%s", path);

    // "." y ".." son entradas validas, la primera es una referencia al
	// directorio donde estamos parados y la segunda indica el directorio padre
	filler(buf, "." , NULL, 0);
	filler(buf, "..", NULL, 0);

    t_package* response = send_function(IDENTIFIER_READDIR, (char*)path, NULL, 0, 0, 0);

    if (response->message_id < 0) {
        destroy_package(&response);
        return -ENOENT;
    }

	size_string_len len = 0;
	for ( int size = 0 ; size < response->size ; size += len ) {
		memcpy(&len, response->data + size, sizeof(len));
		char* entry = malloc(len * sizeof(char));
		size += sizeof(len);
		memcpy(entry, response->data + size, len);
		filler(buf, entry, NULL, 0);
		free(entry);
	}	
	log_trace(g_logger, "[ READDIR ] :: path=%s, result=%li", path, 0l);
	destroy_package(&response);

	return 0;
}

static int do_remove(const char *path) {

    log_trace(g_logger, "[ REMOVE ] :: path=%s", path);

    t_package* response = send_function(IDENTIFIER_REMOVE, (char*)path, NULL, 0, 0, 0);
	int res = response->message_id;
	log_trace(g_logger, "[ REMOVE ] :: path=%s, result=%li", path, res);
	destroy_package(&response);

	return res;
}

// Ante un close() se puede llamar a la funcion flush o release
// En el close() hay que implementar el caso de que el fd no exista
// y retornar 0 igual
static int do_flush(const char *path, struct fuse_file_info *fi) { 
	log_trace(g_logger, "[ FLUSH ] :: path=%s", path);
	return 0;
}

// Ante un close() se puede llamar a la funcion flush o release
// En el close() hay que implementar el caso de que el fd no exista
// y retornar 0 igual
static int do_release(const char *path, struct fuse_file_info *fi) {

    log_trace(g_logger, "[ RELEASE ] :: path=%s", path);

    if ( !_correct_path(path) ) return 0;

	int fd = fi->fh;
    t_package* response = send_function(IDENTIFIER_CLOSE, NULL, NULL, fd, 0, 0);
	int res = response->message_id;
	destroy_package(&response);
	log_trace(g_logger, "[ RELEASE ] :: path=%s, result=%li", path, res);

	return res;
}

static int do_mkdir(const char* path, mode_t mode) {

    log_trace(g_logger, "[ MKDIR ] :: path=%s", path);

    t_package* response = send_function(IDENTIFIER_MKDIR, (char*)path, NULL, mode, 0, 0);
	int res = response->message_id;
	destroy_package(&response);
	log_trace(g_logger, "[ MKDIR ] :: path=%s, result=%li", path, res);
	return res;
}

static int do_rmdir(const char* path) {

    log_trace(g_logger, "[ RMDIR ] :: path=%s", path);

    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) return -1;

    t_package* response = send_function(IDENTIFIER_RMDIR, (char*)path, NULL, 0, 0, 0);
	int res = response->message_id;
	destroy_package(&response);
	log_trace(g_logger, "[ RMDIR ] :: path=%s, result=%li", path, res);

	if (res == -2) return -ENOTEMPTY;

	return res;
}

// Ante un open, FUSE verifica los flags
// Si tiene el flag O_CREAT, en lugar de llamar a open(), va a llamar a create()
// Tanto el create o el open tienen que cargar el fd en la estructura fuse_file_info 
static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

    log_trace(g_logger, "[ CREATE ] :: path=%s", path);

    int flags = O_CREAT|O_WRONLY|O_TRUNC; //O_RDWR|O_CREAT;
	t_package* response = send_function(IDENTIFIER_OPEN, (char*)path, NULL, flags, 0, 0); // Abro el archivo para crearlo
	int fd = response->message_id;
	destroy_package(&response);
	log_trace(g_logger, "[ CREATE ] :: path=%s, fd=%li", path, fd);

	if (fd < 0) return -EACCES;
	else fi->fh = fd;

	return 0;
}


static int do_opendir(const char* path, struct fuse_file_info* fi) {
	log_trace(g_logger, "[ OPENDIR ] :: path=%s", path);
	return 0;
}

static int do_truncate(const char* path, off_t size) {
	log_trace(g_logger, "[ TRUNCATE ] :: path=%s, size=%u", path, size);

    t_package* response = send_function(IDENTIFIER_TRUNCATE, (char*)path, NULL, size, 0, 0);
    int res = response->message_id;
    destroy_package(&response);
    log_trace(g_logger, "[ TRUNCATE ] :: path=%s, size=%li, result=%d", path, size, res);

    return res;//0;
}

static int do_rename(const char* old_path, const char* new_path) {

    log_trace(g_logger, "[ RENAME ] :: old_path=%s, new_path=%s", old_path, new_path);

    t_package* response = send_function(IDENTIFIER_RENAME, (char*)old_path, (char*)new_path, 0, 0, 0);
	int res = response->message_id;
	destroy_package(&response);
	log_trace(g_logger, "[ RENAME ] :: old_path=%s, new_path=%s, result=%li", old_path, new_path, res);
	return res;
}

static int do_access (const char* path, int mode) {
    log_trace(g_logger, "[ ACCESS ] :: path=%s", path);
    return 0;
}

static char** get_ip_and_port_or_default(char* ip_port) {
    if (ip_port != NULL) {
        if ( !string_contains(ip_port, ":") ) {
            printf("\n\t[ Invalid arguments ] :: --ip IP:PORT\n\n");
            exit(-1);
        }
        return string_split(ip_port,":");
    }
    char** args = malloc(sizeof(char*) * 2);
    args[0] = LOCAL_IP;
    args[1] = DEFAULT_PORT;
    return args;
}

static int _correct_path(const char* path) {
	return !string_starts_with((char*)path,"/.") && strcmp((char*)path, "/autorun.inf") != 0;
}

static void __fill_stat_from_buffer(void* buffer, struct stat *stbuf) {
	int16_t is_dir_or_file;
	u_int16_t node_num;
	u_int32_t size;
	u_int64_t mtime, ctime;

	memcpy(&is_dir_or_file, buffer, sizeof(is_dir_or_file));
	memcpy(&node_num, buffer + sizeof(is_dir_or_file), sizeof(node_num));
	memcpy(&size,  buffer + sizeof(is_dir_or_file) + sizeof(node_num), sizeof(size));
	memcpy(&mtime, buffer + sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(size), sizeof(mtime));
	memcpy(&ctime, buffer + sizeof(is_dir_or_file) + sizeof(node_num) + sizeof(size) + sizeof(mtime), sizeof(ctime));

	// Llena los campos segun sea un directorio o un archivo
	if (is_dir_or_file == 2) {
		stbuf->st_mode = S_IFDIR | 0777;   // S_IFDIR = directorio
		stbuf->st_nlink = 2; 			   // hardlinks
	} else { 				   
		stbuf->st_mode = S_IFREG | 0777;   // S_IFREG = archivo regular
		stbuf->st_nlink = 1;               // hardlinks
	}
	stbuf->st_uid   = getuid();
	stbuf->st_gid   = getgid();
	stbuf->st_ino   = node_num;		       // inodo
	stbuf->st_size  = size;                // size archivo
	stbuf->st_mtime = mtime;   	           // time last modification
   	stbuf->st_ctime = ctime;   	           // time last status change
	stbuf->st_atime = stbuf->st_mtime;     // time last access
}
