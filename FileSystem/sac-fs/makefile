COMPILER=gcc
BIN_PATH=bin
CFLAGS=-Wall
LIBS = -lcommons -lpthread

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

sac:
	mkdir -p $(BIN_PATH)
	$(COMPILER) $(CFLAGS) src/*.h src/*.c ../mtserver/src/*.h ../mtserver/src/*.c $(LIBS) -o $(BIN_PATH)/sac

clean:
	rm -rf ./bin
#	sudo rm -f /usr/lib/libcommons.so
#	sudo rm -rf /usr/include/commons
	@tput setaf 2
	@printf "\nDesinstalacion finalizada\n\n"
	@tput sgr0
