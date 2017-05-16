#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tar.h"

int main(int argc, char *argv[]){

    if (argc == 1){
        printf("No se encontró comando a ejecutar\nc");
        return 0;
    }

    char command[100];
    strcpy(command,argv[1]);
    char verbosity = 0;



    char * filename = argv[2];
    const char ** files = (const  char **) &argv[3];

    struct tar_t  * tarFile = NULL;
    int fd = -1;

    if (strcmp(command,"-c")==0){
        printf("Crear archivo\n");
        fd = open(filename,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd==-1){
            printf("Error no se pudo crear el archivo %s\n",filename);
        }
        else{
            if (tar_write(fd, &tarFile, argc-3, files, verbosity) < 0){
                printf("Parece que hubo un problema al crear el archivo\n");
            }
        }

    }
    if (strcmp(command,"-x")==0){
        printf("Extraer archivos \n");
    }
    if (strcmp(command,"-t")==0){
        printf("Listar contenido del archivo \n");
    }
    if (strcmp(command,"-delete")==0){
        printf("Borrar contenido de un archivo \n");
    }
    if (strcmp(command,"-u")==0){
        printf("Actualizar contenido \n");
    }
    if (strcmp(command,"-v")==0){
        printf("Ver reporte\n");
    }
    if (strcmp(command,"-f")==0){
        printf("Empaquetar contenido de un archuvo \n");
    }
    if (strcmp(command,"-r")==0){
        printf("Agregar contenido a un archivo\n");
    }




}

