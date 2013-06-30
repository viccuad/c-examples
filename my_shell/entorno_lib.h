#ifndef ENTORNO_LIB_H
#define ENTORNO_LIB_H 1

#include "defs.h"	// tipo BOOL

#define MAXVAR 50


// struct que contiene todas las variables de entorno de nuestra shell
static struct varslot {
	char *nombre, *valor;
	BOOL exportado;
} simbolos[MAXVAR];


//extern char **environ;
BOOL EVasignar(char *nombre, char *valor);
BOOL EVexportar(char *nombre);
char *EVobtener(char *nombre);
BOOL EViniciar(char **envp);
BOOL EVactualizar(void);
void EVmostrar(void);

#endif /* ENTORNO_LIB_H */
