/*
 * alarma_mutex.c
 *
 * Esta es una mejora del programa alarma_mutex.c, que
 * introduce una variable de condicion con espera temporizada
 * para permitir interrumpir una alarma en curso cuando se solicita
 * una a larma mas urgente.
 */

#include "alarma.h"

// declara e inicializa alarma_mutex y alarma_cond
pthread_mutex_t alarma_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarma_cond = PTHREAD_COND_INITIALIZER;

alarma_t        *alarma_lista = NULL;
time_t          alarma_actual = 0;

int fin_solicitado = 0;

int main(int argc, char *argv[])
{
	int estado;
	char linea[128];
	alarma_t *alarma;
	pthread_t thread;

	setbuf(stdin, NULL);

	// crea el thread que procesa las alarmas
	if ((estado = pthread_create(&thread, NULL, alarma_thread, NULL)) != 0)
		err_abort(estado, "Creacion del thread alarma\n");

	while (1) {
		printf ("Alarma> ");
		if (fgets(linea, sizeof (linea), stdin) == NULL) break;
		if (strlen (linea) <= 1) continue;
		if (!isatty(0)) printf("%s", linea);

		alarma = (alarma_t*)malloc(sizeof (alarma_t));
		if (alarma == NULL) errno_abort("malloc alarm");

		/*
		 * Divide la linea de entrada en segundos (%d) y un mensaje
		 * (%64[^\n]), consistente en hasta 64 caracteres
		 * separados de los segundos por espacios.
		 */
		if (sscanf (linea, "%d %64[^\n]", &alarma->segundos, alarma->mensaje) < 2) {
			if (strcmp(linea, "exit\n") == 0) {
				fin_solicitado = 1;
				if (alarma_actual == 0) {
					if ((estado = pthread_cond_signal(&alarma_cond)) != 0)
						err_abort(estado, "Fallo de join\n");
				}
				if ((estado = pthread_join(thread, NULL)) != 0)
					err_abort(estado, "Fallo de join\n");
				else { // join tiene exito
					printf("FIN\n");
					pthread_cond_destroy(&alarma_cond);
					pthread_mutex_destroy(&alarma_mutex);
					pthread_exit(NULL);
				}
			}
			else {
				warnx("Entrada erronea\n");
			free (alarma);
			}
		}
		else {

			// inserta la alarma en la lista, dentro de una seccion critica
			if ((estado = pthread_mutex_lock(&alarma_mutex)) != 0)
				err_abort(estado, "Lock hilo inicial\n");

			// el tiempo actual + el número de segundos de la alarma es el criterio de orden de expiracion
			alarma->tiempo = time(NULL) + alarma->segundos;
			alarma_inserta(alarma);

			if ((estado = pthread_mutex_unlock(&alarma_mutex)) != 0) // fin seccion critica
				err_abort(estado, "Unlock hilo inicial\n");
		}
	}
	pthread_exit(NULL);
}
