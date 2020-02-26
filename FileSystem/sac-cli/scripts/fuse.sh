#!/bin/bash

# Se ubica en el directorio del script
cd "$(dirname "$0")"

# Compila FUSE
cd ..
make fuse

# Ejecuta FUSE
#
# -f: Desactiva la ejecución en modo background
# -s: La biblioteca de FUSE se ejecuta en modo single thread
# -d: Imprime información de debug
# -o direct_io: Anula la caché que posee la biblioteca de FUSE
#
# $1, $2 son parametros que recibe el script: 
# Para cambiar IP : --ip "127.0.0.1"
#
cd ./bin
mkdir -p ./tmp
./fuse ./tmp $1 $2 $3 $4 -f -d -o direct_io
