#ifndef FILESYSTEM_FUSE_H
#define FILESYSTEM_FUSE_H

#include <stddef.h>
#include <fuse.h>

// Esta es una estructura auxiliar utilizada para almacenar parametros
// que nosotros le pasemos por linea de comando a la funcion principal de FUSE
struct t_runtime_options {
	char* optional_ip;
} runtime_options;

// Esta Macro sirve para definir nuestros propios parametros que queremos que FUSE interprete. 
// Esta va a ser utilizada mas abajo para completar el campos welcome_msg de la variable runtime_options
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }


static int do_getattr (const char *path, struct stat *stbuf);
static int do_open    (const char *path, struct fuse_file_info *fi);
static int do_flush   (const char *path, struct fuse_file_info *fi);
static int do_release (const char *path, struct fuse_file_info *fi);
static int do_truncate(const char* path, off_t size);
static int do_write   (const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
static int do_read    (const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
static int do_rename  (const char* old_path, const char* new_path);
static int do_remove  (const char *path);
static int do_mkdir   (const char* path, mode_t mode);
static int do_rmdir   (const char* path);
static int do_opendir (const char* path, struct fuse_file_info* fi);
static int do_readdir (const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int do_create  (const char *path, mode_t mode, struct fuse_file_info *fi);
static int do_access  (const char* path, int mode);
//static int do_mknod   (const char* path, mode_t mode, dev_t rdev);

static char** get_ip_and_port_or_default(char* ip_port);
static int _correct_path(const char* path);
static void __fill_stat_from_buffer(void* buffer, struct stat *stbuf);


// Esta es la estructura principal de FUSE con la cual nosotros le decimos a
// biblioteca que funciones tiene que invocar segun que se le pida a FUSE.
// Como se observa la estructura contiene punteros a funciones.
static struct fuse_operations do_oper = {
		.getattr  = do_getattr,   // STAT
		.open     = do_open,      // OPEN
		.create   = do_create,    // OPEN
		.truncate = do_truncate,  // OPEN
		.flush    = do_flush,     // CLOSE
		.release  = do_release,   // CLOSE
		.rename   = do_rename,    // MV
		.read     = do_read,      // READ
		.write    = do_write,     // WRITE
		.unlink   = do_remove,    // REMOVE
		.mkdir    = do_mkdir,     // MKDIR
		.rmdir    = do_rmdir,     // RMDIR
		.opendir  = do_opendir,   // LS
		.readdir  = do_readdir,   // LS
		.access   = do_access,    // ?
		//.mknod   = do_mknod,		// ?
};


// keys for FUSE_OPT_ options
enum {
	KEY_VERSION,
	KEY_HELP,
};


// Esta estructura es utilizada para decirle a la biblioteca de FUSE que
// parametro puede recibir y donde tiene que guardar el valor de estos
static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--ip %s", optional_ip, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V"       , KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h"       , KEY_HELP),
		FUSE_OPT_KEY("--help"   , KEY_HELP),
		FUSE_OPT_END,
};

#endif //FILESYSTEM_FUSE_H
