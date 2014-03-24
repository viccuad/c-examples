/*
 ESTE DOCUMENTO ESTA HECHO PARA VERLO CON TAB SIZE = 4
 */

#include <stdlib.h>		// malloc(), realloc
#include <stdio.h>		// printf
#include <string.h>		// strlen, strcpy, strcmp
#include <unistd.h>		// environ
#include "entorno_lib.h"// contiene el struct varslot simbolos[MAXVAR]

extern char **environ;


/**
 * usar como funcion "privada"
 * recorre el struct varslot simbolos que contiene las variables de entorno de
 * nuestra shell, buscando la variable de nombre "nombre". Si la encuentra, se
 * para y devuelve la pos, si no devuelve NULL.
 *
 * @param  nombre 		nombre de la variable a buscar
 * @return        		pos de la var "nombre", NULL si no la encuentra
*/
static struct varslot *hallar(char *nombre) {
	int i;
	struct varslot *v;

	v = NULL;
	for (i=0; i<MAXVAR; i++)
		if (simbolos[i].nombre == NULL) {
			if (v == NULL) v = &simbolos[i];
		} else if (strcmp(simbolos[i].nombre, nombre) == 0) {
			v = &simbolos[i];
			break;
		}
	return v;
}


/**
 * usar como funcion "privada"
 * Dado un array de strings, asigna un string a una posicion de ese array,
 * reservando la memoria necesaria. Si no puede alocar memoria, devuelve false.
 *
 * @param  p 		array de strings
 * @param  s 		strings
 * @return  		TRUE si ha podido asignar, FALSE si no puede alocar memoria
 */
static BOOL asignar(char **p, char *s) {
	int tam;

	tam = strlen(s) + 1;
	if (*p == NULL) {								// si no existe el string, hago malloc
		if ((*p = malloc(tam)) == NULL)
			return FALSE;
	} else if ((*p = realloc(*p, tam)) == NULL)		// si ya existe, hago realloc
			return FALSE;
	strcpy(*p, s);									// le doy valor al string
	return TRUE;
}


/**
 * Recibe una variable y su valor. Comprueba si no esta presente ya, y si no es
 * así la asigna.
 *
 * @param  nombre
 * @param  valor
 * @return        TRUE si éxito, FALSE si la variable ya existe o eoc
 */
BOOL EVasignar(char *nombre, char *valor) {
	struct varslot *v;

	if ((v = hallar(nombre)) == NULL) 									// buscamos la pos de nombre en "simbolos"
		return(FALSE);													// no la ha encontrado  //TODO hacer algo? crear la variable?
	return (asignar(&v->nombre, nombre) && asignar(&v->valor, valor));	// asignamos la variable en "simbolos"
}


/**
 * Recibe una variable y su valor. Comprueba si no esta presente ya, y si no es
 * así la asigna, con valor vacío, y la marca como exportada
 *
 * 																		//TODO mi no entender para q es esto! ¿que diferencia hay con EVasignar ademas de q la marca como exportada??
 * @param  nombre
 * @return        TRUE si éxito, FALSE si la variable ya existe o eoc
 */
BOOL EVexportar(char *nombre) {
	struct varslot *v;

	if ((v = hallar(nombre)) == NULL)									// buscamos la pos de nombre en "simbolos"
		return(FALSE);
	if (v->nombre == NULL)
		if (!asignar(&v->nombre, nombre) || !asignar(&v->valor, ""))	// asigno nombre, y valor a vacio
			return FALSE;
	v->exportado = TRUE;												// marco como exportada la variable
	return TRUE;
}

/**
 * Devuelve el valor de una variable
 * 																		//TODO mi no entender para q es esto! ¿que diferencia hay con EVasignar ademas de q la marca como exportada??
 * @param  nombre
 * @return        valor de variable o NULL si no está
 */
char *EVobtener(char *nombre) {
	struct varslot *v;

	if ((v = hallar(nombre)) == NULL || v->nombre == NULL)
		return(NULL);
	return v->valor;
}


/**
 * Recibe un array de variables entorno y las exporta a nuestra shell
 *
 * @param  entorno 		array de variables de entorno del programa
 * @return         		TRUE si tiene éxito, FALSE eoc
 */
BOOL EViniciar(char **entorno) {
	int i, tam_nombre;
	char nombre[256];	// problemas con dimensiones de nombre[]

	for (i=0; entorno[i] != NULL; i++) {					// para cada variable de entorno
		tam_nombre = strcspn(entorno[i], "=");     			// recogemos el nombre , eligiendo la pos hasta justo "=" inclusive
		strncpy(nombre, entorno[i], tam_nombre);			// copiamos el nombre de la variable + "="
		nombre[tam_nombre] = '\0';							// damos formato correcto al string (tenianos una pos mas, por el "=")
		if (!EVasignar(nombre, &entorno[i][tam_nombre+1])	// introducimos la variable en "simbolos", con el valor
				|| !EVexportar(nombre))						// TODO
			return FALSE;
	}
	return TRUE;
}


/**
 * Actualiza environ según las variables marcadas como exportables en "simbolos"
 *
 * (se suele llamar al hacer hijos)
 *
 * @param
 * @return         		TRUE si tiene éxito, FALSE si no puede reservar memoria al hacer malloc o realloc
 */
BOOL EVactualizar(void) {
	int i, envi, nvtam;
	struct varslot *v;
	//char **envp;
	static BOOL actualizado = FALSE;

	if (!actualizado)
		if ((environ = (char**)malloc((MAXVAR+1)*sizeof(char *))) == NULL)
			return FALSE;
	envi = 0;
	for (i=0; i<MAXVAR; i++) {
		v = &simbolos[i];													// apunta a la var entorno en simbolos[i]
		if (v->nombre == NULL || !v->exportado)								// no tiene nombre o no esta marcada como exportable: sigo
			continue;
		nvtam = strlen(v->nombre) + strlen(v->valor) + 2;					// nvtam == strlength("nombre=valor\0")
		if (!actualizado) {													// reservo espacio en environ[envi] si no actualizado ya
			if ((environ[envi] = malloc(nvtam)) == NULL)
				return FALSE;
		} else if ((environ[envi] = realloc(environ[envi], nvtam)) == NULL)	// reservo espacio en environ[envi] si no actualizado ya
			return FALSE;
		sprintf(environ[envi], "%s=%s", v->nombre, v->valor);
		envi++;
	}
	environ[envi] = NULL;
	actualizado = TRUE;
	return TRUE;
}



void EVmostrar(void) {
	int i;

	for (i=0; i<MAXVAR; i++)
		if (simbolos[i].nombre != NULL)
			printf("%3s %s=%s\n", simbolos[i].exportado ? "[E]" : "",
				simbolos[i].nombre, simbolos[i].valor);
}

