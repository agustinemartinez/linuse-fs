#!/bin/bash

# Se ubica en el directorio del script
cd "$(dirname "$0")"

# Si no existe, crea la carpeta /bin
mkdir -p ../bin

# Compila el archivo de ejemplo
#cd ~/workspace/tp-2019-2c-IfRecursoThenUADE/Linuse/FileSystem/sac-cli/bin
gcc ./sec-test/test.c -o ../bin/test

# Inicia FUSE en otra terminal
terminator -x ./fuse.sh &

# Ejecuta el archivo de ejemplo
./../bin/test

# Finaliza FUSE
killall fuse

# Elimina los ejecutables creados
rm ../bin/test
rm ../bin/fuse
