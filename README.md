# Linuse File System - Sistemas Operativos

Este módulo nos permitirá implementar nuestro propio almacenamiento de archivos, que almacene
los datos de forma centralizada. A diferencia de los otros dos módulos, este módulo no poseerá una
biblioteca, sino que vamos a utilizar directamente un Sistema de archivos (o File System).

Los File Systems son la forma que definen los Sistemas Operativos para poder gestionar los datos
guardados en un medio. Si bien hay varios de miles de implementaciones de File Systems, todos
suelen respetar una interfaz en común mediante llamadas al sistema.

El problema que tiene esta interfaz, es que las implementaciones de File Systems se ejecutan en
espacio de Kernel, lo que provoca que cualquier error en la implementación de ese File System,
afecte al Sistema Operativo entero (ni hablar que interactuar directamente con el SO suele ser
bastante tedioso).

Tratando de solucionar un poco ese problema, utilizaremos un framework llamado FUSE o
FileSystem in Userspace, que actúa como una suerte de “traductor” entre el Sistema Operativo y
nuestra implementación de un File System, la cual ejecuta en espacio de usuario.
Es decir, cuando el SO se encuentre en la necesidad de guardar o leer información, este se lo
comunicará a FUSE y buscará dentro de las operaciones que el programador definió, cómo hacerlo.
Entre los submódulos, será el SAC-cli el que implemente el framework FUSE; comunicándose con el
proceso de almacenamiento central o SAC-server.

![image](https://user-images.githubusercontent.com/31081698/146769030-9bb57f70-b95e-4f73-a87a-4a93a04932cc.png)

De esta forma, cada uno de los SAC-cli funcionará como un adaptador entre las llamadas al sistema
que hagan los programas que usen el módulo y el SAC-servidor, redirigiendo a él todas las consultas
referidas a los archivos, directorios y metadata que posea.
Por lo tanto, cada grupo deberá implementar todas las funciones de FUSE que sean necesarias para proveer un correcto funcionamiento del FileSystem.

## ¿Cómo funciona FUSE?

Cuando un proceso llama a una función de I/O (ejemplo, un ls en la consola), se termina traduciendo
a una (o más) llamadas al sistema. Estas syscalls son capturadas por el Kernel y delegada al VFS
(Virtual FileSystem).

Si bien lo mencionamos antes sin nombrarlo, el VFS es esa capa genérica a todas las posibles
implementaciones de FileSystem que el Kernel pueda tener.

El VFS se encarga de determinar (basándose en el path y los puntos de montaje) la implementación
que le corresponde resolver la llamada al sistema. En este caso, la implementación de FUSE, quien, al
recibir la orden, la delega al proceso que esté encargado de resolverla.

Internamente la biblioteca de FUSE trabaja con un mecanismo multi-thread, es decir, a cada
operación se le asigna un nuevo thread de ejecución. Este mecanismo multi-thread puede ser
deshabilitado pasándole como parámetro a FUSE ”-s”, opción que simplifica la tarea de debuguear
(sin embargo, se remarca que el TP será evaluado en un entorno multithread).

Otro de los mecanismos internos que posee FUSE es una caché interna propia. En la mayoría de las
condiciones estas traen beneficios en cuanto a evitar accesos de I/O. Para el caso del TP esta caché
debe estar deshabilitada.

Para anular la caché que posee la biblioteca de FUSE basta con pasar como parámetro la opción: -o
direct_io.

![image](https://user-images.githubusercontent.com/31081698/146769548-fcbdd908-ae08-4045-bfc9-1728385ffda3.png)

## SAC-servidor

Este proceso gestionará un Filesystem almacenado en un archivo binario que será leído e
interpretado como un árbol de directorios y sus archivos. El filesystem presentado será central a
todos los programas que se ejecuten dentro de nuestro sistema, permitiéndole a cada uno realizar
las siguientes operaciones:

- Crear, escribir, leer y borrar archivos.
- Crear y listar directorios y sus archivos.
- Eliminar directorios.
- Describir directorios y archivos.

SAC-Server debe poder realizar las distintas operaciones solicitadas por los programas/hilos en
paralelo.

### Características del file system

Generales:
- Tamaño máximo de disco soportado: 16 TB
- Tamaño máximo de archivo 3.9GB
- Soporte de directorios y subdirectorios
- Tamaño máximo de nombre de archivo: 71 caracteres
- Cantidad máxima de archivos: 1024

Técnicas:
- Tamaño de bloque: Fijo (4096 bytes)
- Tamaño de la tabla de nodos: 1024 bloques (4 MB)
- Tabla de bloques libres: Bitmap

## Especificación del file system
Cada bloque en el sistema de archivos estará direccionado por un puntero de 4 bytes llamado
ptrGBloque permitiendo así un máximo de 2^32 bloques.

Para un disco de tamaño T [bytes] y el BLOCK_SIZE de 4096 las estructuras se definen de esta
manera:

![image](https://user-images.githubusercontent.com/31081698/146772569-b89ebe6c-c152-4641-9451-5d97a01f10b6.png)

### Header
El encabezado del sistema de archivos estará almacenado en el primer bloque. Contará con los siguientes campos:

![image](https://user-images.githubusercontent.com/31081698/146772894-f6921eba-6618-480a-85b8-78274b6ac9d4.png)

### Bitmap
El Bitmap, también conocido como bitvector, es un array de bits en el que cada bit identifica a un bloque a partir 
de su posición y nos permite conocer su estado, es decir, si se encuentra ocupado (1)
o no (0). Recordar que las implementaciones a nivel bit suelen ser susceptibles al Endianness (o byteordering). 
Por ejemplo: un bitmap con este valor 000001010100 nos indicaría que los bloques 5, 7 y 9 están ocupados 
y los restantes libres (recordar que la primer ubicación es la 0).

Cuando el filesystem necesite localizar un bloque libre para la escritura de datos simplemente
utilizará esta estructura para encontrarlo, marcándolo como ocupado una vez que lo utilizó.
Es importante recalcar que los bloques utilizados por las estructuras administrativas deben siempre
estar marcados como utilizados.

### Tabla de Nodos
La tabla de nodos de SAC es de tamaño fijo: un array de 1024 posiciones de estructuras de tipo GFile.
Cada nodo consta de los siguientes campos:

![image](https://user-images.githubusercontent.com/31081698/146773057-dcd686e3-ead3-40e5-87a5-b17b63544b62.png)
