/*
 ESTE DOCUMENTO ESTA HECHO PARA VERLO CON TAB SIZE = 4
 */

#include <string.h>			// strtok()
#include <err.h>			// warnx()
#include "defs.h"
#include "builtins.h"
#include "entorno_lib.h"	// EVasignar(), EVexportar(), EVimprimir()
#include <stdio.h>			// fprintf()


void asignar(int argc, char *argv[]) {
	/* Tratamiento del comando interno: set var [valor] */
	
	if (argc != 3) fprintf(stderr,"Uso: set [nombre] [valor]\n");
	else EVasignar(argv[1], argv[2]);
}

void exportar(int argc, char *argv[]) {
	/* Tratamiento del comando interno: export [var] */
	
	if (argc != 2) fprintf(stderr,"Uso: export [var]\n");		
	else EVexportar(argv[1]);
}

void entorno(int argc, char *argv[]) {
	/* Tratamiento del comando interno: env */

	EVmostrar();
}
