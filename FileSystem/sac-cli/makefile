COMPILER=gcc
BIN_PATH=bin
CFLAGS=-Wall
FUSEFLAGS=-DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64
LIBS = -lcommons -lpthread -lfuse

#.PHONY: lfs

all: 
#	make so-commons-library 
#	make readline 
	@tput setaf 2
	@printf "\nInstalacion finalizada\n\n"
	@tput sgr0

so-commons-library:
	git clone https://github.com/sisoputnfrba/so-commons-library
	cd ./so-commons-library && sudo make install
	sudo rm -rf ./so-commons-library
	@tput setaf 2
	@printf "\n[ Commons Instalada ]\n\n"
	@tput sgr0

cli:
	mkdir -p $(BIN_PATH)
	$(COMPILER) $(CFLAGS) src/client.h src/client.c ../mtserver/src/serialization.h ../mtserver/src/serialization.c ../mtserver/src/utils.h ../mtserver/src/utils.c $(LIBS) -o $(BIN_PATH)/sac-cli

fuse:
	mkdir -p $(BIN_PATH)
	$(COMPILER) $(CFLAGS) $(FUSEFLAGS) src/*.h src/*.c ../mtserver/src/serialization.h ../mtserver/src/serialization.c $(LIBS) -o $(BIN_PATH)/fuse #../mtserver/src/utils.h ../mtserver/src/utils.c $(LIBS) -o $(BIN_PATH)/fuse

clean:
	rm -rf ./bin
#	sudo rm -f /usr/lib/libcommons.so
#	sudo rm -rf /usr/include/commons
	@tput setaf 2
	@printf "\nDesinstalacion finalizada\n\n"
	@tput sgr0
