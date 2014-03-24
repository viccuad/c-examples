/*
 * alarma_mutex.c
 *
 * Utiliza un solo alarma_thread, que toma la
 * siguiente solicitud de una lista. El thread prinicipal (main)
 * va colocando las nuevas solicitudes en la lista, ordenadas
 * por el tiempo absoluto de expiracion. La lista esta protegida
 * por un mutex, y el alarma_thread duerme durante al menos 1 segundo
 * en cada iteracion, para asegurar que el thread principal pueda
 * bloquear (lock) el mutex cuando añada nuevo trabajo a la lista.
 */

#include "alarma.h"

pthread_mutex_t alarma_mutex = PTHREAD_MUTEX_INITIALIZER;

int fin_solicitado = 0;


alarma_t *alarma_lista = NULL;


int main (int argc, char *argv[])
{
	int estado;
	char linea[128];
	alarma_t *alarma;
	pthread_t thread;

	setbuf(stdin, NULL);

	if ((estado = pthread_create(&thread, NULL, alarma_thread, NULL)) != 0)
		err_abort(estado, "Creacion del thread alarma\n");

	while (1) {
		printf ("Alarma> ");
		if (fgets (linea, sizeof (linea), stdin) == NULL)
			break;
		if (strlen (linea) <= 1)
			continue;
		if (!isatty(0))
			printf("%s", linea);
		if ((alarma = (alarma_t*) malloc(sizeof(alarma_t))) == NULL)
			errno_abort("malloc alarm\n");

		/*
		 * Divide la linea de entrada en segundos (%d) y un mensaje
		 * (%64[^\n]), consistente en hasta 64 caracteres
		 * separados de los segundos por espacios.
		 */
		if (sscanf (linea, "%d %64[^\n]", &alarma->segundos, alarma->mensaje) < 2) {
			if (strcmp(linea, "exit\n") == 0) {
				fin_solicitado = 1;
				if ((estado = pthread_join(thread, NULL)) > 0)
					err_abort(estado, "Fallo de join\n");
				else {
					printf("FIN\n");
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
	pthread_mutex_destroy(&alarma_mutex);
    pthread_exit(NULL);
}
