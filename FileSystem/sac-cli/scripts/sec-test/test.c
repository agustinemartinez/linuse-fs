#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define FUSE_MNT_DIR "./../bin/tmp" // El path varia segun donde se ejecuta. Dentro del script se hace un cd a la carpeta /scripts
#define FILENAME (FUSE_MNT_DIR "/tmpfile.txt")
#define DIRNAME (FUSE_MNT_DIR "/test_dir")
#define OPENMODE O_RDWR|O_CREAT
#define DIRMODE 777

int main(void) {
	int res;
	char* esto_se_escribe_en_el_archivo = "Esto-se-escribe-en-el-archivo";
	size_t size_esto_se_escribe_en_el_archivo = strlen(esto_se_escribe_en_el_archivo) + 1;
	char buffer[size_esto_se_escribe_en_el_archivo];

	printf("\n	[ TEST FUSE ]\n");
	printf("\n FUSE_MNT_DIR: %s \n\n", FUSE_MNT_DIR);
	getchar();
		int fd = open(FILENAME, OPENMODE);
	printf("open(%s)   :: fd %d\n", FILENAME, fd);
	getchar();
		res = write(fd, esto_se_escribe_en_el_archivo, size_esto_se_escribe_en_el_archivo);
	printf("write()  :: caracteres_escritos: %d de %d\n", res, size_esto_se_escribe_en_el_archivo);
	getchar();
		res = close(fd);
	printf("close()  :: status: %d\n", res);
	getchar();
		fd = open(FILENAME, OPENMODE);
	printf("open()   :: fd %d\n", fd);
	getchar();
		read(fd, buffer, size_esto_se_escribe_en_el_archivo);
	printf("read()   :: size_leido: %li, info_leida: %s\n", (long)size_esto_se_escribe_en_el_archivo, (char*)buffer);
	getchar();
		res = close(fd);
	printf("close()  :: status: %d\n", res);
	getchar();
		res = remove(FILENAME);
	printf("remove() :: status: %d\n", res);
	getchar();
		res = mkdir(DIRNAME, DIRMODE);
	printf("mkdir(%s)  :: status: %d\n", DIRNAME, res);
	getchar();
		res = rmdir(DIRNAME);
	printf("rmdir()  :: status: %d\n", res);
	getchar();

	return 0;
}